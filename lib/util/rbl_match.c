/*
 * $Id: rbl_match.c,v 1.3 2000/01/18 06:50:39 krueger Exp $
 *
 *
 */

#include "util.h"
#include "mmdf.h"
#include "cmd.h"
#include "ch.h"

#if defined(HAVE_NAMESERVER) && defined(HAVE_RBL)
#include <arpa/nameser.h>
#include <resolv.h>
#include <string.h>

extern int tb_rbl_tai();
extern int tb_rbl_fetch();

extern LLog *logptr, chanlog;

typedef struct tb_rbl_param {
  char *domain;
  char *link;
} tb_rbl_param_def;


int tb_rbl_init(tblptr)
Table *tblptr;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_rbl_init (%p)", tblptr);
#endif

  tblptr -> tb_tai   = &tb_rbl_tai;
  /*tblptr -> tb_k2val = NULL;*/
  /*tblptr -> tb_print = &tb_rbl_print;*/
  /*tblptr -> tb_check = &tb_rbl_check;*/
  tblptr -> tb_fetch = &tb_rbl_fetch;

  return;
}

/*************************************************************/
#define CMDTNOOP    0
#define CMDTDMN     1
#define CMDTLINK    2

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",         CMDTNOOP,   0},
    {"domain",   CMDTDMN,    1},
    {"link", CMDTLINK,   1},
/*    {"", CMDTxxxx, 1},*/
    {0,          0,          0}
};

#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

int tb_rbl_tai(tbptr, gind, argc, argv)
    Table *tbptr;
    int *gind;
    int argc;
    char *argv[];
{
  int ind = *gind - 1;
  struct tb_rbl_param *param;
  if(tbptr->tb_parameters==NULL)
    tbptr->tb_parameters = param =
      (struct tb_rbl_param *)malloc(sizeof(struct tb_rbl_param));
  else param = tbptr->tb_parameters;
  
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb_rbl_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTNOOP:
        break;

      case CMDTDMN:
        param->domain = strdup(argv[ind+1]);
        break;

      case CMDTLINK:
        param->link = strdup(argv[ind+1]);
        break;
        
      default:
        tai_error ("unknown table parm", argv[ind], argc, argv);
        break;
  }
  printf(">>>>%s<<<\n", param->domain);
  
  return 0;
}

/*************************************************************/
int tb_rbl_fetch (table, name, buf, first) /* use key and return value */
register Table  *table;           /* I: */
register char  name[];            /* I: name of ch "member" / key          */
char   *buf;                      /* O: put value int this buffer          */
int     first;                    /* I: start at beginning of list?        */
{
  struct tb_rbl_param *param = (struct tb_rbl_param *)table->tb_parameters;

  rbl_match(param->domain, "192.168.100.11");
  return (OK);
  
}

int rbl_match(rbl_domain, rbl_hostaddr)
char   *rbl_domain;                           /* RBL domain */
char   *rbl_hostaddr;                         /* hostaddr */
{
  char *rbl_name;
  int rbl_size = 0;
  unsigned long h_addr;
  int ret = NO;
  extern char *malloc();
 
  printf(" rbl_match(%s, %s)\n", rbl_domain, rbl_hostaddr);
  if ((h_addr = dot_quad_addr(rbl_hostaddr)) == INADDR_NONE) {
    /*tcpd_warn("unable to convert %s to address", rbl_hostaddr);*/
    return (NO);
  }
  printf("2) rbl_match(%s, %s)\n", rbl_domain, rbl_hostaddr);

  /*  construct the rbl name to look up */
  rbl_size = strlen(rbl_domain) + (4*4) + 2;
  if ((rbl_name = malloc(rbl_size)) == NULL) {
    /*
      tcpd_jump("not enough memory to build RBL name for %s in %s",
              rbl_hostaddr, rbl_domain);
    /* NOTREACHED */
    return(NO);
  }
  printf("3) rbl_match(%s, %s)\n", rbl_domain, rbl_hostaddr);
  snprintf(rbl_name, rbl_size, "%u.%u.%u.%u.%s",
          (unsigned int) ((h_addr) & 0xff),
          (unsigned int) ((h_addr >> 8) & 0xff),
          (unsigned int) ((h_addr >> 16) & 0xff),
          (unsigned int) ((h_addr >> 24) & 0xff),
          rbl_domain);
  /* look it up */
  printf("4) rbl_match(%s, %s) %s\n", rbl_domain, rbl_hostaddr, rbl_name);
  if (gethostbyname(rbl_name) != NULL) {
    /* successful lookup - they're on the RBL list */
    ret = YES;
  }
  printf("5) rbl_match(%s, %s)\n", rbl_domain, rbl_hostaddr);
  free(rbl_name);
  printf("6) rbl_match(%s, %s)\n", rbl_domain, rbl_hostaddr);

  return ret;
}

/*************************************************************/
void reject_message(tbptr, host, p, size)
Table *tbptr;
char *host;
char *p;
int size;
{
  long csize;
  struct tb_rbl_param *param = (struct tb_rbl_param *)tbptr->tb_parameters;

  snprintf(p, size, "571"); csize+=3;
  
  if(param->link != NULL) {
    snprintf(p, size-csize, "%s Open spam relay %s; see %s\r\n", p,
             host, param->link);
    csize += (strlen(param->link)+7);
  } else {
    snprintf(p, size-csize, "%s rejected due to SPAM list\r\n", p);
    csize += 28;
  }
}

#endif/* not HAVE_NAMESERVER && HAVE_RBL */
