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

#ifdef HAVE_REGEXEC
#include <regex.h>

extern LLog *logptr, chanlog;
extern int tb_regexp_tai();
extern int tb_regexp_fetch();

extern int Tdebug;
#define  logx    if (Tdebug) printf

int tb_regexp_init(tblptr)
Table *tblptr;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_regexp_init (%p)", tblptr);
#endif
  logx( "tb_regexp_init (%p, %s)\n", tblptr, tblptr -> tb_name);

  tblptr -> tb_tai   = &tb_regexp_tai;
  /*tblptr -> tb_k2val = NULL;*/
  /*tblptr -> tb_print = &tb_regexp_print;*/
  /*tblptr -> tb_check = &tb_regexp_check;*/
  tblptr -> tb_fetch = &tb_regexp_fetch;
  
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

int tb_regexp_tai(tbptr, gind, argc, argv)
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
	    ll_log (logptr, LLOGFTR, "tb_regexp_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTNOOP:
        break;
        
      default:
        tai_error ("unknown table parm", argv[ind], argc, argv);
        break;
  }

  return 0;
}

int tb_regexp_fetch (table, name, buf, first) /* use key and return value */
register Table  *table;           /* I: */
register char  name[];            /* I: name of ch "member" / key          */
char   *buf;                      /* O: put value int this buffer          */
int     first;                    /* I: start at beginning of list?        */
{
  regex_t      preg;
  regmatch_t   pmatch[1];
  size_t       nmatch = 0;
  char         regex[LINESIZE];
  int rc;
  int cflag = 0, eflag = 0;

#if 0
  cflag=REG_EXTENDED REG_ICASE REG_NOSUB REG_NEWLINE;
  eflag=REG_NOTBOL REG_NOTEOL;
  cflag=REG_NEWLINE;
  eflag=REG_NOTEOL;
#else
  cflag=REG_ICASE|REG_NEWLINE;
/*  eflag=REG_NOTEOL;*/
#endif

  if (!tb_open (table, first))
	return (NOTOK);		  /* not opened                         */
  
  while (tb_read (table, regex, buf))
  {				  /* cycle through keys                 */
    if(regex[strlen(regex)-1]!='$')
       strcat(regex, "$");
    if(Tdebug>1) printf("tb_regexp_fetch(): '%s'\n", regex);
    if(regcomp(&preg, regex, cflag))
      return (NOTOK);
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tb_regexp_fetch(): '%s' to '%s' ",
            name, regex);
#endif
    if(regexec(&preg, name, nmatch, pmatch, eflag)==0)
      /* does key match search name?        */
	{
      compress (buf, buf);  /* get rid of extra white space       */
      regfree(&preg);
      return (OK);
	}
    regfree(&preg);
  }

  (void) strcpy (buf, "(ERROR)");
  return (NOTOK);
}
#endif /* HAVE_REGEXEC */
