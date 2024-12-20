/*
 *     MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *
 *     Copyright (C) 1979,1980,1981  University of Delaware
 *
 *     Department of Electrical Engineering
 *     University of Delaware
 *     Newark, Delaware  19711
 *
 *     Phone:  (302) 738-1163
 *
 *
 *     This program module was developed as part of the University
 *     of Delaware's Multi-Channel Memo Distribution Facility (MMDF).
 *
 *     Acquisition, use, and distribution of this module and its listings
 *     are subject restricted to the terms of a license agreement.
 *     Documents describing systems using this module must cite its source.
 *
 *     The above statements must be retained with all copies of this
 *     program and may not be removed without the consent of the
 *     University of Delaware.
 *
 *
 *     version  -1    David H. Crocker    March   1979
 *     version   0    David H. Crocker    April   1980
 *     version  v7    David H. Crocker    May     1981
 *     version   1    David H. Crocker    October 1981
 *
 */

/*  READ "NETWORK" HOST NAME TABLES
 *
 *  These routines use the dbm(3x) procedures of Unix Version 7.
 *  All accesses are via the fetch call of the library. Linkage between
 *  elements of the database are contained within the procedures instead
 *  of the database itself due to the nature of the dbm design.
 *
 *  May 81  Jim Lieb, SRI   rewrite by to use dbm library
 *  Jun 82  D. Crocker      minor cleanup & installation
 *                          force key value to be lower case
 *                          partition sequence #s by channel, not global
 *  Dec 85  P.Cockcroft UCL add nameserver support and add bullet protection
 */
#include "util.h"
#include "mmdf.h"
#include "ch.h"                   /* has table state strcture def       */
#include "dm.h"
#include "chdbm.h"
#ifdef HAVE_LIBGDBM
#  include <gdbm.h>
#endif

#if defined(HAVE_LIBGDBM) || defined(HAVE_LIBNDBM) || defined(HAVE_LIBDBM)
#  define HAVE_DBM
#endif

extern LLog *logptr;
extern char *tbldfldir;
extern char *tbldbm;
extern Chan **ch_tbsrch;
extern char *ch_dflnam;

#ifndef HAVE_LIBGDBM
typedef struct {char *dptr; int dsize;} datum;
#endif
int Tdebug = 0;
#define  logx    if (Tdebug) printf

extern datum myfetch();

/* *******************  FIND HOST NAME IN ANY TABLE  ****************** */

Chan   *
	ch_h2chan (hostr, pos) /* which chan name table is host in?  */
