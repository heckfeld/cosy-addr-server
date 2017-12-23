/******************************************************************************/
/*                                                                            */
/* $Header: server.c,v 1.23 93/03/30 16:19:12 tine Exp $                      */
/*                                                                            */
/******************************************************************************/

# include <stdio.h>
# include <stdlib.h>             /* fuer 'malloc()', 'exit()' */
# include <string.h>             /* fuer 'strlen()', 'memset()' */

# include <sys/socket.h>
# include <netinet/in.h>
#include <netdb.h>
# include <arpa/inet.h>

# include <unistd.h>             /* fuer 'getpid()', 'fork()' */
# include <sys/msg.h>            /* fuer 'msgrcv()', msgsnd()' */

# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>
//# include <sys/mknod.h>
# include <sys/ipc.h>
#include <sys/param.h>
#include <sys/types.h>

char gui_host_file[BUFSIZ];
FILE *gui_host;
char  *default_conf_dir = "/usr/local/tcl/conf";
char  *default_host_name = "hosts";
char  *progname = "addr_serv";
char line[BUFSIZ];
char program_name[BUFSIZ];

main (argc, argv)
int argc;
char *argv[];
{ 
   char *tmp;
   char   host_name[BUFSIZ];
   char   *host_node;
   struct hostent *host_entry;
   struct in_addr my_inet_addr;
   int nitems;
   char *lp;

   if ( (tmp = getenv("TCL")) == (char*) 0)
   {
      sprintf (gui_host_file, "%s/hosts",default_conf_dir);
   }
   else
   {
      sprintf (gui_host_file, "%s/conf/hosts",tmp);
   }
   gui_host = fopen(gui_host_file, "r");
   if (gui_host == 0)
   {
      fprintf(stderr, "Datei %s konnte nicht geoeffnet werden\n",
              gui_host_file);
      exit (1);
   }
   while ((lp = fgets(line, sizeof(line), gui_host)) != NULL)
   {
      if ( ( nitems = sscanf( line, "%s %s", program_name, host_name)) == 2 )
      {
         if (strcmp (program_name, progname) == 0)
            break;
      }
   }
   if (lp == NULL)
   {
      fprintf(stderr, "Kein Eintrag %s in der Konfigurationsdatei %s \n",
              progname, gui_host_file);
      exit (1);
   }

   host_entry = gethostbyname(host_name);
   if ( host_entry == NULL ) {
      fprintf(stderr, "Unbekannter Rechner %s in der Konfigurationsdatei %s \n",
              host_name, gui_host_file);
      exit (1);
   } else {
      my_inet_addr.s_addr = *(unsigned long*) (host_entry->h_addr);
      host_node = inet_ntoa(my_inet_addr);
      printf("%s\n", host_node);
      exit(0);
   }
}
