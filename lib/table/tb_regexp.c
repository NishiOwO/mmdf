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
#include <sys/stat.h>

#ifdef HAVE_REGEXEC
#include <regex.h>

extern LLog *logptr, chanlog;
extern int tb_regexp_tai();
extern int tb_regexp_check();
extern int tb_regexp_fetch();

extern int Tdebug;
#define  logx    if (Tdebug) printf

typedef struct {
  int cflags;      /* regcomp() compile-flags */
  int eflags;      /* regexec() execution-flags */
  char addsuffix;  /* add '$' at the end of regular entry */
  char addprefix;  /* add '^' at the beginning of regular entry */
} parmdef;

int tb_regexp_init(tblptr)
Table *tblptr;
{
  parmdef *param;
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_regexp_init (%p)", tblptr);
#endif
  logx( "tb_regexp_init (%p, %s)\n", tblptr, tblptr -> tb_name);

  tblptr->tb_parameters = param =
    (parmdef *)malloc(sizeof(parmdef));

  memset(tblptr -> tb_parameters, 0, sizeof(parmdef));
//  param->cflags = REG_ICASE|REG_NEWLINE;
//  param->eflags = 0;
         
  tblptr -> tb_tai   = &tb_regexp_tai;
  /*tblptr -> tb_k2val = NULL;*/
  /*tblptr -> tb_print = &tb_regexp_print;*/
  tblptr -> tb_check = &tb_regexp_check;
  tblptr -> tb_fetch = &tb_regexp_fetch;
  
  return;
}

#define CMDTNOOP    0
#define CMDCFLAG    1
#define CMDEFLAG    2
#define CMDTPRE      3
#define CMDTSUFF     4

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",          CMDTNOOP,   0},
    {"addprefix", CMDTPRE,    1},
    {"addsuffix", CMDTSUFF,   1},
    {"cflag",     CMDCFLAG,   1},
    {"eflag",     CMDEFLAG,   1},
    {0,           0,          0}
};
#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

#define CMDCNONE    0
#define CMDCEXT     1
#define CMDCICA     2
#define CMDCSUB     3
#define CMDCLINE    4

LOCVAR Cmd cmap[] = {
    {"extended", CMDCEXT,    0},
    {"icase",    CMDCICA,    0},
    {"newline",  CMDCLINE,   0},
    {"none",     CMDCNONE,   0},
    {"nosub",    CMDCSUB,    0},
    {0,          0,          0}
};
#define CMAPENT ((sizeof(cmap)/sizeof(Cmd))-1)

#define CMDENONE    0
#define CMDEBOL     1
#define CMDEEOL     2

LOCVAR Cmd emap[] = {
    {"none",     CMDENONE,   0},
    {"notbol",   CMDEBOL,    0},
    {"noteol",   CMDEEOL,    0},
    {0,          0,          0}
};
#define EMAPENT ((sizeof(emap)/sizeof(Cmd))-1)
    

