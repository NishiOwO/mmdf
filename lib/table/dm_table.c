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

#include "util.h"
#include "mmdf.h"
#include "ch.h"                   /* has table state strcture def       */
#include "dm.h"
#include "chdbm.h"

extern Domain **dm_list;
extern Domain *dm_s2dom();
extern struct ll_struct   *logptr;

/*
 * when mangling addresses, should we try the reverse before or after
 * the RFC822 direction
 * is set by submit otherwise is as below.
 */

typedef struct {char *dptr; int dsize;} datum;


/*
 * Steve Kille   jan 84   Redo this module
 *
 * G. Brendan Reilly May 84     take out dependencies on DBM package
 *
 *
 */


/* *******  FIND VALUE (address), GIVEN ITS KEY (hostname)  ********* */


LOCFUN
dm_k2val (dmntbl, subdmn, domain, dmnroute)
				   /* sub-domain spec -> routing host   */
register Domain  *dmntbl;         /* domain table to look in            */
register char  *subdmn;          /* name of sub-domain to look for     */
char *domain;                   /* where to stuff full domain name      */
Dmn_route *dmnroute;              /* where to put routing information   */
{
    char *p;
    int retval;
    char sdbuf[LINESIZE];
    

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_k2val (%s, %s)", dmntbl -> dm_name, subdmn);
#endif

    if (((dmntbl->dm_table->tb_type) == TBT_NS) &&
	isstr(dmntbl->dm_domain)) {
	snprintf(sdbuf, sizeof(sdbuf), "%s.%s", subdmn, dmntbl->dm_domain);
	retval = tb_k2val (dmntbl->dm_table, TRUE, sdbuf, dmnroute->dm_buf);
    }  
    else
#ifdef HAVE_WILDCARD
	retval = tb_wk2val (dmntbl->dm_table, TRUE, subdmn, dmnroute->dm_buf);
#else /* HAVE_WILDCARD */
	retval = tb_k2val (dmntbl->dm_table, TRUE, subdmn, dmnroute->dm_buf);
#endif /* HAVE_WILDCARD */
    
    switch(retval) {
    case MAYBE:
	return(MAYBE);
    case OK:
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "dm_k2val: hit with '%s'", dmnroute->dm_buf);
#endif
	dmnroute -> dm_argc = str2arg (dmnroute -> dm_buf,
			DM_NFIELD, dmnroute -> dm_argv, (char *)0);
	(void) strcpy(domain, dmnroute -> dm_argv[0]);
	return (OK);
    }

    /* Not found yet--give up if we're not allowed to look for subdomains
       to route via */
    if ((dmntbl->dm_table->tb_flags & TB_ROUTE) != TB_ROUTE)
	return(NOTOK);

    /* Here we're going to look for successive subdomains (b.c.d, c.d, d)
       on the chance that we can route via that subdomain to reach a.b.c.d.
       For example, given oxnard.bitnet, we'll look for a table entry such
       as "bitnet:cunyvm.cuny.edu" where cunyvm.cuny.edu is the route to
       *.bitnet */

    p = subdmn;
    while ((p = strchr (p, '.')) != (char *)0)
    {
	char tdmnbuf[LINESIZE];

	p++;
	if (((dmntbl->dm_table->tb_type) == TBT_NS) &&
	    isstr(dmntbl->dm_domain)) {
	    snprintf(sdbuf, sizeof(sdbuf), "%s.%s", p, dmntbl->dm_domain);
	    retval = tb_k2val (dmntbl->dm_table, TRUE, sdbuf, tdmnbuf);
	}  
	else
	    retval = tb_k2val (dmntbl->dm_table, TRUE, p, tdmnbuf);

	switch(retval) {
	case MAYBE:
	    return(MAYBE);
	default:
	    continue;
	case OK:
	    break;
	}
					/* get the entry */
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "dm_k2val: hit with '%s' for '%s.%s'",
		tdmnbuf, p, dmntbl->dm_domain);
#endif
	*(p - 1) = '\0';            /* build the domain */
	if (!isstr(dmntbl->dm_domain)) {
	    if (*p == '\0')
		(void) strcpy (domain, subdmn);
	    else
		sprintf (domain, "%s.%s", subdmn, p);
	}
	else
	    sprintf (domain, "%s.%s.%s", subdmn, p, dmntbl -> dm_domain);

	sprintf (dmnroute -> dm_buf, "%s %s", domain, tdmnbuf);
	dmnroute -> dm_argc = str2arg (dmnroute -> dm_buf,
			DM_NFIELD, dmnroute -> dm_argv, (char *)0);
	return (OK);
    }

    return (NOTOK);
}

/* ***********  GIVEN Subdomain, FIND HOST ROUTE  ************* */
/*
 * If BOTHEND is defined then we are almost certainly running in JNT
 * land and so we should try the reversed address before the RFC822
 * direction, should save some cpu cycles
 */

LOCFUN
Domain *
#ifndef BOTHEND
	dm_sd2route (value, domain, dmnroute)
    char *value;                /* what the use provides                */
    char *domain;               /* full domain value                    */
    Dmn_route *dmnroute;        /* source route                         */
