/*
 * $Id: rbl_match.c,v 1.1 1998/12/17 09:30:04 krueger Exp $
 *
 *
 */

#include "util.h"

#if defined(HAVE_NAMESERVER) && defined(HAVE_RBLSUPPORT)
int rbl_match(rbl_domain, rbl_hostaddr)
char   *rbl_domain;                           /* RBL domain */
char   *rbl_hostaddr;                         /* hostaddr */
{
  char *rbl_name;
  unsigned long h_addr;
  int ret = NO;
  extern char *malloc();
 
  if ((h_addr = dot_quad_addr(rbl_hostaddr)) == INADDR_NONE) {
    /*tcpd_warn("unable to convert %s to address", rbl_hostaddr);*/
    return (NO);
  }
  /*  construct the rbl name to look up */
  if ((rbl_name = malloc(strlen(rbl_domain) + (4*4) + 2)) == NULL) {
    /*
      tcpd_jump("not enough memory to build RBL name for %s in %s",
              rbl_hostaddr, rbl_domain);
    /* NOTREACHED */
    return(NO);
  }
  sprintf(rbl_name, "%u.%u.%u.%u.%s",
          (unsigned int) ((h_addr) & 0xff),
          (unsigned int) ((h_addr >> 8) & 0xff),
          (unsigned int) ((h_addr >> 16) & 0xff),
          (unsigned int) ((h_addr >> 24) & 0xff),
          rbl_domain);
  /* look it up */
  if (gethostbyname(rbl_name) != NULL) {
    /* successful lookup - they're on the RBL list */
    ret = YES;
  }
  free(rbl_name);

  return ret;
}
#endif/* not HAVE_NAMESERVER && HAVE_RBLSUPPORT */
