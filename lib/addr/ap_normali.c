#include "config.h"
#include "conf.h"
#include "ll_log.h"
#include "ch.h"
#include "ap.h"
#include "ap_norm.h"
#include "dm.h"

/*  Normalize a parse tree, to fill-in host references, etc.
 *
 *  Returns:    0  if ok
 *              -1 if error
 */

extern LLog *logptr;
extern struct ap_hstab *ap_exhstab;     /* translation table */
extern char *locname;
extern char *locdomain;
extern char *locfullname;
extern char *locfullmachine;

extern char *multcat ();
Domain *dm_v2route();

LOCFUN void logtree();
LOCFUN void ap_ptinit();

/**/

AP_ptr
	ap_normalize (dflhost, dfldomain, thetree, exorchan)
    char *dflhost,             /* string to append, as host name */
	 *dfldomain;           /* string to append, as domain name */
    AP_ptr thetree;             /* the parse tree */
    Chan  *exorchan;            /* The channel to exorcise with respect to */
{
    struct ap_node  basenode;     /* first node in routing chain          */
    AP_ptr r822prefptr,
	   perptr,
	   mbxprefptr,
	   dmprefptr,
	   lstcmntprefptr,
	   lastptr,
	   grpptr,
	   ap;
    struct ap_hstab  *tabp;
    char    official[128];
    char *save;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ap_normalize (%s, %s, ..., %s)",
		seenull(dflhost), seenull(dfldomain),
		exorchan ? seenull(exorchan->ch_show) : "no-chan");
    logtree (thetree, "normalize input");
#endif

    ap_ninit (&basenode);
    ap_sqinsert (&basenode, APP_ETC, thetree);
			    /* make sure that 'pref' ptrs are easy  */
    ap_ptinit (&basenode, &perptr, &r822prefptr, &mbxprefptr, &dmprefptr,
				&lstcmntprefptr, &lastptr, &grpptr);
				/* set some pointers */


			/* SEK - lts try replacing this section        */
			/* It may have marginal use                    */
    if (perptr == (AP_ptr) 0 && grpptr == (AP_ptr) 0
	&& lstcmntprefptr != (AP_ptr) 0)
    {                   /* no person name or group, so use last comment */
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "ap_normalize: comment -> person name");
#endif

	ap_insert (&basenode, APP_ETC,
		    ap_new (APV_NPER,
				lstcmntprefptr -> ap_chain -> ap_obvalue));

	if (mbxprefptr == &basenode)  /* first node was local-part */
	    mbxprefptr = basenode.ap_chain;

	ap_append (lastptr, APV_EPER, (char *) 0);
#ifdef DEBUG
	logtree (basenode.ap_chain, "ap_normalize: comment result");
#endif
    }

    /*
     *  Remember "list: ;"...   -DPK-
     */
    if (mbxprefptr == (AP_ptr) 0) {
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "ap_normalize: No mailbox!?!");
#endif
	return (basenode.ap_chain);
    }

				/* Normalize all refs in source route   */
    if (r822prefptr != (AP_ptr) 0)
    {
	ap = r822prefptr;
	FOREVER
	{
	    switch (ap -> ap_ptrtype) {
		case APP_NIL:
		case APP_NXT:
		    break;
		case  APP_ETC:
		    ap = ap -> ap_chain;
		    if(ap == (AP_ptr)0)
			break;
		    switch (ap -> ap_obtype) {
		    case APV_DOMN:
		   	if(ap_dmnormalize (ap, exorchan) == MAYBE)
				return( (AP_ptr)MAYBE);
		    case APV_DLIT:
		    case APV_CMNT:
			continue;
		    }
	    }
	    break;
	}
    }

#ifndef JNTMAIL
    /*
     *  SEK if JNT mail, % is treated as lexically equivalent
     *  to @, and CSNET style routes ignored.  Might accept
     *  CSNET routes if someone wants this...
     */
    if (dmprefptr == (AP_ptr) 0)
    {                           /* no domain, so add default and leave */
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "ap_normalize: no domain");
#endif
	ap_locnormalize (&basenode, &r822prefptr, &mbxprefptr, &dmprefptr);