#else
	dm_sd2route (value, reverse, domain, dmnroute)
    char *value;                /* what the use provides                */
    char *reverse;              /* the same bacwards                    */
    char *domain;               /* full domain value                    */
    Dmn_route *dmnroute;        /* source route                         */
#endif
{
    char *sdptr;
    Domain *dmnptr;
    char official[LINESIZE];
#ifdef BOTHEND
    char *revsdptr = reverse;
#endif
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_sd2route (%s)", seenull(value));
#endif
					/* first try the JNT way round if */
					/* got BOTHEND defined */
#ifdef	BOTHEND
    if ((dmnptr = dm_s2dom (reverse, official, dmnroute -> dm_buf)) !=
(Domain *) NOTOK || (dmnptr = dm_s2dom (value, official, dmnroute -> dm_buf))
	!= (Domain *) NOTOK)
#else
    if ((dmnptr = dm_s2dom (value, official, dmnroute -> dm_buf))
	!= (Domain *) NOTOK)
#endif
    {
	if(dmnptr == (Domain *)MAYBE)
	    return(dmnptr);
	(void) strcpy(domain, official);
	dmnroute -> dm_argc = str2arg (dmnroute -> dm_buf, DM_NFIELD,
		dmnroute -> dm_argv, (char *)0);
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "dm_sd2route: Domain value '%s' from '%s'",
		domain, dmnptr -> dm_show);
#endif
	return (dmnptr);
    }

    /*
     *  If not, move in from left.  Progressing on both forwards
     *  and backwards options.
     */
    sdptr = value;
    while ((sdptr = strchr (sdptr, '.')) != (char *) 0)
    {
	char tbuf[LINESIZE];

#ifdef  BOTHEND
	revsdptr = strchr (revsdptr, '.') + 1;
	if ((dmnptr = dm_s2dom (revsdptr, official, tbuf)) != (Domain *) NOTOK)
	{
	    if(dmnptr == (Domain *)MAYBE)
		return(dmnptr);
	    *(revsdptr - 1) = '\0';
	    sprintf (domain, "%s.%s", reverse, official);
	    sprintf (dmnroute -> dm_buf, "%s %s", domain, tbuf);
	    dmnroute -> dm_argc = str2arg (dmnroute -> dm_buf, DM_NFIELD,
		dmnroute -> dm_argv, (char *)0);
#ifdef DEBUG
	    ll_log(logptr,LLOGFTR, "Domain value (rev) '%s' from '%s',o = '%s'",
		domain, dmnptr -> dm_show, official);
#endif
	    return (dmnptr);
	}
#endif /* BOTHEND */

	if ((dmnptr = dm_s2dom (++sdptr, official, tbuf)) != (Domain *) NOTOK)
	{
	    if(dmnptr == (Domain *)MAYBE)
		return(dmnptr);
	    *(sdptr - 1) = '\0';
	    sprintf (domain, "%s.%s",  value, official);
	    sprintf (dmnroute -> dm_buf, "%s %s", domain, tbuf);
	    dmnroute -> dm_argc = str2arg (dmnroute -> dm_buf, DM_NFIELD,
		dmnroute -> dm_argv, (char *)0);
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "Domain value '%s' from '%s', o = '%s'",
		domain, dmnptr -> dm_show, official);
#endif
	    return (dmnptr);
	}
    }

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "'%s' not found", value);
#endif
    return ((Domain *) NOTOK);
}

/* ***********  GIVEN VALUE, FIND HOST ROUTE  ******************** */

/* This will take any value, and map it into a domain route.            */
/* It is optimised for val being a fully qualified domain               */

#ifndef BOTHEND

