/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 * $Id: cnvtbytestr.c,v 1.2 2001/02/28 21:37:16 krueger Exp $
 *
 *
 * Written by Kai Krueger
 * Email: krueger@mathematik.uni-kl.de                                            *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include <stdio.h>

void cnvtbytestr(str, val)
char *str;
long *val;
{
  if(str==NULL || val==NULL) return;
  
  sscanf(str, "%d", val);
  switch(str[strlen(str)-1]) {
      case 'k':
      case 'K': (*val)*=1000;  break;
                
      case 'm':
      case 'M': (*val)*=1000000;  break;
                
      case 'g':
      case 'G': (*val)*=1000000000;  break;
      default:
        break;
  }
}
