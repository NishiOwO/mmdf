/*
 *      TB_LDAP.C
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

#ifdef HAVE_LDAP
#include <lber.h>
#include <ldap.h>

extern LLog *logptr, chanlog;
extern int tb_ldap_tai();
extern int tb_ldap_fetch();
extern int tb_ldap_check();

extern int Tdebug;
#define  logx    if (Tdebug) printf

typedef struct {
  LDAP *conn;

  char *server_host;
  int  server_port;
  int  scope;
  char *search_base;
  char *query_filter;
  char *result_attribute;
  int  timeout;
} parmdef;


int tb_ldap_init(tblptr)
Table *tblptr;
{
  parmdef *param;
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "tb_ldap_init (%p)", tblptr);
#endif
  logx( "tb_ldap_init (%p, %s)\n", tblptr, tblptr -> tb_name);

  tblptr->tb_parameters = param = (parmdef *)malloc(sizeof(parmdef));
  
  /* init parameters */
  memset(tblptr -> tb_parameters, 0, sizeof(parmdef));
  param->server_port = LDAP_PORT;
  param->server_host = "localhost";
  param->result_attribute = "maildrop";
  param->query_filter     = "uid=%s";
  param->scope   = LDAP_SCOPE_SUBTREE;
  param->timeout = 5;
  
  tblptr -> tb_tai   = &tb_ldap_tai;
  /*tblptr -> tb_k2val = NULL;*/
  /*tblptr -> tb_print = &tb_ldap_print;*/
  tblptr -> tb_check = &tb_ldap_check;
  tblptr -> tb_fetch = &tb_ldap_fetch;
  
  return;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#define CMDTNOOP    0
#define CMDTHOST    1
#define CMDTPORT    2
#define CMDTBASE    3
#define CMDTTOUT    4
#define CMDTFILTER  5
#define CMDTSCOPE   6
#define CMDTATTRIB  7

LOCVAR Cmd
	    cmdtbl[] =
{
    {"",            CMDTNOOP,   0},
    {"attrib",	    CMDTATTRIB, 1},
    {"filter",      CMDTFILTER, 1},
    {"scope",	    CMDTSCOPE,  1},
    {"search_base", CMDTBASE,   1},
    {"server_host", CMDTHOST,   1},
    {"server_port", CMDTPORT,   1},
    {"timeout",     CMDTTOUT,   1},
    {0,             0,          0}
};

#define CMDTBENT ((sizeof(cmdtbl)/sizeof(Cmd))-1)

int tb_ldap_tai(tbptr, gind, argc, argv)
    Table *tbptr;
    int *gind;
    int argc;
    char *argv[];
{
  int ind = *gind - 1;
  parmdef *param = (parmdef *)tbptr->tb_parameters;

#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "tb_ldap_tai '%s'(%d)",
                argv[ind], argc - ind);
#endif
  
  switch (cmdbsrch (argv[ind], argc - ind, cmdtbl, CMDTBENT))
  {
      case CMDTBASE: param->search_base = argv[ind+1]; break;
      case CMDTHOST: param->server_host = argv[ind+1]; break;
      case CMDTPORT: param->server_port = atoi (argv[ind+1]); break;

      case CMDTATTRIB: param->result_attribute = argv[ind+1]; break;
      case CMDTFILTER: param->query_filter     = argv[ind+1]; break;
      case CMDTTOUT:  param->timeout = atoi (argv[ind+1]); break;
        
      case CMDTSCOPE:
        if(strcmp(argv[ind+1], "base")==0)
          param->scope = LDAP_SCOPE_BASE;
        else if(strcmp(argv[ind+1], "one")==0)
          param->scope = LDAP_SCOPE_ONELEVEL;
        else if(strcmp(argv[ind+1], "sub")==0)
          param->scope = LDAP_SCOPE_SUBTREE;
        else {
          logx("unknown scope parm '%s', %d\n", argv[ind+1], argc, argv);
          tai_error ("unknown scope parm", argv[ind+1], argc, argv);
        }
        break;
      default:
        logx("unknown table parm '%s', %d\n", argv[ind], argc, argv);
        tai_error ("unknown table parm", argv[ind], argc, argv);
        break;
  }

  return 0;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int tb_ldap_fetch (table, name, buf, first) /* use key and return value */
