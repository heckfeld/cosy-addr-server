
   
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
#define EXTERN
#define BUF_LEN 128

#include <server.h>
#include <errno.h>
extern int errno;
unsigned int alarm_intvl = 60;

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

int
main (int argc, char *argv[])
{
   char *tmp;
   int i;             /* Zaehlindex */
   int c;
   int option_port = 0;
   GUITAB *guxtab;

   table_init();

   while ((c = getopt(argc, argv, "aguhdntp")) != -1)
   {
      option_port = c;
   }


   if ( (tmp = getenv("TCL")) == (char*) 0)
   {
      sprintf (gui_conf_name, "%s/%s",default_dir_name, default_conf_name);
   }
   else
   {
      sprintf (gui_conf_name, "%s/conf/%s", tmp, default_conf_name);
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
      guxtab = allocate_guitab();
      i = read_tab(line, guxtab);
      if (i == 8)
      {
         if ( strcmp( guxtab->name, argv[optind]) == 0)
         {
            switch  (option_port)
            {
               case 'a': printf("%d\n", guxtab->addr_port); break;
               case 'g': printf("%d\n", guxtab->server_port); break;
               case 'u': printf("%d\n", guxtab->ust_port); break;
               case 'h': printf("%s\n", guxtab->host); break;
               case 'd': printf("%s\n", guxtab->display); break;
               case 'n': printf("%s\n", guxtab->name); break;
               case 't': printf("%s\n", guxtab->title); break;
               case 'p': printf("%s\n", guxtab->process); break;
               default: printf("0\n"); exit(1);
            }
            return (0);
         }
      }
   }
   printf("0\n");
   exit (2);
}

