#define MAXMSG 81920

struct cctpmsg {
        long    mtype;
        unsigned char  mtext [MAXMSG];
};


# include <stdio.h>
# include <stdlib.h>             /* fuer 'malloc()', 'exit()' */
# include <string.h>             /* fuer 'strlen()', 'memset()' */

# include <limits.h>

# include <sys/socket.h>
# include <netinet/in.h>
#include <netdb.h>
# include <arpa/inet.h>

# include <sys/msg.h>            /* fuer 'msgrcv()', msgsnd()' */

# include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>             /* fuer 'getpid()', 'fork()' */
#include <sys/stat.h>
#ifdef hpux
# include <sys/mknod.h>
#endif
# include <sys/ipc.h>
#include <sys/param.h>
#include <sys/types.h>

# include <signal.h>

# include <errno.h>
extern int errno;

#define MAX_SOCKETS 128
#define MASK(f)     (1 << (f))
#define NWORDS  howmany(MAX_SOCKETS, NFDBITS)
typedef struct gui_tab {
   int addr_port;
   int server_port;
   int ust_port;
   int reception_socket;
   int active_socket;
   int not_alive;
   unsigned long inet_addr;
   char name[64];
   char host[64];
   char display[64];
   char title[64];
   char process[128];
   int is_listed;
   struct gui_tab *next;
   struct gui_tab *prev;
} GUITAB;

extern int read_tab(char *line, GUITAB *guitab);
extern int sgetline (char *fp, char *buf);

extern int tpopen (int port, u_long ip);