register Table  *table;           /* I: */
register char  name[];            /* I: name of ch "member" / key          */
char   *buf;                      /* O: put value int this buffer          */
int     first;                    /* I: start at beginning of list?        */
{
  int retval = NOTOK;
  parmdef *param = (parmdef *)table->tb_parameters;
  char filter[1024];
  int rc;
  struct timeval tv;
  LDAPMessage *res = 0;
  LDAPMessage *entry = 0;
  char *result_attributes[1];
  char **attr_values;
  static int pos=0;
  
  logx( "tb_ldap_fetch (%p, %s, %p, %d)\n", table, table -> tb_name,
        buf, first);
  ll_log(logptr, LLOGFTR, "tb_ldap_fetch (%p, %s, %p, %d)", table,
         table -> tb_name, buf, first);

  if(first) pos=0;
  tv.tv_sec = param->timeout;
  tv.tv_usec = 0;

  if(param->conn == NULL) {
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tb_ldap_fetch is NULL");
    logx ("tb_ldap_fetch is NULL\n");
#endif
    param->conn = ldap_open(param->server_host, param->server_port);
    if(param->conn == NULL) {
      pos=0;
      logx ("tb_ldap_fetch still NULL\n");
      ll_log (logptr, LLOGFTR, "tb_ldap_fetch still NULL");
      return retval;
    } else {
      logx ("tb_ldap_fetch now not NULL\n");
      ll_log (logptr, LLOGFTR, "tb_ldap_fetch now not OPEN");
    }
  }
  memset(filter, 0, sizeof(filter));
  snprintf(filter, sizeof(filter), param->query_filter, name);
  logx("base='%s', filter='%s'\n", param->search_base, filter);
  ll_log(logptr, LLOGFTR, "base='%s', filter='%s'\n",
         param->search_base, filter);

  result_attributes[0] = param->result_attribute;

  if(ldap_search_st(param->conn, param->search_base, param->scope,
                   filter, result_attributes, 0, &tv, &res)
     != LDAP_SUCCESS) return retval;
  
  logx("ldap_search: %d\n", ldap_count_entries(param->conn, res));
  ll_log(logptr, LLOGFTR, "ldap_search: %d\n",
         ldap_count_entries(param->conn, res));

  for(entry = ldap_first_entry(param->conn, res); entry != NULL;
      entry = ldap_next_entry(param->conn, entry)) {

    attr_values = ldap_get_values(param->conn, entry,
                                  param->result_attribute);
    if(attr_values == NULL) {
      logx("attr_values is NULL\n");
      continue;
    }
    
//    for(rc=0; attr_values[rc] != NULL; rc++) {
//      logx("%d: '%s'\n", rc, attr_values[rc]);
//    }
    if(attr_values[pos]!=NULL) {
      strcpy(buf, attr_values[pos++]);
      compress (buf, buf);  /* get rid of extra white space       */
      retval=OK;
      //if(attr_values[pos]==NULL) pos=0;
    } else pos=0;
    
    ldap_value_free(attr_values);
  }
  
  return (retval);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
int tb_ldap_check(mmdfuid, mmdfgid, MMDFlogin, tb, hdrfmt, title)
int mmdfuid, mmdfgid;
char *MMDFlogin;
Table *tb;
char *hdrfmt;
char *title;
{
  parmdef *param = (parmdef *)tb->tb_parameters;

  que(LEVEL6, hdrfmt, title, "(ldap table)");
  return 0;
}
#endif /* HAVE_LDAP */
