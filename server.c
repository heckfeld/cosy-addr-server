
   
/******************************************************************************/
/*                                                                            */
/* server.c                                                                  */
/*                                                                            */
/******************************************************************************/

/****
      Schema:
      Konfiguration aus $TCL/conf/guis.conf auslesen und in giutab eintragen.

      Fuer jeden Eintrag Socktet 's' unter der Port-Nummer  oeffnen und
      in guitab  unter .reception_socket eintragen. 

      Bei Anmeldung einer Oberflaeche mit 'accept' unter dem Socket 
      .active_socket eintragen.
****/

/* avoid duplicate definitions */
#define BUF_LEN 128

#include <stdlib.h>
#include <syslog.h>
#include <libgen.h>

#include "server.h"
#include <errno.h>
extern int errno;
unsigned int alarm_intvl = 60;

void release_connection();

void information();

struct cctpmsg *msgs;

/** guitab[] enthalten die Namen, Server-Ports ,
    Adressen-Server-Port-Nummern und Zeiger auf den geoeffneten 
    Adressen-Server-Socket fuer die entsprechende Oberflaeche.
**/
GUITAB guitab[MAX_SOCKETS+1];
GUITAB *free_guitab_head, *guitab_head;

int s;
FILE *gui_conf;
FILE *gui_host;
char  gui_conf_name[80];

pid_t server_pid;
FILE *addr_serv_pid;
char addr_serv_pid_name[80];
char program_name[80];
char addr_serv_host_name[80];
char gui_host_file[80];

char  *default_conf_dir = "/usr/local/tcl/conf";
char  *default_pid_dir = "/usr/local/tcl/etc";
char  *default_dir_name = "/usr/local/tcl/conf";
char  *default_conf_name = "guis.conf";
char  *default_host_name = "hosts";
char  *default_pid_name = "addr_serv.pid";
#define LINE_LEN 256

char line[LINE_LEN];
char gui_list[MAXMSG];
char   host_name[257];
int    host_node;
struct hostent *host_entry;
struct hostent gethostbyXXX = { "unknown", NULL, AF_INET, 0, NULL };
unsigned long my_inet_addr;

int targ_ls;
socklen_t recv_addrlen;
struct sockaddr_in recv_addr;
#ifdef hpux
struct fd_set readmask, readfds;
#else
fd_set readmask, readfds;
#endif
int nfound;

char mtext[MAXMSG];
char *ping = "PING\n";

int inform = 0;
GUITAB *this_inform;
GUITAB *this_gui;
int this_socket;

void
table_init ()
{
   int k;

   for (k= 0; k < MAX_SOCKETS; k++)
   {
      guitab[k].inet_addr = 0;
      guitab[k].reception_socket = 0;
      guitab[k].active_socket = 0;
      guitab[k].next = &guitab[k+1];
      guitab[k+1].prev = &guitab[k];
   }
   guitab[MAX_SOCKETS - 1].next = (GUITAB *) 0;
   guitab[0].prev = (GUITAB *) 0;
   guitab_head = (GUITAB *) 0;
   free_guitab_head = &guitab[0];
}

void disconn( GUITAB *gt)
{
   int socket = gt->reception_socket;

   close(socket);
   gt->reception_socket = 0;
   FD_CLR (socket, &readmask);
}

GUITAB*
allocate_guitab()
{
   GUITAB* gp = free_guitab_head;
   if (free_guitab_head  != (GUITAB*) 0)
   {
      free_guitab_head = gp->next;
      if (free_guitab_head)
         free_guitab_head->prev = (GUITAB*) 0;

      if (guitab_head)
         guitab_head->prev = gp;
      gp->next = guitab_head;
      gp->prev = (GUITAB*) 0;
      guitab_head = gp;
   }
   return (gp);
}

void
free_guitab( GUITAB *gp)
{
   GUITAB* tmp;
   tmp = gp->next;
   if (tmp)
      tmp->prev = gp->prev;
   tmp = gp->prev;
   if (tmp)
      tmp->next = gp->next;
   else
      guitab_head = gp->next;

   gp->next = free_guitab_head;
   gp->prev = (GUITAB*) 0;
   free_guitab_head = gp;
}