char   *hostr;                   /* name of host                       */
int     pos;                    /* which position to get  (0 = first) */
{
    register	Chan	*chanptr;
    register	struct DBvalues *dp;
    register	char	*cp;
    DBMValues   dbm;
    Chan	**chp;
    char        hostname[ADDRSIZE];
    int         tb_done[TBT_LAST];
    int         tb_type;
    
#ifdef	HAVE_NAMESERVER
    int         ns_done = 0;
#endif /* HAVE_NAMESERVER */
    int         dbm_done = 0;
    memset(tb_done, 0, sizeof(tb_done));
    
/*  the list of channel name tables is first searched.  If a hit is found
 *  in the table for the local channel, OK is returned to indicate a local
 *  reference.
 */

#ifdef DEBUG
    ll_log( logptr, LLOGFTR, "h2chan ('%s', %d)", hostr, pos);
#endif
    for (chp = ch_tbsrch; (chanptr = *chp) != (Chan *)0; chp++)
    {
#ifdef DEBUG
      ll_log( logptr, LLOGFTR, "h2chan table '%s'",
              chanptr->ch_table->tb_name);
#endif
      
      tb_type = chanptr -> ch_table -> tb_type;
      switch(tb_type) {
          default:
            if ((!tb_done[tb_type]) &&
                (chanptr->ch_table->tb_fetch!=NULL) ){
              /*
               * assumes the fact we are looking for a 'channel'
               * if this is not the case you must do the lookup every
               * time round the loop
               * This code believes all NS channel tables are equivalent,
               * and they will return the same answer.  Not unreasonable
               * at the current time, but watch out.
               */
              switch(chanptr->ch_table->tb_fetch (chanptr->ch_table,
                                                  hostr, hostname, 1)){
                  case OK:
                    tb_done[tb_type]++;
                    break;
                  case MAYBE:
                    return( (Chan *)MAYBE);
              }
              tb_done[tb_type]++;
            }
            if (tb_done[tb_type] == 1)
              continue;
            if(--pos > 0)
              continue;
#if DEBUG > 1
            ll_log( logptr, LLOGFTR, "NSconsider ('%s', '%s')",
                    hostname, chanptr->ch_lname);
#endif
            if (lexequ (chanptr -> ch_name, ch_dflnam))
              return( (Chan *)OK );   /* local ref             */
            return( chanptr );
            break;

          case TBT_FILE:
            if (tb_k2val (chanptr -> ch_table, TRUE, hostr, hostname) == OK)
            {                         /* the hostname IS in this table      */
              if (lexequ (chanptr -> ch_name, ch_dflnam))
              {
                return ((Chan *) OK); /* local reference                */
              }
              return (chanptr);    /* let caller know which chan         */
            }
            break;
            
          case TBT_DBM:
#ifdef HAVE_DBM
            if (!tb_done[tb_type]) {
              tb_done[tb_type]++;
              if (tb_fetch (hostr, dbm))
                tb_done[tb_type]++;
            }
            if (tb_done[tb_type] == 1)
              continue;
            else
            for (dp = dbm ; (cp = dp->RECname) != NULL ; dp++) {
#if DEBUG > 1
              ll_log( logptr, LLOGFTR, "consider ('%s')", cp);
#endif
              if (lexequ(cp, chanptr -> ch_table -> tb_name)) {
                if (--pos > 0)
                  break;
                if (lexequ(chanptr -> ch_name, ch_dflnam))
                  return( (Chan *)OK );
                return(chanptr);
              }
            }
#endif /* HAVE_DBM */
            break;
      }
    }
    
    return ((Chan *) NOTOK);
}

/* ***********  GIVEN Subdomain, FIND Domain  ***************** */

Domain *
	dm_s2dom (subdomain, official, dmbuf)
char    *subdomain;             /* name of subdomain to look up              */
char    *official;              /* Where to put official name */
char    *dmbuf;                 /* Domain route buffer */
{
    register	Domain	*dmnptr;
    register	char	*cp;
    register	struct DBvalues *dp;
    DBMValues   dbm;
    extern	Domain	**dm_list;
    Domain	**dmp;
    char	*argv[DM_NFIELD];
    char        sdbuf[LINESIZE];
    int         tb_done[TBT_LAST];
    int         tb_type;

#ifdef DEBUG
    ll_log( logptr, LLOGFTR, "dm_s2dom ('%s')", subdomain);
#endif
    memset(tb_done, 0, sizeof(tb_done));

    for (dmp = dm_list; (dmnptr = *dmp) != (Domain *)0; dmp++)
    {
      /* If flags=partial is not set or this is a top domain table    */
      /* (indicating that we've already tried this match as a "route" */
      /* match), skip it.                                             */
      tb_type = dmnptr -> dm_table -> tb_type;
      memset(sdbuf, 0, sizeof(sdbuf));

      if (((dmnptr -> dm_table -> tb_flags & TB_PARTIAL) != TB_PARTIAL) ||
          (!isstr(dmnptr->dm_domain)))
        snprintf(sdbuf, sizeof(sdbuf), "%s", subdomain);
      else
        snprintf(sdbuf, sizeof(sdbuf), "%s.%s", subdomain, dmnptr->dm_domain);
      
      switch(dmnptr -> dm_table -> tb_type) {
          default:
            if( dmnptr->dm_table->tb_fetch!=NULL ) {
              switch (dmnptr->dm_table->tb_fetch(dmnptr->dm_table,
                                                 sdbuf, official, 1)) {
                  case OK:
                    /* route is simply official name */
                    (void) strcpy(dmbuf,official);
                    return( dmnptr );
                  case MAYBE:
                    break;
                    return( (Domain *)MAYBE);
              }
            }
            break;
            
          case TBT_FILE:
            if (tb_k2val (dmnptr -> dm_table, TRUE, subdomain, sdbuf) == OK) {
              (void) strcpy(dmbuf, sdbuf);
              str2arg(sdbuf, DM_NFIELD, argv, 0);
              (void) strcpy(official, argv[0]);
              return (dmnptr);
            }
            break;
            
          case TBT_DBM:
#ifdef HAVE_DBM
            if (!tb_done[tb_type]) {
              tb_done[tb_type]++;
              if (tb_fetch (subdomain, dbm))
                tb_done[tb_type]++;
            }
            if (tb_done[tb_type] == 1)
              continue;
            for ( dp = dbm ; (cp = dp->RECname) != NULL ; dp++) {
#ifdef DEBUG
              ll_log( logptr, LLOGFTR, "dm_s2dom: consider ('%s')", cp);
#endif
              if (!lexequ(cp, dmnptr -> dm_table -> tb_name))
                continue;
              if(dp->RECval == NULL) {	/* should never happen */
                *dmbuf = *official = '\0';
                return(dmnptr);
              }
              (void) strcpy (dmbuf, dp->RECval);
              str2arg(dp->RECval, DM_NFIELD, argv, 0);
              (void) strcpy(official, argv[0]);
              return(dmnptr);
            }
#endif /* HAVE_DBM */
            break;
	}
    }
    
    (void) strcpy (official, subdomain);
    return ((Domain *) NOTOK);
}

