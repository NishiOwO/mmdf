/*
 *     MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *     Copyright (C) 1979,1980,1981  University of Delaware
 *
 *     Department of Electrical Engineering
 *     University of Delaware
 *     Newark, Delaware  19711
 *
 *     Phone:  (302) 738-1163
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
/*                SEND FROM DELIVER TO SUBMIT
 *
 *  Feb 83  Doug Kingston       Initial version of list processor
 *  Apr 86  Craig Partridge     Minor efficiency tweaking
 *  Dec 87  Dan Long            Converted to address blocking function
 */

#include "util.h"
#include "mmdf.h"
#include "phs.h"
#include "ch.h"
#include "ap.h"
#include <pwd.h>
#include "ml_send.h"
#include "mm_io.h"

extern struct ll_struct   *logptr;
extern Chan *chanptr;
extern long qu_msglen;
extern char *supportaddr;

extern char *index();
extern char *rindex();
extern char *strdup();
extern char *multcat();
extern char *blt();

extern int ml_state;              /* to fake ml_send(3) out */

LOCVAR int  nadrs, ndone, notify;
LOCVAR char sender[ADDRSIZE];
LOCVAR char adr[ADDRSIZE];
LOCVAR int  started = FALSE;
LOCVAR int  rejecting = FALSE;
LOCVAR char *notifylist[128];

LOCVAR struct rp_construct
	rp_bdrem =
{
    RP_BHST, 'B', 'a', 'd', ' ', 'r', 'e', 's', 'p', 'o', 'n', 's', 'e',
    ' ', 'f', 'r', 'o', 'm', ' ', 's', 'u', 'b', 'm', 'i', 't', '\0'
},
	rp_adr =
{
    RP_AOK, 'a', 'd', 'd', 'r', 'e', 's', 's', ' ', 'o', 'k', '\0'
},
	rp_gdtxt =
{
    RP_MOK, 't', 'e', 'x', 't', ' ', 's', 'e', 'n', 't', ' ', 'o', 'k', '\0'
},
	rp_noop =
{
    RP_NOOP, 's', 'u', 'b', '-', 'l', 'i', 's', 't', ' ', 'n', 'o', 't', ' ',
    's', 'p', 'e', 'c', 'i', 'a', 'l', '\0'
},
	rp_vhost =
{
    RP_USER, 'H', 'o', 's', 't', 'n', 'a', 'm', 'e', ' ', 'n', 'o', ' ', 'l',
    'o', 'n', 'g', 'e', 'r', ' ', 'v', 'a', 'l', 'i', 'd', '\0'
},
	rp_vuser =
{
    RP_USER, 'U', 's', 'e', 'r', 'n', 'a', 'm', 'e', ' ', 'n', 'o', ' ', 'l',
    'o', 'n', 'g', 'e', 'r', ' ', 'v', 'a', 'l', 'i', 'd', '\0'
};

/**/

qu2ba_send ()             /* overall mngmt for batch of msgs    */
{
    short       result;
    char        info[LINESIZE];

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "qu2ba_send ()");
#endif

    if (rp_isbad (result = qu_pkinit ()))
	return (result);

    if (isstr(chanptr->ch_confstr) && lowtoup(*chanptr->ch_confstr) == 'R')
	rejecting=TRUE;

    /*
     *  While there are messages to process ...
     */
    for(;;){                /* get initial info for new message   */
	result = qu_rinit (info, sender, chanptr -> ch_apout);

	switch(rp_gval(result)) {

	    case RP_NS:
	    case RP_FIO:
		qu_rend();
		continue;

	    case RP_OK:
		break;

	    default:
		goto done;
	}

	/* make sure submit is running */

	if ((!started) && (rp_isbad(mm_init()) || rp_isbad(mm_sbinit()))) {
    	    /* quit... */
    	    ll_log(logptr, LLOGTMP, "couldn't start submit");
    	    goto done;
        }
	started = TRUE;
	notify = 0;

	switch (rp_gbval (result = qu2ba_each())) {

	    case RP_BOK:
	    case RP_BPOK:
		break;

	    case RP_BTNO:	/*  submit was difficult */
		started = FALSE;
		break;

	    case RP_BNO:	/* deliver was difficult */
	    default:
		goto done;
	}

	if (notify)
	    sendnotification();

	qu_rend();
    }

done:

    qu_rend();

    if (rp_gval (result) != RP_DONE) {
	ll_log (logptr, LLOGFAT, "not DONE (%s)", rp_valstr (result));
	return (RP_RPLY);         /* catch protocol errors              */
    }

    if (started)
	mm_end(OK);

    qu_pkend ();                  /* done getting messages              */
    return (result);
}
/**/

LOCFUN
	qu2ba_each ()            /* send one copy of text per message */
{
    struct rp_bufstruct thereply;
    short   result;
    int     len;
    char    host[ADDRSIZE];
    char    info[LINESIZE];
    char    *adrp;
    char    *colonp,*commap,*atp;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "qu2ba_each()");