void
release_connection()
{
   char *tm_str;
   time_t t;

   int socket = this_gui->active_socket;
   if ( (socket)  && (this_gui != (GUITAB*) 0))
   {
      FD_CLR(socket, &readmask);
      close (socket);

      t = time(0);
      tm_str = ctime(&t);
      tm_str[strlen(tm_str) - 1] = '\0';
      syslog( LOG_NOTICE,
         "%s: Socket: %d Oberflaeche: \"%s\" geschlossen \n", 
          tm_str, this_gui->active_socket, this_gui->title);

      if (socket == inform)
         inform = 0;

      this_gui->inet_addr = 0;
      this_gui->active_socket = 0;
      this_gui->not_alive = 0;
      information();
   }
}

void 
information()
{
   char tmp[256];
   char list[512];
   struct in_addr i_addr;
   if (inform)
   {
/*
 * Liste der Form:
 *                "name:internetadresse:port:display"
 */
      list[0] = '\0';
      strcat (list, this_gui->name);
      strcat (list, ":");
      i_addr.s_addr = this_gui->inet_addr;
      strcat(list, inet_ntoa(i_addr)); 
      strcat (list, ":");
      sprintf(tmp, "%d:", this_gui->server_port);
      strcat (list, tmp);
      sprintf(tmp, "%s\n", this_gui->display);
      strcat (list, tmp);
      if (write(inform, list, strlen(list)) == -1)
      {
         if (errno != EINTR)
         {
            GUITAB *s_swap = this_gui;
            perror ("Fehler: ");
            this_gui = this_inform;
            release_connection();
            inform = 0;
            this_gui =  s_swap;
         }
      }
   }
}


