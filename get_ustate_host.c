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
# include <sys/ipc.h>
#include <sys/param.h>
#include <sys/types.h>


main (argc, argv)
int argc;
char *argv[];
{ 
   char   host_name[256];
   char   *host_node;
   struct hostent *host_entry;
   struct in_addr my_inet_addr;

   gethostname(host_name, 256);
   host_entry = gethostbyname(host_name);
   my_inet_addr = *(struct in_addr*) (host_entry->h_addr);
   host_node = inet_ntoa(my_inet_addr);
   printf("%s\n", host_node);
   exit(0);
}
