/*
 * $Id: rbl_match.c,v 1.8 2003/03/02 15:08:15 krueger Exp $
 *
 *
 */

#include "util.h"
#include "mmdf.h"
#include "tb_check.h"
#include "cmd.h"
#include "ch.h"

#if defined(HAVE_NAMESERVER) && defined(HAVE_RBL)
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

extern int tb_rbl_tai();
extern int tb_rbl_fetch();
extern int tb_rbl_check();

static unsigned long my_dot_quad_addr();

extern LLog *logptr, chanlog;

static   char h_hostid[32];
static   char *h_hostname = NULL;

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
  tblptr -> tb_check = &tb_rbl_check;
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
/*         param->domain = strdup(argv[ind+1]);*/
        param->domain = argv[ind+1];
        break;

      case CMDTLINK:
/*         param->link = strdup(argv[ind+1]);*/
        param->link = argv[ind+1];
        break;
        
      default:
        tai_error ("unknown table parm", argv[ind], argc, argv);
        break;
  }
  
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
  struct in_addr    hp_addr;
  struct hostent	*hp;
  
  if(h_hostname != NULL) free(h_hostname);
  h_hostname = strdup(name);
  
  memset(&hp_addr, 0, sizeof(struct in_addr));
  hp = gethostbyname(h_hostname);
  
  if (hp != NULL) {
    memcpy(&hp_addr, hp->h_addr, sizeof(struct in_addr));
    snprintf(h_hostid, sizeof(h_hostid), "%s", inet_ntoa(hp_addr));
    
    return(rbl_match(param->domain, h_hostid));
  } else {
    return(rbl_match(param->domain, h_hostid));
  }

  return (NOTOK);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int tb_rbl_check(mmdfuid, mmdfgid, MMDFlogin, tb, hdrfmt, title)
int mmdfuid, mmdfgid;
char *MMDFlogin;
Table *tb;
char *hdrfmt;
char *title;
{
  struct tb_rbl_param *param = (struct tb_rbl_param *)tb->tb_parameters;
  char probfmt[] = "%-24s: (via ns with domain %s)\n";

  if(param->domain != NULL) 
    que(LEVEL6, probfmt, "", param->domain);
  else
    que(LEVEL1, hdrfmt, "no rbl-domain","Cannot match entries without domain");
  if(param->link == NULL)
    que(LEVEL1, hdrfmt, "no rbl-link","not set");
 return 0;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int rbl_match(rbl_domain, rbl_hostaddr)
char   *rbl_domain;                           /* RBL domain */
char   *rbl_hostaddr;                         /* hostaddr */
{
  char *rbl_name;
  int rbl_size = 0;
  unsigned long hp_addr;
  int ret = NOTOK;
 
  if ((hp_addr = my_dot_quad_addr(rbl_hostaddr)) == NOTOK) {
    /*tcpd_warn("unable to convert %s to address", rbl_hostaddr);*/
    return (MAYBE);
  }

  /*  construct the rbl name to look up */
  rbl_size = strlen(rbl_domain) + (4*4) + 2;
  if ((rbl_name = malloc(rbl_size)) == NULL) {
    /*
      tcpd_jump("not enough memory to build RBL name for %s in %s",
              rbl_hostaddr, rbl_domain);
    /* NOTREACHED */
    return(MAYBE);
  }
  snprintf(rbl_name, rbl_size, "%u.%u.%u.%u.%s",
          (unsigned int) ((hp_addr >> 24) & 0xff),
          (unsigned int) ((hp_addr >> 16) & 0xff),
          (unsigned int) ((hp_addr >> 8) & 0xff),
          (unsigned int) ((hp_addr) & 0xff),
          rbl_domain);
  /* look it up */
  if (gethostbyname(rbl_name) != NULL) {
    /* successful lookup - they're on the RBL list */
    ret = OK;
  }
  free(rbl_name);
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
  char *name;
  struct tb_rbl_param *param = (struct tb_rbl_param *)tbptr->tb_parameters;

  if(h_hostname != NULL) {
    if(strcmp(h_hostname, host) == 0) name = h_hostid;
    else name = host;
  } else name = host;
  
  if(param->link != NULL) {
    snprintf(p, size-csize, "571 Open spam relay %s; see %s\r\n",
             name, param->link);
    csize += (strlen(param->link)+10);
  } else {
    snprintf(p, size-csize, "571 rejected due to SPAM list\r\n");
    csize += 31;
  }
}

/*************************************************************/
/* my_dot_quad_addr - convert dotted quad to internal form */

unsigned long my_dot_quad_addr(str)
char   *str;
{
    int     in_run = 0;
    int     runs = 0;
    char   *cp = str;

    /* Count the number of runs of non-dot characters. */

    while (*cp) {
        if (*cp == '.') {
            in_run = 0;
        } else if (in_run == 0) {
            in_run = 1;
            runs++;
        }
        cp++;
    }
    return (runs == 4 ? inet_addr(str) : NOTOK);
}

#endif/* not HAVE_NAMESERVER && HAVE_RBL */