int
main (int argc, char *argv[])
{ 
   char name[257];
   char *tmp;
   int i, j, k; 
   GUITAB *ix;             /* Zaehlindex */
   int imsg, icount, irec;   /* empfangene Zeichen */
   int max_retries = 10;
   int retries;
   char dest[80], src[80], cmd[MAXMSG];

   int msg_id;
   int check;
   char *tm_str;
   time_t t;
   GUITAB *gt;
   struct in_addr i_addr;

   int failure;

   void sig_handling(int);
   void configure(int);
   void check_alive(int);

   char *this_program;
   char * basec;
   int  opt, optind;

   int daemon = 0;

/* BEGIN */
   basec = strdup( argv[0]);
   this_program = basename( basec);

   if ( (tmp = getenv("TCL")) == (char*) 0) {
      sprintf (gui_conf_name, "%s/%s",default_conf_dir, default_conf_name);
      sprintf (gui_host_file, "%s/hosts",default_conf_dir);
      sprintf (addr_serv_pid_name, "%s/%s.%s",
                     default_pid_dir, default_pid_name, host_name);
   } else {
      sprintf (gui_conf_name, "%s/conf/%s", tmp, default_conf_name);
      sprintf (gui_host_file, "%s/conf/hosts",tmp);
      sprintf (addr_serv_pid_name, "%s/etc/%s.%s", 
                    tmp, default_pid_name, host_name);
   }

   while ((opt = getopt(argc, argv, "dt:")) != -1) {
	switch (opt) {
               case 'd':
		    daemon = 1;
		    break;
               case 't':
		   sprintf (gui_conf_name, "%s/conf/%s", optarg, default_conf_name);
		   sprintf (gui_host_file, "%s/conf/hosts",optarg);
		   sprintf (addr_serv_pid_name, "%s/etc/%s.%s", optarg, default_pid_name, host_name);
                   break;
               default: /* '?' */
                   fprintf(stderr, "Usage: %s [-t TCL path]\n",
                           argv[0]);
                   exit(EXIT_FAILURE);
	}
   }

   signal (SIGPIPE, sig_handling);
   signal (SIGINT, sig_handling);      /* interrupt */
   signal (SIGTERM, sig_handling);     /* software termination signal */
   signal (SIGABRT, sig_handling);     /* schluss, aus, vorbei */
   signal (SIGUSR1, configure);
   signal (SIGALRM, check_alive);

   openlog("addr_serv", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);

   table_init();
   msgs = (struct cctpmsg *) malloc (sizeof (struct cctpmsg));
   recv_addrlen = sizeof (struct sockaddr_in);
   memset ((void *) &recv_addr, 0, recv_addrlen);

   failure = gethostname(host_name, 257);
   if ( failure) {
	fprintf(stderr, "Error with gethostname, exiting\n");
	exit(1);
   }
   host_entry = gethostbyname(host_name);
   if ( host_entry == NULL) {
	fprintf(stderr, "Error with gethostbyname, exiting\n");
	exit(1);
   }
   my_inet_addr = ntohl(*(unsigned long*) host_entry->h_addr);
  
   sprintf (addr_serv_host_name, "unknown");
   gui_host = fopen(gui_host_file, "r");
   if (gui_host == NULL) {
      fprintf(stderr, "Datei %s konnte nicht geoeffnet werden\n", gui_host_file);
      exit (1);
   }
   while (fgets(line, LINE_LEN, gui_host) != NULL) {
      if ( ( i = sscanf( line, "%s %s", program_name, addr_serv_host_name)) == 2 ) {
         if (strcmp (program_name, this_program) == 0)
            break;
      }
   }
   if (strcmp(addr_serv_host_name, "localhost") != 0 && strcmp (host_name, addr_serv_host_name) != 0) {
      fprintf( stderr, "%s muss auf \"%s\" gestartet werden \n", 
               this_program, addr_serv_host_name);
      exit(2);
   }

if (0) {
/**** Ein anderer Server laeuft schon!!!!! ***********/
   if ((addr_serv_pid = fopen(addr_serv_pid_name, "r")) != 0)
   {
      int conversion = fscanf(addr_serv_pid, "%d", &server_pid);
      fclose (addr_serv_pid);
      errno = 0;
      if (kill(server_pid, SIGUSR1) != 0)
      {
/***** Der Server ist abgestuerzt und "addr_serv_pid" uebriggeblieben ****/
         if ( errno == ESRCH)
            unlink(addr_serv_pid_name);
         else 
            exit (1);
      }
      else 
         exit(0);
   }
}

   if (daemon && fork() != 0)
      exit (0);

if (0) {
   addr_serv_pid = fopen(addr_serv_pid_name, "w+");
   if (addr_serv_pid == 0)
   {
      fprintf(stderr, "%s konnte nicht geoeffnet werden! \n", 
                   addr_serv_pid_name);
      exit (2);
   }
   server_pid = getpid();
   i = fprintf(addr_serv_pid, "%d", server_pid);
   fflush(addr_serv_pid);
   fclose (addr_serv_pid);
}

/******
   for (k=0; k < NWORDS; k++)
******/
   for (k=0; k < (MAX_SOCKETS); k++)
      FD_CLR (k, &readmask);

   for (gt = guitab_head; gt; gt = gt->next)
   {
      this_gui->inet_addr = 0;
      gt->reception_socket = 0;
      gt->active_socket = 0;
   }

   configure(SIGUSR1);
   alarm(alarm_intvl);
   check_alive(SIGALRM);

/*
 * Empfangsschleife 
 */
   while (1)
   {
      FD_ZERO(&readfds);
      for (k=0; k < (MAX_SOCKETS); k++)
      {
         if (FD_ISSET(k, &readmask))
         {
            FD_SET (k, &readfds);
         }
      }

      if ((nfound = select (MAX_SOCKETS, &readfds, 0, 0, 0))== -1)
      {
          continue;
      }

      for (this_socket=0, k=0; (k < MAX_SOCKETS) && nfound; 
           k++, j++, this_socket++)
      {
         if (FD_ISSET(k, &readfds))
         {
            this_gui = (GUITAB*)0;
            for ( ix = guitab_head; ix; ix = ix->next)
            {
               if ( (this_socket == ix->active_socket) ||
                    (this_socket == ix->reception_socket) )
               {
                  this_gui = ix;
                  break;
               }
            }

            if (this_socket == ix->reception_socket)
            {
/*** toc toc toc!!! is there anybody? ************************************/
               recv_addrlen = sizeof (struct sockaddr_in);

               s = accept (this_gui->reception_socket, 
                        (struct sockaddr*) &recv_addr, &recv_addrlen);

               if (s != -1)
               {
/**** come in! Oberflaeche noch nicht angemeldet ******************/
                  if (this_gui->active_socket == 0)
                  {
                     this_gui->inet_addr = 
                                recv_addr.sin_addr.s_addr;
                     this_gui->active_socket = s;
                     this_gui->not_alive = 0;
                     t = time(0);
                     tm_str = ctime(&t);
                     tm_str[strlen(tm_str) - 1] = '\0';
                     syslog( LOG_NOTICE,
                        "%s: %s %d Oberflaeche: \"%s\" geoeffnet \n", 
                        tm_str, "Socket:",
                        this_gui->active_socket, 
                        this_gui->title);

                     FD_SET (s, &readmask);
                  
                     sprintf(mtext,"$\n");
                     if (write(s, mtext, strlen(mtext)) == -1)
                     {
                        if (errno != EINTR)
                        {
                           perror ("Fehler: ");
                           release_connection();
                        }
                     }

/******** Erst wenn Display eintragen ist !!!!!!! ***********
                     information();
*************************************************************/
                  }
/**** stay out! Oberflaeche schon angemeldet ******************/
                  else
                  {
		     int numberOfBytesWritten;
                     i_addr.s_addr = this_gui->inet_addr;
                     host_entry = gethostbyaddr ( 
				(char*)&(i_addr.s_addr),
				sizeof (struct in_addr), AF_INET);
		    if (host_entry == NULL)
			host_entry = & gethostbyXXX;
                     sprintf(mtext,"! %s %s \n", host_entry->h_name, 
                         inet_ntoa(i_addr));
                     numberOfBytesWritten = write(s, mtext, strlen(mtext));
                     close (s);
                  }
               } else {
                  syslog( LOG_WARNING, "GUI: %s error in accept: %d \n",
                           this_gui->name, errno);
               }
               nfound--;
               continue;
            }

            irec = recvfrom (this_socket, 
                             msgs->mtext, MAXMSG, 0,
                             (struct sockaddr*)&recv_addr, &recv_addrlen);
            nfound--;

            this_gui->not_alive = 0;

/************ good bye! Oberflaeche beendet ***********/
            if (irec == 0) 
            {
               release_connection();
            }
/******** ??? ***************************************/
            else if (msgs->mtext[0] == '\0')
            {
               continue;
            }

            for ( retries = 0;
                  (retries < max_retries)         && 
                  (irec < MAXMSG)                 && 
                    (msgs->mtext[irec-1] != '\n') &&
                    (msgs->mtext[irec-1] != '\r');
                  retries++)
            {
               int xrec;
               char *xtmp = &msgs->mtext[irec];
               msgs->mtext[irec] = '\0';

               xrec = recvfrom (this_socket, xtmp, MAXMSG, 0, 
                               (struct sockaddr*)&recv_addr, &recv_addrlen);
               if (xrec > 0)
               {
                  *(xtmp + xrec) = '\0';
                  irec += xrec;
               }
            }
/* Immer noch kein <CR> oder <LF> ?  */
            if ( (msgs->mtext[irec-1] != '\n') &&
                 (msgs->mtext[irec-1] != '\r') )
               msgs->mtext[irec++] = '\n';
            msgs->mtext[irec] = '\0';

            imsg = icount = 0;
            while ((icount =  sgetline( &(msgs->mtext)[imsg], mtext)))
            {
               imsg += icount;
               mtext[icount] = '\0';
/******** ??? ***************************************/
/**
**/
               switch (mtext[0])
               {
                  case '$':
/*
 * Befehl an den Server!
 */
                     switch (mtext[1])
                     {
                        case 'g': /* "gc"*/
                           switch (mtext[2])
                           {
/*
 * Liste aller Verbindungen 
 */
                              case 'c':
                                 gui_list[0] = '\0';
                                 strcat (gui_list, "$gc ");
                                 for (ix = guitab_head; ix; ix = ix->next)
                                 {
                                    char tmp[1024];
                                    if (ix->active_socket != 0)
                                    {
                                       strcat (gui_list, ix->name);
                                       strcat (gui_list, ":");
                                       i_addr.s_addr = ix->inet_addr;
                                       strcat(gui_list, 
                                        inet_ntoa(i_addr));
                                       strcat (gui_list, ":");
                                       sprintf(tmp, "%d", 
                                               ix->server_port);
                                       strcat (gui_list, tmp);
                                       strcat (gui_list, " ");
                                    }
                                 }
                                 strcat (gui_list, "\n");
                                 if (write(this_gui->active_socket, 
                                       gui_list, strlen(gui_list)) == -1)
                                    if (errno != EINTR)
                                       release_connection();
                                 break;
/* Liste aller geoeffnetetn Verbindungen ********/
/* Form:
 *         "$gl@name1:internetadresse1:port1:display1:titel1@name2:...
 *
 */
                              case 'l':
                                 gui_list[0] = '\0';
                                 strcat (gui_list, "$gl@");
                                 for (ix = guitab_head; ix; ix = ix->next)
                                 {
                                    char tmp[1024];
                                    strcat (gui_list, ix->name);
                                    strcat (gui_list, ":");
                                    if (ix->active_socket != 0)
                                    {
                                       i_addr.s_addr = ix->inet_addr;
                                       strcat(gui_list, 
                                             inet_ntoa(i_addr));
                                    }
                                    else
                                    {
                                       i_addr.s_addr = 0;
                                       strcat(gui_list, inet_ntoa(i_addr));
                                    }
                                    strcat (gui_list, ":");
                                    sprintf(tmp, "%d", 
                                                 ix->server_port);
                                    strcat (gui_list, tmp);
                                    strcat (gui_list, ":");
                                    sprintf(tmp, "%s", 
                                                 ix->display);
                                    strcat (gui_list, tmp);
                                    strcat (gui_list, ":");
                                    sprintf(tmp, "%s", 
                                                 ix->title);
                                    strcat (gui_list, tmp);
                                    strcat (gui_list, ":");
                                    sprintf(tmp, "%s", 
                                                 ix->process);
                                    strcat (gui_list, tmp);
                                    strcat (gui_list, "@");
                                 }
                                 strcat (gui_list, "\n");
                                 if (write(this_gui->active_socket, 
                                       gui_list, strlen(gui_list)) == -1)
                                    if (errno != EINTR)
                                       release_connection();
                                 break;
/***** Befehl $gt <name> gibt Titel zurueck *****************/
                              case 't': 
                                 sscanf (&mtext[4], "%s", name);
                                 for (ix = guitab_head; ix; ix = ix->next)
                                 {
                                    if (strcmp(ix->name, name) == 0)
                                    {
                                       break;
                                    }
                                 }
                                 sprintf (gui_list, "$gt %s ", name);
                                 strcat (gui_list, ix->title);
                                 strcat (gui_list, "\n");
                                 if (write(this_socket, gui_list, 
                                             strlen(gui_list)) == -1)
                                    if (errno != EINTR)
                                       release_connection();
                                 break;
                              case 'd':
                                 sscanf (&mtext[4], "%s", name);
                                 for (ix = guitab_head; ix; ix = ix->next)
                                 {
                                    if (strcmp(ix->name, name) == 0)
                                    {
                                       break;
                                    }
                                 }
                                 sprintf (gui_list, "$gd %s ", name);
                                 strcat (gui_list, ix->display);
                                 strcat (gui_list, "\n");
                                 if ( write(this_socket, gui_list, 
                                               strlen(gui_list)) == -1)
                                    if (errno != EINTR)
                                       release_connection();
                                 break;
/****** $gh gibt host zurueck *******************************/
                              case 'h': 
                                 sscanf (&mtext[4], "%s", name);
                                 for (ix = guitab_head; ix; ix = ix->next)
                                 {
                         
                                    if (strcmp(ix->name, name) == 0)
                                    {
                                       break;
                                    }
                                 }
                                 sprintf (gui_list, "$gh %s ", name);
                                 strcat (gui_list, ix->host);
                                 strcat (gui_list, "\n");
                                 if (write(this_socket, gui_list, 
                                            strlen(gui_list)) == -1)
                                    if (errno != EINTR)
                                       release_connection();
                                 break;
                              default:
                                 fprintf (stderr, 
                                           "%s: No such command \"%s\"\n",
                                            "server", mtext);
                                 break;
                           }
                           break;

                        case 'p': /* "pd"*/
                           if  (mtext[2] == 'd') 
                           {
                              GUITAB *tmp;
                              char *sp;
			      char display[64];
/******* $pd <name> <display> traegt Display ein *************/
                              sscanf (&mtext[4], "%s %s", name, display);
/*
 *     Extension ':0.0' , falls vorhanden wegwerfen!
 */
                              for (sp = display; (*sp != '\0') && (*sp != ':'); *sp++) ;
                              *sp = '\0';

                              for (ix = guitab_head; ix; ix = ix->next)
                              {
                                 if (strcmp(ix->name, name) == 0)
                                 {
                                    break;
                                 }
                              }
                              ix->display[0] = '\0';
                              strcat (ix->display, display);
/******* Erst jetzt den Master informieren !!!! ********************/
                              tmp = this_gui;
                              this_gui = ix;
                              information();
                              this_gui = tmp;
                           }
                           break;
/**** Information bei Veraenderungen! ******/
                        case 'i':
                           this_inform = this_gui;
                           inform = this_socket;
                           break;

/**** Information beenden! ******/
                        case 'c':
                           inform = 0;
                           break;

                        default :
                           fprintf (stderr, "%s: No such command \"%s\"\n",
                             "server", mtext);
                           break;
                     } /* switch (mtext[1]) */
                     break;

/**** Nachrichtenaustausch zwischen Oberflaechen ******/
                  case '>':
                  case '<':
/* Befehl an Oberflaeche der Form:
 *
 *          (Ziel,Quelle,ID) Befehl  (NEU)
 * oder     (Ziel,Quelle) Befehl     (ALT)
 */

                     check = sscanf( &mtext[1], 
                          "(%[^ ,] %*c %[^ ,)] %*c %d %*c %[\011-\176]",
                             dest, src, &msg_id, cmd );
                     
                     if  (check == 4)
                     {
                        int found;
                        for (found = 0, ix = guitab_head; 
                                    ix; ix = ix->next)
                        {
                           if (strcmp(ix->name, dest) == 0)
                           {
                              found = 1;
                              break;
                           }
                        }
                        if ( (found ) && (ix->active_socket))
                        {
                           if (write(ix->active_socket,
                                              mtext, strlen(mtext)) == -1)
                           {
                              if (errno != EINTR)
                              {
                                 GUITAB *s_swap = this_gui;
                                 this_gui = ix;
                                 release_connection();
                                 this_gui =  s_swap;
                              }
                           }
                        }
                        else if (mtext[0] == '>')
                        {
                           char s[128];
                           sprintf (s, "<(%s,%s,%d) IDLE\n",
                                  src, dest, msg_id);
                           if (write(this_gui->active_socket,
                                   s, strlen(s)) == -1)
                              if (errno != EINTR)
                                 release_connection();
                        }
                     }
                     break;
                  case 'P':
                     if (strncmp (mtext, "PING", 4) == 0)
                     {
                        this_gui->not_alive = 0;
                     }
                     break;
                
               } /* switch ( mtext[0]) */
               
            } /* while ( icount =  sgetline( &(msgs->mtext)[imsg], mtext) */
         }    /* if (FD_ISSET(k, &readfds))                               */
      }          /* for (this_socket=0, k=0; (k < (MAX_SOCKETS/NFDBITS)) ... */
   }             /* while (1) */
   exit(0);
}