/* *******  Get a record from the database, given a key    ********** */
/*            Note that the strings are in internal statics             */

#ifdef HAVE_DBM
tb_fetch (name, dbm)                /* return filled entry for name */
char *name;                         /* use this key to fetch entry  */
DBMValues dbm;                      /* put the entry here           */
{
    LOCVAR int dbmopen = FALSE;
    LOCVAR char dbvalue[256];
    datum key, value;
    char *lastchar;
    register char *p, *cp;
    register struct DBvalues *dp;
    register int cnt;

    if (!dbmopen)
    {
	extern int mydbminit();
	char filename[128];

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "tb_fetch: dbminit");
#endif
	getfpath (tbldbm, tbldfldir, filename);
	if (mydbminit (filename, "r") < 0) {
#ifdef HAVE_LIBGDBM
      ll_log (logptr, LLOGBTR, "tb_fetch: gdbm_open failed: [%d] %s",
              gdbm_errno, gdbm_strerror(gdbm_errno));
#endif
      err_abrt (LLOGTMP, "Error opening database '%s'", filename);
    }
    
	dbmopen = TRUE;
    }

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "fetch (%s)", name);
#endif
    key.dptr = name;
    key.dsize = strlen (name) + 1;
    for (p = name; *p; p++)
	*p = uptolow (*p);

    value = myfetch (key);
    if (value.dptr == NULL)
    {
#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "fetch of '%s' failed", name);
#endif
	return (FALSE);
    }
    for (cp = value.dptr, p= dbvalue, lastchar = &p[value.dsize];
		p < lastchar; *p++ = *cp++) ;
    *p = '\0';                  /* copy value to separate storage */

    for (p = dbvalue , dp = dbm, cnt = 0; *p && cnt < MAXVAL ; cnt++, dp++)
    {
	for(dp->RECname = p ; *p ; p++)
	    if (*p == ' ') {      /* get table name */
		*p++ = '\0';
		break;
	    }
	for (dp->RECval = (*p ? p : NULL); *p; p++)
	    if (*p == FS) {      /* get value-part */
		*p++ = '\0';
		break;
	    }
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "fetch val(%d)='%s/%s'",
		cnt, dp->RECname, (dp->RECval==NULL) ? "<NULL>": dp->RECval);
#endif
    }
    for(;cnt <= MAXVAL; cnt++, dp++)    /* zero the rest of the structures */
	dp->RECname = dp->RECval = NULL;

    return (TRUE);
}
#endif /* HAVE_DBM */

