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

char gui_host_file[80];
FILE *gui_host;
char  *default_conf_dir = "/usr/local/tcl/conf";
char  *default_host_name = "hosts";
char  *default_host_addr = "0.0.0.0";
char  *progname = "addr_serv";
#define LINE_LEN 256
char line[LINE_LEN];
char program_name[80];

main (argc, argv)
int argc;
char *argv[];
{ 
   char *tmp;
   char   host_name[80];
   char   h_name[80];
   char   *host_addr;
   char   *namep;
   struct hostent *host_entry;
   struct in_addr my_inet_addr;
   int i, j, len;

   if ( argc <= 1)
   {
      fprintf (stderr, "unknown\n");
      exit (1);
   }

   host_addr = argv[1];

   i = inet_aton( host_addr, &my_inet_addr);
   host_entry = gethostbyaddr((char *)&(my_inet_addr.s_addr), sizeof(my_inet_addr.s_addr), AF_INET );

   if (host_entry == NULL)
   {
      fprintf (stderr, "unknown\n");
      exit (1);
   }

   len = strlen (host_entry->h_name);
   for (namep = host_entry->h_name, i = 0; (i < len) && (*namep != '.'); i++, namep++)
      host_name[i] = *namep;
   host_name[i] = '\0';
   
   

   printf("%s\n", host_name);

   exit(0);
}
