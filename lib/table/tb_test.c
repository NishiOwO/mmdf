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
#include "ch.h"
#include "cmd.h"

extern LLog *logptr, chanlog;
extern int tb_test_tai();
extern int tb_test_fetch();

int tb_test_init(tblptr)
Table *tblptr;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_test_init (%p)", tblptr);
#endif
  printf( "tb_test_init (%p, %s)\n", tblptr, tblptr -> tb_name);

  tblptr -> tb_tai   = &tb_test_tai;
  /*tblptr -> tb_k2val = NULL;*/
  /*tblptr -> tb_print = &tb_test_print;*/
  tblptr -> tb_check = &tb_test_check;
  /*tblptr -> tb_fetch = &tb_test_fetch;*/
  
  return;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#define CMDTNOOP    0
#define CMDTOPT1    1
#define CMDTOPT2    2
#define CMDTOPT3    3

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",         CMDTNOOP,   0},
    {"option1",  CMDTOPT1,   1},
    {"option2",  CMDTOPT2,   1},
    {"option3",	 CMDTOPT3,   1},
    {0,          0,          0}
};

#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

int tb_test_tai(tbptr, gind, argc, argv)
    Table *tbptr;
    int *gind;
    int argc;
    char *argv[];
{
  int ind = *gind - 1;
  struct tb_ns_param *param;
/*  tbptr->tb_parameters = param =
    (struct tb_ns_param *)malloc(sizeof(struct tb_ns_param));*/
  
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb_test_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTOPT1:
      case CMDTOPT2:
      case CMDTOPT3:
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
int tb_test_fetch (table, name, buf, first) /* use key and return value */
register Table  *table;           /* I: */
register char  name[];            /* I: name of ch "member" / key          */
char   *buf;                      /* O: put value int this buffer          */
int     first;                    /* I: start at beginning of list?        */
{
  int retval = NOTOK;

/*  return (OK);*/
  
  return (retval);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int tb_test_check(mmdfuid, mmdfgid, MMDFlogin, tb, hdrfmt, title)
int mmdfuid, mmdfgid;
char *MMDFlogin;
Table *tb;
char *hdrfmt;
char *title;
{
  que(LEVEL6, hdrfmt, title, "(test table / example)");
  return 0;
}
