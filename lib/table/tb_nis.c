/*
 *      TB_NS.C
 *
 * Original version by Phil Cockcroft to use MF and MD records.
 * Also had several speed improvements such as caching.  Later
 * reworked by Phil to do MX lookups.
 *
 * Mostly rewritten by Craig Partridge to move routing decisions
 * into SMTP channel.
 *
 */

#include "util.h"
#include "mmdf.h"
#include "tb_check.h"
#include "ch.h"
#include "cmd.h"

#ifdef HAVE_NIS
extern LLog *logptr, chanlog;
extern int tb_nis_tai();
extern int tb_nis_fetch();
extern int tb_nis_check();


int tb_nis_init(tblptr)
Table *tblptr;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_nis_init (%p)", tblptr);
#endif
  printf( "tb_nis_init (%p, %s)\n", tblptr, tblptr -> tb_name);

  tblptr -> tb_tai   = &tb_nis_tai;
  /*tblptr -> tb_k2val = &tb_nis_k2val;*/
  /*tblptr -> tb_print = &tb_nis_print;*/
  tblptr -> tb_check = &tb_nis_check;
  tblptr -> tb_fetch = &tb_nis_fetch;
  
  return;
}

#define CMDTNOOP    0

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",         CMDTNOOP,   0},
    {0,          0,          0}
};

#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

int tb_nis_tai(tbptr, gind, argc, argv)
    Table *tbptr;
    int *gind;
    int argc;
    char *argv[];
{
  int ind = *gind - 1;
/*  struct tb_ns_param *param;
  tbptr->tb_parameters = param =
    (struct tb_ns_param *)malloc(sizeof(struct tb_ns_param));*/
  
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb_nis_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTNOOP:
        break;
        
      default:
        printf("unknown table parm '%s', %d\n", argv[ind], argc, argv);
        tai_error ("unknown table parm", argv[ind], argc, argv);
        break;
  }

  return 0;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int tb_nis_fetch (table, name, buf, first) /* use key and return value  */
register Table  *table;           /* I: pointer to the current table    */
register char  name[];            /* I: name of ch "member" / key       */
char   *buf;                      /* O: put value int this buffer       */
int     first;                    /* I: start at beginning of list?     */
{
  int retval = NOTOK;
  char *domain;
  int  len;
  char *value = NULL;

  domain = NULL;

  if (domain == NULL) /* get NIS-domain first */
    if ((retval = yp_get_default_domain(&domain)) != 0)
    {
#ifdef DEBUG
      ll_log (logptr, LLOGFTR,
              "tb_fetch(nis): Can't get default domainname. Reason: %s.\n",
              yperr_string(retval));
#endif
      (void) strcpy (buf, "(ERROR)");
      return (NOTOK);	  /* cannot get domainname              */
    }

  retval = yp_match(domain, table->tb_file, name, strlen(name),
                    &value, &len);
  if(!retval) {
#ifdef DEBUG
    ll_log (logptr, LLOGFTR,"tb_fetch: NIS entry found: %s, %d", 
            value, retval);
#endif
    value[len]='\0';
    strcpy(buf, value);
    compress (buf, buf);  /* get rid of extra white space       */
    return (OK);
  }
#ifdef DEBUG
  ll_log (logptr, LLOGFTR, "tb_fetch(nis) failed");
#endif
  (void) strcpy (buf, "(ERROR)");
  return (NOTOK);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
//int tb_nis_check(que, tb, subhdrfmt, title)
int tb_nis_check(mmdfuid, mmdfgid, MMDFlogin, tb, subhdrfmt, title)
int mmdfuid, mmdfgid;
char *MMDFlogin;
Table *tb;
char *subhdrfmt;
char *title;
{
  char *domain;
  int r, order;
  char *master;
  char *inmap;
  char probfmt[] = "%-18s: %s\n";

  domain = NULL;
  if (domain == NULL) /* get NIS-domain first */
    if ((r = yp_get_default_domain(&domain)) != 0) {
      que (LEVEL1, probfmt, "no nis-domainname", 
	   yperr_string(r));
	return (NOTOK);	  /* cannot get domainname              */
    }

  r = yp_order(domain, tb->tb_file, &order);
  if(r) {
    que(LEVEL1, probfmt, "no such map", tb->tb_file);
    return(NOTOK); /* cannot get map */
  }
  r = yp_master(domain, tb->tb_file, &master);
  if(r) {
    que(LEVEL4, subhdrfmt, "      NIS domain", domain);
    que(LEVEL1, probfmt, "", "no master-server");
    return(NOTOK); /* cannot get master-server */
  }
  que(LEVEL4, subhdrfmt, "      NIS domain", domain);
  que(LEVEL4, subhdrfmt, "      master Server", master);
  que(LEVEL4, "    %-20s: %s has order %d, %s", "", tb->tb_file, order,
      ctime((time_t *)&order));
  /*qflush (LEVEL4);*/

  return(OK);

/*  que(level, subhdrfmt, title, "(via NIS)");
  return 0;
*/
}
#endif /* HAVE_NIS */