#ifdef HAVE_WILDCARD
int tb_wk2val(table, first, name, buf)
register Table  *table;
int     first;                    /* start at beginning of list?        */
register char  name[];            /* name of ch "member" / key          */
char   *buf;                      /* put value int this buffer          */
{
  int retval;
  Dmn_route tmpdmn;           /* hold parsed domain string            */
  char *q, *p;
  int sublen;                 /* length of right-hand to look up      */
  int ind; 

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "tb_wk2val (%s, first=%d, %s)",
			table -> tb_name, first, name);
#endif
  strcpy (tmpdmn.dm_buf, name);
  tmpdmn.dm_argc = cstr2arg (tmpdmn.dm_buf, DM_NFIELD, tmpdmn.dm_argv, '.');
#ifdef DEBUG
  ll_log(logptr, LLOGBTR, "tb_wk2val: dm_argc=%d", tmpdmn.dm_argc);
#endif
  for (ind = sublen = 0; ind < tmpdmn.dm_argc; ind++) {
#ifdef DEBUG
    ll_log(logptr, LLOGBTR, "tb_wk2val: %s", &name[sublen]);
#endif
    retval = tb_k2val(table, first, &name[sublen], buf);
    if(retval == OK) return(retval);
    sublen += strlen (tmpdmn.dm_argv[ind]);
    if (ind != tmpdmn.dm_argc && ind>0) sublen++;
  }
  return(retval);
}
#endif /* HAVE_WILDCARD */

/* *******  FIND VALUE (address), GIVEN ITS KEY (hostname)  ********* */


tb_k2val (table, first, name, buf) /* use key and return value */
register Table  *table;
int     first;                    /* start at beginning of list?        */
register char  name[];            /* name of ch "member" / key          */
char   *buf;                      /* put value int this buffer          */
{
LOCVAR DBMValues dbm;
LOCVAR struct DBvalues *dp = (struct DBvalues *) 0;
    register char *cp;
    int retval;
    char    host[LINESIZE];

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "tb_k2val (%s, first=%d, %s)",
			table -> tb_name, first, name);
#endif

    switch(table->tb_type) {
        default:
          if(table -> tb_fetch != NULL) {
            if ((retval = table -> tb_fetch (table, name, buf, first))
                != NOTOK)
              return (retval);
          }
#ifdef DEBUG
          ll_log (logptr, LLOGFTR, "tb_k2val failed");
#endif
          (void) strcpy (buf, "(ERROR)");
          return (NOTOK);
          break;
          
        case TBT_FILE:
          if (!tb_open (table, first))
            return (NOTOK);           /* not opened                         */

          memset(host, 0, sizeof(host));
          while (tb_read (table, host, buf))
          {                             /* cycle through keys                 */
            if (lexequ (name, host))  /* does key match search name?        */
            {
              compress (buf, buf);  /* get rid of extra white space       */
              return (OK);
            }
            memset(host, 0, sizeof(host));
          }

          (void) strcpy (buf, "(ERROR)");
          return (NOTOK);
          break;

        case TBT_DBM:
          break;
    }
    
#ifdef HAVE_DBM
    if (!first) {
      if(dp!=NULL) dp++;
    }
    else
    {
	if (tb_fetch (name, dbm))
	{
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "hit");
#endif
	    dp = dbm;
	}
	else
	    dp = (struct DBvalues *) 0;  /* none */
    }

    if (dp)
	for (; (cp = dp->RECname) != NULL; dp++)
	{
	    if (strequ (cp, table -> tb_name))
	    {                       /* this is the right channel        */
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "table(%s)", cp);
#endif
		if(dp->RECval == NULL)
		    *buf = '\0';
		else
		    (void) strcpy (buf, dp->RECval);
		return (OK);      /* give them the value-part         */
	    }
	}
#endif /* HAVE_DBM */
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "tb_k2val failed");
#endif
    (void) strcpy (buf, "(ERROR)");
    return (NOTOK);
}