Domain *
	dm_v2route (value, domain, dmnroute)
    char *value;                /* what the use provides                */
    char *domain;               /* full domain value                    */
    register Dmn_route *dmnroute;        /* source route                */
{
    Dmn_route tmpdmn;           /* hold parsed domain string            */
    register Domain *dmnptr;
    Domain **seed;
    int sublen;                 /* length of right-hand to look up      */
    int ind;
    int perhaps = 0;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_v2route (%s)", seenull(value));
#endif
    if (value != (char *) 0)
        (void) strncpy (tmpdmn.dm_buf, value, sizeof(tmpdmn.dm_buf));
    else
        *tmpdmn.dm_buf = '\0';
    tmpdmn.dm_argc = cstr2arg (tmpdmn.dm_buf, DM_NFIELD, tmpdmn.dm_argv, '.');
    if (tmpdmn.dm_argc == NOTOK) {
#ifdef  DEBUG
	ll_log (logptr, LLOGTMP, "Cstr2arg failed (%s)", tmpdmn.dm_buf);
#endif
	return ( (Domain *)NOTOK);
    }

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "%d fields", tmpdmn.dm_argc);
#endif

    dmnroute -> dm_argc = 1;    /* initialize with something safe */
    (void) strncpy (dmnroute -> dm_buf, value, sizeof(dmnroute -> dm_buf));
    dmnroute -> dm_argv[0] = dmnroute -> dm_buf;

    tmpdmn.dm_argc--;

    for (ind = sublen = 0; ind <= tmpdmn.dm_argc; ind++)
    {
	if (ind != 0)
		tmpdmn.dm_buf[sublen - 1] = '.';
				/* undo cstr2arg!                       */
	sublen += strlen (tmpdmn.dm_argv[ind]);
				/* how much to peel off, from right     */
				/* the +1 is for the delimiter          */
	if (ind != tmpdmn.dm_argc)
		sublen++;       /* point to next if not last            */
	seed = (Domain **) 0;
	while ((dmnptr=dm_nm2struct(&value[sublen],&seed)) != (Domain *) NOTOK)
	{
	    switch(dm_k2val (dmnptr, tmpdmn.dm_argv[0], domain, dmnroute)){
	    case MAYBE:
	        if ((dmnptr->dm_table->tb_flags & TB_ABORT) == TB_ABORT)
		    return((Domain *) MAYBE);
		perhaps++;
		continue;       /* keep trying other domain entries */
	    case OK:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR,
		    "%s' hit in domain table '%s' with '%s'",
		    value, dmnptr -> dm_show, dmnroute -> dm_buf);
#endif
		return (dmnptr);
	    }
	}
    }
    return (perhaps? (Domain *) MAYBE : dm_sd2route (value, domain, dmnroute));
}

#else /* BOTHEND */

LOCFUN Domain *
dm_rv2route (value, domain, dmnroute)
    char *value;                /* what the use provides                */
    char *domain;               /* full domain value                    */
    register Dmn_route *dmnroute;        /* source route                */
{
    Dmn_route tmpdmn;           /* hold parsed domain string            */
    register Domain *dmnptr;
    int sublen;                 /* length of right-hand to look up      */
    int ind;
    char buf[LINESIZE];
    Domain **seed;
    int perhaps = 0;

    (void) strncpy (tmpdmn.dm_buf, value, sizeof(tmpdmn.dm_buf));
    tmpdmn.dm_argc = cstr2arg (tmpdmn.dm_buf, DM_NFIELD, tmpdmn.dm_argv, '.');
    if (tmpdmn.dm_argc == NOTOK) {
#ifdef  DEBUG
	ll_log (logptr, LLOGTMP, "Cstr2arg failed (%s)", tmpdmn.dm_buf);
#endif
	return ( (Domain *)NOTOK);
    }

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "%d fields", tmpdmn.dm_argc);
#endif

    tmpdmn.dm_argc--;
    for (ind = sublen = 0; ind <= tmpdmn.dm_argc; ind++)
    {
	if (ind != 0)
		tmpdmn.dm_buf[sublen - 1] = '.';
				/* undo cstr2arg!                       */
	sublen += strlen (tmpdmn.dm_argv[ind]);
				/* how much to peel off, from right     */
				/* the +1 is for the delimiter          */
	if (ind != tmpdmn.dm_argc)
		sublen++;       /* point to next if not last            */
	seed = (Domain **) 0;
	while ((dmnptr=dm_nm2struct(&value[sublen],&seed)) != (Domain *) NOTOK)
	{
	    switch(dm_k2val (dmnptr, tmpdmn.dm_argv[0], domain, dmnroute)){
	    case MAYBE:
	        if ((dmnptr->dm_table->tb_flags & TB_ABORT) == TB_ABORT)
		    return((Domain *) MAYBE);
		perhaps++;
		continue;       /* keep trying other domain entries */
	    case OK:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR,
		    "%s' hit in domain table '%s' with '%s'",
		    value, dmnptr -> dm_show, dmnroute -> dm_buf);
#endif
		return (dmnptr);
	    }
	}
    }
    return (perhaps? (Domain *) MAYBE : (Domain *) 0);     /* not found */
}

Domain *
	dm_v2route (value, domain, dmnroute)
    char *value;                /* what the use provides                */
    char *domain;               /* full domain value                    */
    Dmn_route *dmnroute;        /* source route                         */
{
    Domain *dmnptr;
    char *reverse;              /* to handle bigend mess                */
    extern char *ap_dmflip();

    if (value == (char *) 0) value = "";
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "dm_v2route (%s)", value);
#endif
    dmnroute -> dm_argc = 1;    /* initialize with something safe */
    (void) strncpy (dmnroute -> dm_buf, value, sizeof(dmnroute -> dm_buf));
    dmnroute -> dm_argv[0] = dmnroute -> dm_buf;

    if ((dmnptr = dm_rv2route (value, domain, dmnroute)) != (Domain *)0)
	return (dmnptr);
    reverse = ap_dmflip (value);
    if( (dmnptr = dm_rv2route (reverse, domain, dmnroute)) != (Domain *)0){
	free (reverse);
	return (dmnptr);
    }
    dmnptr = dm_sd2route (value, reverse, domain, dmnroute);
    free (reverse);
    return (dmnptr);
}
#endif /* BOTHEND */