#endif

    nadrs = ndone = 0;

    /*
     *  For every address in the message ...
     */
    while( 1 ) {
	result = qu_radr (host, adr);
	if (rp_isbad (result))
	    return (RP_BNO);      /* get address from Deliver           */

	switch (rp_gval (result))
	{
	    case RP_HOK:          /* end of sub-list */
		qu_wrply ((RP_Buf *)&rp_noop, sizeof rp_noop);
		break;

	    case RP_DONE:         /* end of full address list           */
		if (ndone != 0) {
		    qu2ba_txtcpy(&thereply);
    		    qu_wrply(&thereply, sizeof (thereply.rp_val) +
    		        strlen (thereply.rp_line));
		    if (rp_isbad(thereply.rp_val)) {
			mm_end(NOTOK);
			return(RP_AGN);
		    }
    		    return (RP_OK);         /* END for this message */
		} else {
		    qu_wrply ((RP_Buf *)&rp_bdrem, sizeof rp_bdrem);
		    mm_end(NOTOK);
		    return(RP_AGN);
		}

	    default:            /* actually have an address */
		/* Build notification string for sender */
		if ((colonp = index(adr, ':')) == (char *) 0) {

		    /* username[@host] */
		    adrp = adr;
		    if ((atp = rindex (adrp, '@')) != (char *) 0)
		        *atp = '\0';   /* strip hostname */         
		    if ((tb_k2val(chanptr->ch_table, 1, adrp, info) != OK) ||
		        (strlen(info) == 0))    
			    notifylist[++notify] = multcat(adrp,  
				(rejecting)?
				    "\t<== This username is no longer valid\n":
				    "\t <== This username is soon to expire\n",
				(char *) 0);
		    else
    			    notifylist[++notify] = multcat(adrp,
					"\t<== ", info, "\n",  (char *) 0);
    		    if (rejecting)
    		        /* Tell deliver this is a bad address */
    		        qu_wrply((RP_Buf *)&rp_vuser, sizeof rp_vuser);
		    else
			/* avoid mail loops */
			adrp = multcat("~", adr, (char *) 0);

		} else {

		    /* @channel[,route[,route]]:username[@host] */
		    if ((commap=index(adr, ',')) == (char *) 0) {

			adrp=colonp+1;

			/* username[@host] */
			if (((atp = rindex (adrp, '@')) == (char *) 0) ||
    		        (tb_k2val(chanptr->ch_table, 1, atp+1, info) != OK) ||
    		        (strlen(info) == 0))
    			    notifylist[++notify] = multcat(adrp, 
			                "\t<== Use this address\n", (char *) 0);
			else
			    notifylist[++notify] = multcat(adrp,
					"\t<== ", info, "\n", (char *) 0);
		    } else {

			adrp = commap+1;

    		        /* @route[,@route]:username[@host] */
			if (((atp = rindex (colonp+1, '@')) == (char *) 0) ||
    		        (tb_k2val(chanptr->ch_table, 1, atp+1, info) != OK) ||
    		        (strlen(info) == 0)) {
			    *colonp = '\0';
			    atp = rindex(adrp,'@'); /* start of last route */
			    notifylist[++notify] = multcat(colonp+1, 
			        "\t<== This is probably not a good address.\n\t\t\t(Write \"Postmaster", 
                                atp, "\" for advice.)\n", (char *) 0);
			    *colonp = ':';
			} else
			    notifylist[++notify] = multcat(colonp+1,
					"\t<== ", info, "\n", (char *) 0);
		    }
    		    if (rejecting)
        	        /* Tell deliver this is a bad address */
	    	        qu_wrply((RP_Buf *)&rp_vhost, sizeof rp_vhost);
		}
        	ll_log(logptr, LLOGFST, "%s to %s for %s",
       		       (rejecting)?"Rejection":"Warning", sender, adrp);

		if (!rejecting)
    		    if (rp_isbad(result = submitaddr(adrp)))
			return(result);

	} /* end switch */
    }
    /* NOTREACHED */
}

/**/

LOCFUN
	qu2ba_txtcpy (rp)     /* copy the text of the message       */
RP_Buf *rp;
{
    int       len;
    short     result;
    char      buffer[BUFSIZ];

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "qu2ba_txtcpy()");
#endif

    qu_rtinit (0L);

    if (rp_isbad (result = mm_waend())) {
	blt ((char *)&rp_bdrem, (char *) rp, sizeof rp_bdrem);
	return;
    }

    printx ("sending...:");
    fflush (stdout);

    len = sizeof(buffer);
    while ((rp_gval (result = qu_rtxt (buffer, &len))) == RP_OK)
    {
	result = mm_wtxt (buffer, len);
	if (rp_isbad (result))
	    break;
	printx (".");
	fflush (stdout);
	if (rp_gval (result) != RP_OK) {
	    blt ((char *)&rp_bdrem, (char *) rp, sizeof rp_bdrem);
	    return;
	}
	len = sizeof(buffer);
    }

    if (rp_isbad (result) || rp_gval (result) != RP_DONE) {
	blt ((char *)&rp_bdrem, (char *) rp, sizeof rp_bdrem);
	return;
    }

    if (rp_isbad(mm_wtend()) || rp_isbad(mm_rrply(rp,&len))) {
	rp->rp_val = RP_RPLY;
	strcpy(rp->rp_line,"Unknown problem");
	return;
    }

    blt ((char *)&rp_gdtxt, (char *)rp, sizeof rp_gdtxt);
}

