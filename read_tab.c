/******************************************************************************/
/*                                                                            */
/* $Header: server.c,v 1.23 93/03/30 16:19:12 tine Exp $                      */
/*                                                                            */
/******************************************************************************/

/*avoid duplicate definitions */
# include <stdio.h>
#include <unistd.h>
# include <string.h>             /* fuer 'strlen()', 'memset()' */

# include <fcntl.h>
# include <server.h>

int
read_tab(char *line, GUITAB *guitab)
{
  char *s = line;
  char *u;
  char tmp[256];
  char *t = tmp;
  int count = 0;
  int j, k, ix;
  
  while ((*s != '#') && (*s != '\n'))
     *t++ = *s++;
  *t = '\0';
  s = tmp;

  for (j = 1; j <= 8; j++)
  {
     if (*s == '\0')
        return (count);

     while ( (*s == ' ') || (*s =='\t') )
     {
        s++;
     }
     if (*s == '\0') 
        return (count);

     switch (count)
     {
         case 0:
            k = sscanf(s, "%s", &guitab->name[0]);
            break;
         case 1:
            k = sscanf(s, "%d", &guitab->addr_port);
            break;
         case 2:
            k = sscanf(s, "%d", &guitab->server_port);
            break;
         case 3:
            k = sscanf(s, "%d", &guitab->ust_port);
            break;
         case 4:
            k = sscanf(s, "%s", &guitab->host[0]);
            break;
         case 5:
            k = sscanf(s, "%s", &guitab->display[0]);
            break;
         case 6:
            if (*s != '"') 
            {
               k = 0;
               break;
            }
            s++;
            u = guitab->title;
            for (ix = 0; ix < 63; ix++)
            {
               if (*s == '"')
               {
                  s++;
                  break;
               }
               *u++ = *s++;
            }
            *u = '\0';
            k = strlen(guitab->title);
            break;
         case 7:
            k = sscanf(s, "%s", &guitab->process[0]);
            break;
     }
     if (k)
        count++;
     if (count < 8)
        while ( (*s != ' ') && (*s != '\t') )
           s++;
  }

  while ( (*s == ' ') && (*s =='\t') )
  {
     if (*s == '\0') 
        return (count);
     s++;
  }

  return (count);
}