/*
 * Special Reaction on received Signal ... (Ignorance or Termination)
 */

void sig_handling (sig)
int sig;
{
   GUITAB *ix;

   switch (sig) 
   {
/* 
   Der Server darf nicht beendet werden!!!!!
*/
      case SIGTERM:
      case SIGINT:
      case SIGPIPE:
         signal (sig, sig_handling);
         break;
      default:
         syslog (LOG_WARNING, "server: Termination (%d) on signal %d.\n", 
                  getpid (), sig);
         for (ix = guitab_head; ix; ix = ix->next)
         {
            if (ix->active_socket)
               close (ix->active_socket);
            if (ix->reception_socket)
               close (ix->reception_socket);
         }

         unlink(addr_serv_pid_name);
	 closelog();
         exit (1);
         break;
   }
}

void configure(int sig)
{
  int i;
  GUITAB *gt, *th;


  for (th = guitab_head; th; th = th->next)
  {
     th->is_listed = 0;
  }


   gui_conf = fopen(gui_conf_name, "r");
   if (gui_conf == 0)
   {
      fprintf(stderr, "Datei %s konnte nicht geoeffnet werden\n", 
              gui_conf_name);
      exit (1);
   }

   while (fgets(line, LINE_LEN, gui_conf) != 0)
   {
      gt = allocate_guitab();
      i = read_tab(line, gt);
      if  (i >= 4) 
      {
         for (th = gt->next; th; th = th->next)
         {
            if ((gt->addr_port == th->addr_port) ||
                    (strcmp(gt->name,  th->name) == 0) )
            {
               th->is_listed = 1;
               break;
            }
         }
         if (th != (GUITAB*) 0)
            free_guitab(gt);
         else
         {
            gt->reception_socket = 0;
            gt->active_socket = 0;
            gt->is_listed = 1;
         }
      }
      else
         free_guitab(gt);
   }
   fclose(gui_conf);

   for (th = guitab_head; th; th = th->next)
   {
     if ( (th->is_listed == 0) && ( th->active_socket == 0))
     {
        disconn (th);
        gt = th->prev;
        free_guitab(th);
        if (!gt)
           gt = guitab_head;
        th = gt;
     }
   }
   for (th = guitab_head; th; th = th->next)
   {
     if ( th->reception_socket == 0)
     {
        if ((s = tpopen (htons(th->addr_port), my_inet_addr)) == -1)
        {
          perror ("Fehler beim Oeffnen: ");
          continue;
        }
        syslog (LOG_NOTICE, "GUI: %s Port %d auf %s geoeffnet  \n",
                  th->name, th->addr_port, host_name);
        fflush (stderr);

 /*
 Aus MAN-PAGE accept(2):
 NOTES
       There may not always be a connection waiting after a SIGIO is  delivered  or
       select(2) or poll(2) return a readability event because the connection might
       have been removed by an asynchronous network error or another thread  before
       accept  is called.  If this happens then the call will block waiting for the
       next connection to arrive.  To ensure that accept never blocks,  the  passed
       socket s needs to have the O_NONBLOCK flag set (see socket(7)).

 */
        fcntl(s, F_SETFL, O_NONBLOCK);

        th->reception_socket = s;
        th->active_socket = 0;
        FD_SET (s, &readmask);
     }
   }
   signal (SIGUSR1, configure);
}

void check_alive(sig)
int sig;
{
  int l;
  GUITAB *tmp_gui;
  GUITAB *gt;


  for (gt = guitab_head; gt; gt = gt->next)
  {

/* Wenn Verbindung besteht, pruefen!!!!! */
     if ( gt->active_socket != 0)
     {
/*
   Verbindung unterbrochen => loesen!!!
*/
        if (gt->not_alive > (10 * alarm_intvl) )
        {
           tmp_gui = this_gui;
           this_gui = gt;
           release_connection();
           this_gui = tmp_gui;
        }
/** 
  Immer noch keine Antwort: weiter pruefen!
**/
        else if (gt->not_alive > 0)
        {
           gt->not_alive += alarm_intvl;
           l = write (gt->active_socket, ping, strlen(ping));
           if ((l == -1) && (errno != EINTR))
           {
              tmp_gui = this_gui;
              this_gui = gt;
              release_connection();
              this_gui = tmp_gui;
           }
        }
/*** 
    Wenn zwischenzeitlich kein Lebenszeichen, wird beim naechsten Mal geprueft 
******/
        else if (gt->not_alive == 0)
        {
           gt->not_alive++;
        }
    }
  }
  signal (SIGALRM, check_alive);
  alarm(alarm_intvl);
}