#ifdef DEBUG
	logtree (basenode.ap_chain, "ap_normalize: locnorm result");
#endif
    }
#endif /* not JNTMAIL */

    if (dmprefptr != (AP_ptr) 0) {
	switch(ap_dmnormalize (dmprefptr -> ap_chain, exorchan)) {
	case MAYBE:
	    return ( (AP_ptr)MAYBE);
	case OK:;
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "ap_normalize: Local reference");
#endif
#ifndef JNTMAIL
	    ap_locnormalize (&basenode, &r822prefptr, &mbxprefptr, &dmprefptr);
#ifdef DEBUG
	logtree (basenode.ap_chain, "ap_normalize: locnorm(2) result");
#endif
#endif /* not JNTMAIL */
	}
    }


    if (dmprefptr == (AP_ptr) 0 && r822prefptr == (AP_ptr) 0)
    {                           /* no host references anywhere */
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "ap_normalize: mbx = %o, mbx->typ = %o, mbx->ch = %o",
		mbxprefptr, mbxprefptr->ap_obtype, mbxprefptr->ap_chain);
	if (mbxprefptr != (AP_ptr) 0)
		ll_log (logptr, LLOGFTR, "mbx->ch->ty = %o", mbxprefptr->ap_chain->ap_obtype);
#endif
	if (mbxprefptr == (AP_ptr) 0
	   || mbxprefptr -> ap_chain -> ap_obtype == APV_NGRP
	   || mbxprefptr -> ap_chain -> ap_obtype == APV_GRUP)  /* Yuck. */
	    return (basenode.ap_chain);

	if (!isstr(dflhost))
	{
	    strncpy(official, locfullname, sizeof(official));
	    ap_append (mbxprefptr -> ap_chain, APV_DOMN, official);
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "appending %s domain (local)",
		    official);
#endif
	}
	else
	{
	    if (!isstr(dfldomain))
		(void) strncpy (official, dflhost, sizeof(official));
	    else
		snprintf (official, sizeof(official), "%s.%s", dflhost, dfldomain);
	    ap_append (mbxprefptr -> ap_chain, APV_DOMN, official);
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "appending %s domain",
		    official);
#endif
	    if(ap_dmnormalize (mbxprefptr -> ap_chain -> ap_chain,
							exorchan) == MAYBE)
		return( (AP_ptr)MAYBE);
	}
	return (basenode.ap_chain);
    }

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "exorcise special host refs");
#endif
				/* SEK change the dreaded exorcising */
				/* so as only to apply to the host   */
				/* directly connected to.  It is     */
				/* absurd to do this to the middle of*/
				/* a source route                    */
    if (r822prefptr != (AP_ptr) 0)
	ap = r822prefptr -> ap_chain;
    else
	ap = dmprefptr -> ap_chain;

    for (tabp = ap_exhstab; tabp -> name != (char *) 0; tabp++)
	if (lexequ (tabp -> name, ap -> ap_obvalue))
	{             /* Yes, perform holy rites */
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "exorcising: %s -> %%%s@%s",
			ap -> ap_obvalue, tabp -> dot, tabp -> at);
