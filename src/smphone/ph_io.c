#include "util.h"
#include "mmdf.h"
#include "d_returns.h"
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
/*#define RUNALON   */

/*                  Handle telephone-based i/o                          */

extern struct ll_struct   *logptr;
extern int    d_errno;              /* dial package error                 */
extern char *blt ();
extern struct ps_rstruct ps_rp;

/*  NOTE:  load initialization routines from ph_iouser or ph_ioslave    */

/*   ************  (ph_)  PHONE MAIL I/O SUB-MODULE  ****************** */

ps_cmd (cmd)		  /* Send a command */
char    *cmd;
{
    short     retval;

#ifdef DEBUG
    ll_log (logptr, LLOGPTR, "ps_cmd (%s)", cmd);
#endif

    if ( rp_isbad(retval = ph_wrec(cmd, strlen(cmd))) )
        return(retval);

    if (rp_isbad (retval = ps_irdrply ())) {
	return( ps_rp.sm_rval = retval );
    }
    return (RP_OK);

}
/**/

ps_irdrply ()             /* get net reply & stuff into ps_rp   */
{
    static char sep[] = "; ";     /* for sticking multi-lines together  */
    static char sm_rnotext[] = "No reply text given";
    short     len,
	    tmpreply,
            retval;
    char    linebuf[LINESIZE];
    char    tmpmore;
    register char  *linestrt;     /* to bypass bad initial chars in buf */
    register short    i;
    register char   more;	  /* are there continuation lines?      */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ps_irdrply ()");
#endif

newrply: 
    for (more = FALSE, ps_rp.sm_rgot = FALSE, ps_rp.sm_rlen = 0;
	    rp_isgood (retval = ph_rrec (linebuf, &len));)
    {				  /* 1st col in linebuf gets reply code */
	for (linestrt = linebuf;  /* skip leading baddies, probably     */
		len > 0 &&        /*  from a lousy Multics              */
		    (!isascii ((char) *linestrt) ||
			!isdigit ((char) *linestrt));
		linestrt++, len--);

	tmpmore = FALSE;          /* start fresh                        */
	tmpreply = atoi (linestrt);
	blt (linestrt, ps_rp.sm_rstr, 3);	/* Grab reply code	*/
	if ((len -= 3) > 0)
	{
	    linestrt += 3;
#ifdef SPACED_CONTINUE
	    for ( ; len > 0 && isspace (*linestrt); linestrt++, len--);	
                       /* Skip white space in case dash is offset  	*/
#endif SPACED_CONTINUE
	    if (len > 0 && *linestrt == '-')
	    {
		tmpmore = TRUE;
		linestrt++;
		if (--len > 0)
		    for (; len > 0 && isspace (*linestrt); linestrt++, len--);
	    }
	}

	if (more)		  /* save reply value from 1st line     */
        {		          /* we at end of continued reply?      */
	    if (tmpreply != ps_rp.sm_rval || tmpmore)
		continue;
	    more = FALSE;	  /* end of continuation                */
	}
	else			  /* not in continuation state          */
	{
	    ps_rp.sm_rval = tmpreply;
	    more = tmpmore;   /* more lines to follow?              */

	    if (len <= 0)
	    {			  /* fake it, if no text given          */
		blt (sm_rnotext, linestrt = linebuf,
		       (sizeof sm_rnotext) - 1);
		len = (sizeof sm_rnotext) - 1;
	    }
	}

	if ((i = min (len, (LINESIZE - 1) - ps_rp.sm_rlen)) > 0)
	{			  /* if room left, save the human text  */
	    blt (linestrt, &ps_rp.sm_rstr[ps_rp.sm_rlen], i);
	    ps_rp.sm_rlen += i;
	    if (more && ps_rp.sm_rlen < (LINESIZE - 4))
	    {			  /* put a separator between lines      */
		blt (sep, &(ps_rp.sm_rstr[ps_rp.sm_rlen]), (sizeof sep) - 1);
		ps_rp.sm_rlen += (sizeof sep) - 1;
	    }
	}
#ifdef DEBUG
	else
	    ll_log (logptr, LLOGFTR, "skipping");
#endif

	if (!more)
	{
#ifdef DEBUG
	    ll_log (logptr, LLOGBTR, "(%u)%.*s", ps_rp.sm_rval,
		    ps_rp.sm_rlen, ps_rp.sm_rstr);
#endif
	    if (ps_rp.sm_rval < 100)
		goto newrply;     /* skip info messages                 */

	    ps_rp.sm_rstr[ps_rp.sm_rlen] = '\0';  
				  /* keep things null-terminated */

	    ps_rp.sm_rgot = TRUE;
	    return (RP_OK);
	}
    }
    return (retval);		  /* error return                       */
}

/*                READ FROM REMOTE MAIL PROCESS                    */

ph_rrec (linebuf, len)           /* read one "record"                    */
char   *linebuf;		  /* where to stuff the text              */
int      *len;                      /* where to stuff the length count      */
{
    short     retval;
    int       goteot;               /* completed packet                   */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_rrec ()");
#endif

#ifdef RUNALON
    *len = read (0, linebuf, LINESIZE - 1);
    *len -= 1;
#else
    linebuf[0] = '\0';

    if ((*len = d_rcvdata (linebuf, LINESIZE - 1, &goteot)) < 0)
    {
	retval = ph_d2rpval (*len);
	printx("problem reading from remote host (%s)\n", rp_valstr (retval));
	ll_log (logptr, LLOGTMP,
		"ph_rrec:  d_rcvdata err (%s)", rp_valstr (retval));
	return (retval);
    }
#endif

    if (*len == 0)
    {
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "zero len read");
#endif
	return (RP_DONE);
    }
    linebuf[*len] = '\0';	  /* keep things null-terminated        */