int tb_regexp_tai(tbptr, gind, argc, argv)
    Table *tbptr;
    int *gind;
    int argc;
    char *argv[];
{
  int ind = *gind - 1;
  parmdef *param = (parmdef *)tbptr->tb_parameters;
  
  
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb_regexp_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTNOOP:
        break;

      case CMDTPRE:
        param->addprefix =  atoi (argv[ind+1]);
        break;

      case CMDTSUFF:
        param->addsuffix =  atoi (argv[ind+1]);
        break;

      case CMDCFLAG:
		    switch (cmdbsrch (argv[ind+1], 0, cmap, CMAPENT)) {
                case CMDCNONE:
                  param->cflags = 0;
                  ll_log (logptr, LLOGFTR, "empty"); break;
                case CMDCEXT:
                  param->cflags |= REG_EXTENDED;
                  ll_log (logptr, LLOGFTR, "extended"); break;
                case CMDCICA:
                  param->cflags |= REG_ICASE;
                  ll_log (logptr, LLOGFTR, "icase"); break;
                case CMDCSUB:
                  param->cflags |= REG_NOSUB;
                  ll_log (logptr, LLOGFTR, "nosub"); break;
                case CMDCLINE:
                  param->cflags |= REG_NEWLINE;
                  ll_log (logptr, LLOGFTR, "newline"); break;
                default:
                  tai_error ("unknown table parmvalue", argv[ind+1],
                             argc, argv);
                  break;
            }
        break;

      case CMDEFLAG:
		    switch (cmdbsrch (argv[ind+1], 0, emap, EMAPENT)) {
                case CMDEBOL:
                  param->eflags = REG_NOTBOL;
                  ll_log (logptr, LLOGFTR, "nobol"); break;
                case CMDEEOL:
                  param->eflags = REG_NOTEOL;
                  ll_log (logptr, LLOGFTR, "noeol"); break;
                default:
                  tai_error ("unknown table parmvalue", argv[ind+1],
                             argc, argv);
                  break;
            }
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
  char         *regex, buffer[LINESIZE+1];
  parmdef *param = (parmdef *)table->tb_parameters ;
  int rc;
  int offset;
  int regresult;
  
#if 0
  cflag=REG_EXTENDED REG_ICASE REG_NOSUB REG_NEWLINE;
  eflag=REG_NOTBOL REG_NOTEOL;
  cflag=REG_NEWLINE;
  eflag=REG_NOTEOL;
#else
/*  eflag=REG_NOTEOL;*/
#endif

  if (!tb_open (table, first))
	return (NOTOK);		  /* not opened                         */

#ifdef DEBUG
  ll_log (logptr, LLOGFTR, "tb_regexp_fetch() pre=%d, suf=%d, c=%x, e=%x",
          param->addprefix, param->addsuffix,
          param->cflags, param->eflags);
  ll_log (logptr, LLOGFTR, "tb_regexp_fetch() %s %s %s %s %s %s\n",
        param->cflags & REG_EXTENDED ? "ext" : "!ext",
        param->cflags & REG_ICASE ? "icase" : "!icase",
        param->cflags & REG_NOSUB ? "nosub" : "!nosub",
        param->cflags & REG_NEWLINE ? "nl" : "!nl",
        param->eflags == REG_NOTBOL ? "bol" : "!bol",
        param->eflags == REG_NOTEOL ? "eol" : "!eol");
#endif /* DEBUG */
  if(param->addprefix) { offset=1; buffer[0]='^'; }
  else offset=0;
  
  while (tb_read (table, buffer+offset, buf))
  {				  /* cycle through keys                 */
    if(param->addsuffix) {
      if(buffer[strlen(buffer)-1]!='$') strcat(buffer, "$");
    }
    if(buffer[offset] == '!') {
      regresult = REG_NOMATCH;
      if(offset) buffer[offset]==buffer[0];
      regex = buffer+1;
    } else {
      regresult = 0;
      regex = buffer;
    }
    
    if(Tdebug>1) printf("tb_regexp_fetch(): %d '%s'\n", regresult, regex);
    if(regcomp(&preg, regex, param->cflags))
      return (NOTOK);
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tb_regexp_fetch(): '%s' to '%s' ",
            name, regex);
#endif
    if(regexec(&preg, name, nmatch, pmatch, param->eflags)==regresult)
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


int tb_regexp_check(mmdfuid, mmdfgid, MMDFlogin, tb, hdrfmt, title)
int mmdfuid, mmdfgid;
char *MMDFlogin;
Table *tb;
char *hdrfmt;
char *title;
{
  struct stat statbuf;
  char probfmt[] = "%-18s: %s\n";

 que(LEVEL6, hdrfmt, title, "(via regular expression)");
 que (LEVEL6, hdrfmt, title, tb -> tb_file);

 chkfile (tb -> tb_file, 0644, 0664, mmdfuid, mmdfgid, MMDFlogin);
 return 0;
}
#endif /* HAVE_REGEXEC */