#endif
	    if (isstr (tabp -> dot)) {
		free (ap -> ap_obvalue);
		ap -> ap_obvalue = strdup (tabp -> at);
		save = mbxprefptr -> ap_chain -> ap_obvalue;
		mbxprefptr -> ap_chain -> ap_obvalue = multcat (
		   mbxprefptr -> ap_chain -> ap_obvalue, "%", tabp -> dot, (char *)0);
	    } else {
		save = ap -> ap_obvalue;
		ap -> ap_obvalue = strdup (tabp -> at);
	    }
	    free (save);
	    return (basenode.ap_chain);
	}

    /*
     *  We do not have a specific exorcising rule.  See if the
     *  host is known to this channel, and if not, exocise it anyway.
     *
     *  (I think this will work, closer examination welcome.  -DPK-)
     *
     *   SEK -
     *          only play this game if ch_known is specified.
     *           to allow it to be turned off
     *
     *          assume that the table is a channel table form
     *          and thus we need the first host name
     */
    if (exorchan != NULL && exorchan -> ch_known != NULL) {
	Dmn_route route;
	char dm_buf[LINESIZE];
	Domain *lrval;
	int irval;

	lrval = dm_v2route (ap -> ap_obvalue, dm_buf, &route);
	if(lrval == (Domain *)MAYBE)
	    return( (AP_ptr)MAYBE);
	else if(lrval == (Domain *)NOTOK) {
	    ll_log (logptr, LLOGTMP, "failure to reconvert domain (%s)",
			ap-> ap_obvalue);
	    return (basenode.ap_chain);
	}
				/* SEK this might need beefing up to cope    */
				/* with unknown subdomains              */

#ifdef HAVE_WILDCARD
	irval = tb_wk2val(exorchan->ch_known, TRUE, route.dm_argv[0], official);
#else /* HAVE_WILDCARD */
	irval = tb_k2val(exorchan->ch_known, TRUE, route.dm_argv[0], official);
#endif /* HAVE_WILDCARD */
	if(irval == MAYBE)
	    return( (AP_ptr)MAYBE);
	else if(irval == NOTOK) {
	    if (r822prefptr == (AP_ptr) 0) {
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "exorcising: %s -> %%%s@%s",
			ap -> ap_obvalue,
			route.dm_argv[0], exorchan -> ch_lname);
#endif

		save = mbxprefptr -> ap_chain -> ap_obvalue;
		mbxprefptr -> ap_chain -> ap_obvalue = multcat (
			mbxprefptr->ap_chain->ap_obvalue, "%",
			route.dm_argv[0], (char *)0);
		free (save);
		free (dmprefptr -> ap_chain -> ap_obvalue);
		dmprefptr -> ap_chain -> ap_obvalue =
			multcat (exorchan->ch_lname,
				    ".", exorchan->ch_ldomain, (char *)0);
	   } else {
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "adding exorcise component to route");
#endif
			      /* SEK note ap points to r822prefptr - chain */
		ap_sqinsert (ap, APP_ETC, ap_new (ap -> ap_obtype, ap -> ap_obvalue));
		free (ap -> ap_obvalue);
		ap -> ap_obvalue = multcat (exorchan->ch_lname,
						".", exorchan->ch_ldomain, (char *)0);
	   }
        }
    }
    return (basenode.ap_chain);
}
/**/

LOCFUN void
	ap_ptinit (baseprefptr, perptr, r822prefptr, mbxprefptr, dmprefptr,
				lstcmntprefptr, lastptr, grpptr)
    AP_ptr baseprefptr;
    AP_ptr *perptr,
	   *r822prefptr,
	   *mbxprefptr,
	   *dmprefptr,
	   *lstcmntprefptr,
	   *lastptr,
	   *grpptr;
{
    AP_ptr ap;

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "ap_ptinit (%s, %d)", 
		seenull(baseprefptr->ap_obvalue), baseprefptr->ap_obtype);
#endif

    *perptr =
	*r822prefptr =
	*mbxprefptr =
	*dmprefptr =
	*lstcmntprefptr =
	*lastptr =
	*grpptr = (AP_ptr) 0;

				/* SEK need switch here to catch */
				/* leadin mbox or domn           */
     switch (baseprefptr -> ap_chain -> ap_obtype)
     {
	case APV_MBOX:
	    *mbxprefptr = baseprefptr;
	    break;
	case APV_DLIT:
	case APV_DOMN:
	    *r822prefptr = baseprefptr;
     }


    for (ap = baseprefptr -> ap_chain; ap -> ap_obtype != APV_NIL;
		ap = ap -> ap_chain)
    {
	*lastptr = ap;
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "ap_ptinit: val '%s'", seenull(ap -> ap_obvalue));
#endif
	switch (ap -> ap_obtype)
	{
	    case APV_NPER:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "ap_ptinit: perptr");
#endif
		*perptr = ap;
		break;

	    case APV_NGRP:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "ap_ptinit: grpptr");