#ifdef DEBUG
    if (!goteot)		  /* packet is too long                 */
	ll_log (logptr, LLOGFTR, "*** oversized packet");
#endif
				  /* let is slip, for now               */
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "(%d)'%s'", *len, linebuf);
#endif

    printx ("<-(%s)\n", linebuf);
    fflush (stdout);

    return (RP_OK);
}
/**/

ph_rstm (buffer, len)            /* read buffered block of text        */
char   *buffer;			  /* where to stuff the text            */
int      *len;                      /* where to stuff count               */
{
    static int    goteos;           /* save end-of-stream across calls    */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_rstm ()");
#endif

    if (goteos)			  /* was set automatically by dial      */
    {
#ifdef DEBUG
	ll_log (logptr, LLOGFTR, "returning DONE");
#endif
	goteos = FALSE;
	*buffer = '\0';
	*len = 0;
	return (RP_DONE);
    }

#ifdef RUNALON
    printx ("dm_rstm: ");

    *len = read (0, buffer, *len - 1);
    if (*len == 0)
	goteos = TRUE;
#else
    *len = d_rcvdata (buffer, *len - 1, &goteos);
#endif

    if (*len < 0)
    {
	ll_log (logptr, LLOGGEN, "rdstm: error rcvdata");
	return (ph_d2rpval (*len));
    }
    buffer[*len] = '\0';

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "(%d)'%s'", *len, buffer);
#endif
    return (RP_OK);
}
/*                WRITE TO REMOTE MAIL PROCESS                        */

ph_wrec (linebuf, len)           /* write a record/packet              */
char    linebuf[];		  /* chars to write                     */
int       len;                      /* number of chars to write           */
{
    short     retval;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wrec ()");
    ll_log (logptr, LLOGFTR, "(%d)'%s'", len, linebuf);
#endif

#ifdef RUNALON
    if (linebuf == 0 && len == 0)
    {
	printf ("p_wrec (eof)\n");
	fflush (stdout);
	return (RP_OK);
    }
    if (write (1, linebuf, len) != len)
	return (RP_DHST);
    write (1, "\n", 1);		  /* make it pretty */
    return (RP_OK);
#endif

    if ((retval = d_snstream (linebuf, len)) < D_OK ||
	    (retval = d_sneot ()) < D_OK)
	return (ph_d2rpval (retval));

    printx("->(%s)\n",linebuf);
    fflush(stdout);    

    return (RP_OK);
}
/**/

ph_wstm (buffer, len)            /* write next part of char stream     */
char    buffer[];		  /* chars to write                     */
int       len;                      /* number of chars to write           */
{
    short     retval;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "ph_wstm () (%d)'%s'", len, buffer ? buffer : "");
#endif

#ifdef RUNALON
    retval = write (1, buffer, len);
    if (retval == len)
	return (RP_OK);
    else
	return (RP_NO);
#endif

    if (buffer == 0 && len == 0)
	retval = d_sneot ();     /* flush the buffer                   */
    else
	retval = d_snstream (buffer, len);

    if (retval < D_OK)
	return (ph_d2rpval (retval));
    return (RP_OK);
}
/**/

ph_d2rpval (retval)               /* map d_errno into rp.h value        */
int       retval;
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "(%d) => dial err (%d)", retval, d_errno);
#endif

    switch (d_errno)
    {
	case D_LOGERR: 		  /* probably locked                    */
	    ll_err (logptr, LLOGTMP, "phone log");
	    return (RP_AGN);      /* maybe  transient problem           */

	case D_NOPORT:            /* no port or line available          */
	case D_BUSY: 
	case D_ABAN: 
	    return (RP_AGN);      /* highly transient problem           */

	case D_TIMEOUT:           /* alarm during port read/write call  */
	    return (RP_TIME);     /* same type of code                  */

	case D_RCVQUIT:           /* a QUIT packet has been received    */
	    return (RP_DONE);

	case D_PORTEOF:           /* eof on port                        */
	    return (RP_EOF);

	case D_NOPWR:             /* the dialer has no power            */
	case D_PORTOPEN:          /* error trying to open port          */
	    return (RP_NET);

	case D_PORTRD:            /* error on port read                 */
	case D_PORTWRT:           /* error on port write                */
	    return (RP_NIO);

	case D_NORESP:            /* no response to transmitted packet  */
	    return (RP_DHST);     /* assume the host is messed up       */

	case D_PATHERR:           /* error in path packet               */
	case D_PACKERR:           /* error in packet format             */
	    return (RP_BHST);

	case D_TSOPEN:            /* error opening transcript file      */
	case D_LOCKERR:           /* error opening/creating lock file   */
	    return (RP_LOCK);

	case D_CALLLOG:           /* error opening call log file        */
	    return (RP_FOPN);

	case D_TSWRITE:           /* error writing on transcript file   */
	    return (RP_FIO);

	case D_SYSERR:            /* undistinguished system error       */
	case D_FORKERR:           /* couldn't fork after several tries  */
	case D_PRTSGTTY:          /* error doing stty or gtty on port   */
	    return (RP_LIO);

	case D_SCRERR:            /* error in script file               */
	case D_ACCERR:            /* error in access file               */
	case D_NONUMBS:           /* no numbers given to dialer         */
	case D_NOESC:             /* no esacpe character could be found */
	case D_BADSPD:            /* bad speed designation              */
	    return (RP_PARM);

	case D_INTERR:            /* internal package error             */
	case D_INITERR:           /* trouble initializing               */
	case D_BADDIG:            /* bad digit passed to dialer         */
	    return (RP_NO);

	default: 		  /* punt                               */
	    ll_err (logptr, LLOGTMP,
		    "unclassified category (%d) phone error (%d)",
				retval, d_errno);
	    return (RP_NO);
    }
}