/**/

LOCFUN
	submitaddr(adrp)           /* pass adr to submit */
char *adrp;
{
    struct rp_bufstruct thereply;
    int len;

    /* Use the new address for resubmission */
    if (nadrs++ == 0) {

        /* first address -- need to crank up submit first */
		    /* should allow W to be passed here */
        if (rp_isbad (mm_winit (chanptr->ch_name, "tlvmk30*", sender))
	      || rp_isbad (mm_rrply(&thereply, &len))) {
	    mm_end (NOTOK);
	    printx ("Error in submit startup\n");
	    fflush (stdout);
	    qu_wrply((RP_Buf *)&rp_bdrem, sizeof rp_bdrem);
	    return(RP_AGN);
        }

        switch (rp_gbval (thereply.rp_val)) {
        case RP_BNO:
        case RP_BTNO:
            mm_end (NOTOK);
            printx ("Error in submit startup (%s)\n", thereply.rp_line);
            fflush (stdout);
            qu_wrply(&thereply, len);
            return(RP_AGN);
        }
        if (rp_isbad (mm_wadr ("", adrp))
             || rp_isbad (mm_rrply(&thereply, &len))) {
            mm_end (NOTOK);
            printx ("Error in passing first address\n");
            fflush (stdout);
            qu_wrply((RP_Buf *)&rp_bdrem, sizeof rp_bdrem);
            return(RP_AGN);
        }
    }
    else /* subsequest addresses */
    {
        if (rp_isbad (mm_wadr ("", adrp))
               || rp_isbad (mm_rrply(&thereply, &len)))
        {
            mm_end (NOTOK);
            printx ("Error in passing address\n");
            fflush (stdout);
            qu_wrply((RP_Buf *)&rp_bdrem, sizeof rp_bdrem);
            return(RP_AGN);
        }
    }
    
    switch (rp_gval (thereply.rp_val))
    {                 /* was address acceptable?            */
        case RP_AOK:
        case RP_DOK:
	    qu_wrply((RP_Buf *)&rp_adr, sizeof  rp_adr);
	    ndone++;
	    return(RP_OK);

        case RP_PARM:
        case RP_USER:
        case RP_NO:
        case RP_NS:
	    break;    /* report failure and continue        */

        case RP_RPLY:
	    ll_log (logptr, LLOGTMP, "unusual return: (%s)%s",
		    rp_valstr (thereply.rp_val), thereply.rp_line);
	    break;    /* notify deliver */
    }
    /* tell deliver about all this */
    qu_wrply (&thereply, len);
    
    return(RP_OK);
}

LOCFUN
	sendnotification()
{
    FILE *fp;

    /* send the notification */    

    if (started) /* prime ml_send */
	ml_state = ML_MSG;
    else
        ml_state = ML_FRESH;
    started = FALSE;            /* if sendnotify returns prematurely because
				   an ml_send routine returned NOTOK, we know
				   that ml_state will be ML_FRESH (i.e. submit
				   shut down) because ml_send is very careful */

    if (ml_1adr(NO, YES, supportaddr,
            (rejecting)?"Expired addresses"
      		       :"Soon-to-expire addresses", sender) != OK) {
	ll_log(logptr, LLOGTMP, "Failed to send blockaddr warning to %s",
				sender);
	return;
    }

    if ((fp=fopen(chanptr->ch_confstr+1, "r")) == 0)
    {
        ll_log(logptr, LLOGTMP, "Blockaddr file (%s) not found.",
				chanptr->ch_confstr+1);
	if (ml_txt((rejecting)?"Please resend your message with the new addresses shown below:\n":
   	           	       "Your message has been sent to the new addresses shown below:\n") != OK) {
	    ll_log(logptr, LLOGTMP, "Failed to send blockaddr warning to %s",
				    sender);
	    return;
	}
    }
    else {
        if (ml_file (fp) != OK) {
	    ll_log(logptr, LLOGTMP, "Failed to send blockaddr warning to %s",
				    sender);
	fclose(fp);
	return;
        }
        fclose(fp);
    }

    ml_txt("\n");
    while(notify > 0) {
	if (ml_txt(notifylist[notify]) != OK) {
	    ll_log(logptr, LLOGTMP, "Failed to send blockaddr warning to %s", 
		                    sender);
	    return;
	}
	free(notifylist[notify--]);
    }

    if (ml_end(OK) != OK) {
	ll_log(logptr, LLOGTMP, "Failed to send blockaddr warning to %s",
	                        sender);
	return;
    }
    started=TRUE;
    return;    

}    