#endif
		*grpptr = ap;
		break;
	}

	if (ap -> ap_ptrtype == APP_NXT)
	    break;
	if (ap -> ap_ptrtype == APP_NIL)
	    break;
	if (ap -> ap_chain == (AP_ptr) 0)
	    break;

	switch (ap -> ap_chain -> ap_obtype)
	{
	    case APV_CMNT:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "ap_ptinit: gotcmnt");
#endif
		*lstcmntprefptr = ap;
		break;                          /** DPK@BRL **/

	    case APV_WORD:                      /** DPK@BRL **/
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "ap_ptinit: gotword");
#endif
		if (*mbxprefptr != (AP_ptr) 0)
		    break;

	    case APV_MBOX:
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "ap_ptinit: gotmbxpref");
#endif
		*mbxprefptr = ap;    /* one before the mbox                */
		break;

	    case APV_DLIT:
	    case APV_DOMN:
		if (*r822prefptr == (AP_ptr) 0 && *mbxprefptr == (AP_ptr) 0) {
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "ap_ptinit: got r822prefptr");
#endif
		    *r822prefptr = ap;
		} else
		if ((*dmprefptr == (AP_ptr) 0) && (*mbxprefptr != (AP_ptr) 0)) {
					/* SEK need mailbox befor domain */
#ifdef DEBUG
		    ll_log (logptr, LLOGFTR, "ap_ptinit: gotdmprefptr");
#endif
		    *dmprefptr = ap;
		}
		break;
	}
    }
#ifdef DEBUG
    if (*lastptr == (AP_ptr) 0)
	ll_log (logptr, LLOGFTR, "ap_ptinit: no lastptr");
    else
	ll_log (logptr, LLOGFTR, "ap_ptinit: lastptr '%s'", 
			seenull((*lastptr) -> ap_obvalue));
#endif

}
/**/
#ifndef	JNTMAIL

void ap_locnormalize (obaseptr, or822prefptr, ombxprefptr, odmprefptr)
    AP_ptr obaseptr,
	   *or822prefptr,
	   *ombxprefptr,
	   *odmprefptr;
{                           /* tear local-part apart            */
    struct ap_node basenode;
    AP_ptr curptr;
    AP_ptr r822prefptr,
	   perptr,
	   mbxprefptr,
	   dmprefptr,
	   lstcmntprefptr,
	   lastptr,
	   grpptr;
    char *cptr;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ap_locnormalize ()");
#endif
				/* SEK need fixex here to normalize any new */
				/* domain references                        */

#ifdef DONE_IN_SUBMIT
    if( ! ((*ombxprefptr) -> ap_obtype == APV_DTYP &&   /* protect files */
      lexequ((*ombxprefptr) -> ap_obvalue, "include")))
	for (cptr = (*ombxprefptr) -> ap_chain -> ap_obvalue;
	     *cptr != '\0'; cptr++)
	    if (*cptr == '.' || *cptr == '%')
		*cptr = '@';
#endif

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "parsing '%s'", (*ombxprefptr) -> ap_chain -> ap_obvalue);
#endif

    ap_ninit (&basenode);
    ap_sqinsert (&basenode, APP_ETC,
		    ap_s2tree ((*ombxprefptr) -> ap_chain -> ap_obvalue));
#ifdef DEBUG
	logtree (obaseptr -> ap_chain, "parse result");
