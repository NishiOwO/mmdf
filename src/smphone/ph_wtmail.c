#include "util.h"
#include "mmdf.h"
#include "adr_queue.h"
#include "ph.h"
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

extern struct ll_struct   *logptr;

/* **************  (ph_)  PHONE MAIL-WRITING SUB-MODULE  ************** */

LOCVAR short  ph_nadrs;             /* number of addresses this message   */
LOCVAR  long ph_nchrs;            /* number of chars in msg text        */
LOCVAR struct rp_construct
	rp_gdtxt =
{
    RP_MOK, 't', 'e', 'x', 't', ' ', 's', 'e', 'n', 't', ' ', 'o', 'k', '\0'
};

struct ps_rstruct ps_rp;

/* ARGSUSED */

ph_winit (vianet, info, retadr)  /* pass msg initialization info       */
char    vianet[];		  /* what channel coming in from        */
char    info[],			  /* general info                       */
        retadr[];		  /* return address for error msgs      */
{
    short     retval;
    char  initbuf[LINESIZE];


/* DBG:  make sure info has right form */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_winit");
#endif

    ph_nadrs = 0;
    ph_nchrs = 0L;
    
    switch (info[0])
    { 
	case (ADR_TTY):
	    snprintf (initbuf, sizeof(initbuf), "SEND FROM:<%s>", retadr);
	    break;
	case (ADR_BOTH):
	    snprintf (initbuf, sizeof(initbuf), "SAML FROM:<%s>", retadr);
	    break;
	case (ADR_TorM):
	    snprintf (initbuf, sizeof(initbuf), "SOML FROM:<%s>", retadr);
	    break;
	case (ADR_MAIL):
	default:
	    snprintf (initbuf, sizeof(initbuf), "MAIL FROM:<%s>", retadr);
	    break;
    }    
    retval = ps_cmd (initbuf);  /* ignore return */
    return (retval);
}
/**/

ph_wadr (host, adr, thereply)  /* send one address spec to local     */
char    host[],			  /* "next" location part of address    */
        adr[];			  /* rest of address                    */
struct rp_bufstruct *thereply;     
{
    extern char *multcat ();
    short     retval;
    register char  *adrstr;
    char linebuf[LINESIZE];

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wadr(%s, %s)", host, adr);
#endif

    ph_nadrs++;

    snprintf (linebuf, sizeof(linebuf), "RCPT TO:<%s>", adr);
    if (rp_isbad (ps_cmd (linebuf)))
	return (RP_DHST);

    switch (ps_rp.sm_rval)
    {
	case 250:
	case 251:
	    ps_rp.sm_rval = RP_AOK;
	    break;

	case 421:
	case 450:
	case 451:
	case 452:
	    ps_rp.sm_rval = RP_NO;
	    break;

	case 550:
	case 551:
	case 552:
	case 553:
	    ps_rp.sm_rval = RP_USER;
	    break;

	case 500:
	case 501:
	    ps_rp.sm_rval = RP_PARM;
	    break;

	default:
	    ps_rp.sm_rval = RP_RPLY;
    }

    if (ps_rp.sm_rgot) 
	strncpy (thereply->rp_line, ps_rp.sm_rstr, ps_rp.sm_rlen+1);
    else
	strncpy (thereply->rp_line, "Unknown Problem", sizeof(thereply->rp_line));
    
    thereply->rp_val = ps_rp.sm_rval;
    return (RP_OK);
}

ph_waend ()                      /* end of address list                */
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_waend");
#endif

    return (RP_DONE);
}
/**/
ph_wtinit()
{

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wtinit()");
#endif

    if (rp_isbad (ps_cmd ("DATA"))) {
	return(RP_RPLY);
    }

    switch( ps_rp.sm_rval ) {
    case 354:
	break;          /* Go ahead and send mail */

    case 500:
    case 501:
    case 503:
    case 554:
	return(RP_NDEL);

    case 421:
    case 451:
    default:
	return(RP_AGN);
    }
    return (RP_OK);
}

ph_wtxt (buffer, len)            /* send next part of msg text         */
char    buffer[];		  /* the text                           */
short     len;                      /* length of text                     */
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wtxt()");
#endif

    ph_nchrs += (long) len;

    return (ph_wstm (buffer, len));
}

ph_wtend (thereply)        /* end of message text                */
struct rp_bufstruct *thereply;
{

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wtend");
#endif

    if (rp_isbad (ph_wstm ((char *) 0, 0))) {
				/* flush text buffer / send eos       */
	strncpy (thereply->rp_line, "Error flushing text to remote host", sizeof(thereply->rp_line));
	return(thereply->rp_val = RP_RPLY);
    }

    if (rp_isbad (ps_cmd("."))) {
	strncpy (thereply->rp_line, "Bad response to final dot", sizeof(thereply->rp_line));
	return(thereply->rp_val = RP_BHST);
    }

    switch( ps_rp.sm_rval ) {
    case 250:
    case 251:
	blt (&rp_gdtxt, (char *)thereply, sizeof rp_gdtxt);
	break;

    case 552:
    case 554:
	thereply->rp_val = RP_NDEL;
	if (ps_rp.sm_rgot)
		strncpy (thereply->rp_line, ps_rp.sm_rstr, ps_rp.sm_rlen+1);
	else
		strncpy (thereply->rp_line, "Unknown Problem", sizeof(thereply->rp_line));
	break;

    case 421:
    case 451:
    case 452:
    default:
	thereply->rp_val = RP_AGN;
	if (ps_rp.sm_rgot)
		strncpy (thereply->rp_line, ps_rp.sm_rstr, ps_rp.sm_rlen+1);
	else     
		strncpy (thereply->rp_line, "Unknown Problem", sizeof(thereply->rp_line));
    }

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wtend reply (%s) %s",
		             rp_valstr(thereply->rp_val), thereply->rp_line);
#endif DEBUG

    return(RP_OK);

}