#endif
    ap_ptinit (&basenode, &perptr, &r822prefptr, &mbxprefptr, &dmprefptr,
				&lstcmntprefptr, &lastptr, &grpptr);
    if (dmprefptr != (AP_ptr) 0)    /* actually have some stuff */
    {
	free ((*ombxprefptr) -> ap_chain -> ap_obvalue);
	(*ombxprefptr) -> ap_chain -> ap_obvalue =
		    strdup (mbxprefptr -> ap_chain -> ap_obvalue);
			    /* replace old reference */
#ifdef DEBUG
	ll_log (logptr, LLOGFTR,
		"newlocal '%s'", (*ombxprefptr) -> ap_chain -> ap_obvalue);
#endif
	if (r822prefptr != (AP_ptr) 0 || *odmprefptr != (AP_ptr) 0)
	    if (*or822prefptr == (AP_ptr) 0) {
#ifdef DEBUG
		logtree (obaseptr -> ap_chain, "odm move result");
#endif
		*or822prefptr = obaseptr;
	    }                   /* initialize route pointer */

	if (*odmprefptr == (AP_ptr) 0)
	    *odmprefptr = mbxprefptr -> ap_chain;
	else                    /* get rid of old domain reference */
	{
	    ap_move (*or822prefptr, *odmprefptr);
#ifdef DEBUG
	    logtree (obaseptr -> ap_chain, "odm move result");
#endif
	}

	if (r822prefptr != (AP_ptr) 0)
	{                       /* put new chain at end of old          */
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR, "adding new routing to end info");
#endif
	    for (curptr = *or822prefptr;
			curptr -> ap_chain -> ap_obtype == APV_DOMN ||
			    curptr -> ap_chain -> ap_obtype == APV_DLIT ||
			    curptr -> ap_chain -> ap_obtype == APV_CMNT;
			curptr = curptr -> ap_chain);
	    ap_sqmove (curptr, r822prefptr, APV_DOMN);
#ifdef DEBUG
	    logtree (obaseptr -> ap_chain, "routing move result");
#endif
	}

	ap_move (*odmprefptr, dmprefptr);
#ifdef DEBUG
	logtree (obaseptr -> ap_chain, "dm move result");
#endif
    }
}
#endif /* not	JNTMAIL */

/**/

int ap_dmnormalize (dmptr, thechan)
register AP_ptr dmptr;
Chan *thechan;
{
    Dmn_route dmnroute;
    char official [LINESIZE];
    char buf[LINESIZE];
    Domain *domain;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ap_dmnormalize (%s, %s)",
		seenull(dmptr->ap_obvalue),
		thechan ? seenull(thechan->ch_show) : "no-chan");
#endif

    domain = dm_v2route (dmptr -> ap_obvalue, official, &dmnroute);
    switch ((int)domain) {
					/* do we know this reference? */
	case NOTOK:			/* unknown host */
#ifdef DEBUG
	    ll_log (logptr, LLOGBTR, "unknown domain (ap_dmnormalize): '%s'",
				dmptr -> ap_obvalue);
#endif
	    switch ((int)ch_nm2struct (dmptr -> ap_obvalue))
	    {                   /* explicit channel routing? */
		default:        /* it IS a channel name */
		    return (OK);
				/* return an analyse local part        */
				/* leave channel ref in address        */
		case NOTOK:
		   ll_log (logptr, LLOGFST,
				"nonexistent dom/host (ap_dmnormalize): '%s'",
				seenull(dmptr -> ap_obvalue));
		    return (NOTOK);
	    }
	case MAYBE:
		return(MAYBE);
	default:
	    strncpy (buf, locfullname, sizeof(buf));
				/* SEK might be better to get dm_v2route */
				/* to do this                            */
	    if (lexequ (official, buf) && (thechan != NULL)) {
		if (isstr(thechan -> ch_ldomain))
		    sprintf (official, "%s.%s", thechan -> ch_lname,
				thechan -> ch_ldomain);
		else
		    snprintf (official, sizeof(official), "%s", thechan -> ch_lname);
	    }
    }

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "ap_dmnormalize: Route = '%s', Official name = '%s'",
		dmnroute.dm_buf, official);
#endif
    free (dmptr -> ap_obvalue);
    dmptr -> ap_obvalue = strdup (official);
    return (7);                 /* anything other than OK /  NOTOK */
}

/**/

#ifdef DEBUG
LOCFUN void
     logtree(thetree, text)  /* if FTR, convert tree to string & log */

AP_ptr thetree;
char *text;
{
    if (logptr->ll_level == LLOGFTR) {

	char *cptr;

	ap_t2s (thetree, &cptr);
	ll_log (logptr, LLOGFTR, "%s:  %s", text, cptr);
	free (cptr);
    }
}
#endif /* DEBUG */
