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
 *     This program module was developed as part of the University
 *     of Delaware's Multi-Channel Memo Distribution Facility (MMDF).
 *
 *     This module and its listings may be used freely by United States
 *     federal, state, and local government agencies and by not-for-
 *     profit institutions, after sending written notification to
 *     Professor David J. Farber at the above address.
 *
 *     For-profit institutions may use this module, by arrangement.
 *
 *     Notification must include the name of the acquiring organization,
 *     name and contact information for the person responsible for
 *     maintaining the operating system, name and contact information
 *     for the person responsible for maintaining this program, if other
 *     than the operating system maintainer, and license information if
 *     the program is to be run on a Western Electric Unix(TM) operating
 *     system.
 *     
 *     Documents describing systems using this module must cite its
 *     source.
 *     
 *     Development of this module was supported in part by the
 *     University of Delaware and in part by contracts from the United
 *     States Department of the Army Readiness and Materiel Command and
 *     the General Systems Division of International Business Machines.
 *     
 *     Portions of the MMDF system were built on software originally
 *     developed by The Rand Corporation, under the sponsorship of the
 *     Information Processing Techniques Office of the Defense Advanced
 *     Research Projects Agency.
 *
 *     The above statements must be retained with all copies of this
 *     program and may not be removed without the consent of the
 *     University of Delaware.
 *     
 *
 *     version  -1    David H. Crocker    March  1979
 *     version   0    David H. Crocker    April  1980
 *     version   1    David H. Crocker    May    1981
 *
 *	7/88	#ifndef'ed out code that was removing file locks on SYS5
 *		systems. Edward C. Bennett <edward@engr.uky.edu>
 *	8/98	Made all file access go through stdio.
 *		Centralized flag changing, added error checking.
 *		Avoids GNU LIBC bug and general messiness.
 *		Mike Muuss <Mike@ARL.MIL>
 *
 */
#include "util.h"
#include "mmdf.h"
#include <signal.h>
#include <sys/stat.h>
#include "ch.h"
#include "msg.h"
/*  msg_cite() is defined in adr_queue.h  */
#include "adr_queue.h"
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#else /* HAVE_UNISTD_H */
#  ifdef HAVE_SYS_UNISTD_H
#    include <sys/unistd.h>
#  endif /* HAVE_SYS_UNISTD_H */
#endif /* HAVE_UNISTD_H */

#define OLD_WARN

#ifndef EWOULDBLOCK
#ifdef SYS5
#define EWOULDBLOCK	EACCES
#else
#define EWOULDBLOCK	ETXTBSY /* To handle Berkeley 4.2 and Others */
#endif
#endif

extern	LLog	*logptr;
extern	time_t	time();
extern	char	*supportaddr;
extern	char	*aquedir;
extern	char	*mquedir;
extern	char	*squepref;	/* string to preface sub-queue name	*/
extern	char	*lckdfldir;
extern	int	lk_open();

LOCFUN long	mq_gtnum();	/* reads long ascii # from addr file		*/

LOCVAR	int	mq_fd;		/* read/write file descriptor, saved	*/
LOCVAR	FILE	*mq_fp;		/* read/wr handle, for getting entries	*/
LOCVAR	char	mq_dlm;		/* mq_gtnum's delimeter char		*/
LOCVAR	long	mq_curpos;	/* Current offset of read pointer (rfp)	*/
LOCVAR	long	mq_lststrt;	/* Offset to start of addr list		*/
LOCVAR	long	mq_optstrt;	/* Offset to options			*/
LOCVAR	Msg	*mq_curmsg;	/* SEK handle on current message	*/
/**/

/*
 *			M Q _ C H A N G E F L A G
 *
 * Change flag in address file from 'from' to 'to'.
 * As address files are damaged when lseek fails (either due to GNU LIBC
 * bug, or due to sys-admin editing the files out from under us),
 * employ conservative strategy of verifying that flag character is
 * either 'from' or 'to' already, else abort.
 *
 *  Returns -
 *	-1	failure
 *	0	success
 */
static int
mq_changeflag (pos, from, to)
    long pos;
    int from;
    int to;
{
	char	buf[8];
	int	got;

	buf[0] = '?';
	if( fseek ( mq_fp, pos, 0 ) != 0 || fread ( buf, 1, 1, mq_fp ) != 1 )  {
		ll_log (logptr, LLOGFAT,
			"mq_changeflag (pos=%ld, from=%c, to=%c) fseek/fread error",
			pos, from, to );
		return -1;
	}
	if( (got=buf[0]) != from && got != to )  {
		ll_log (logptr, LLOGFAT,
			"mq_changeflag (pos=%ld, from=%c, to=%c) got=%c, aborting",
			pos, from, to, got );
		return -1;
	}
	buf[0] = (char)to;
	if( fseek ( mq_fp, pos, 0 ) != 0 || fwrite( buf, 1, 1, mq_fp ) != 1 )  {
		/* perror("mq_changeflag: write") */
		ll_log (logptr, LLOGFAT,
			"mq_changeflag (pos=%ld, from=%c, to=%c) fseek/fwrite failure",
			pos, from, to );
		return -1;
	}
	ll_log (logptr, LLOGFTR,
		"mq_changeflag (pos=%ld, from=%c, to=%c) was=%c, OK",
		pos, from, to, got );
	return 0;
}
/**/

mq_rkill (type)			/* give-up access to msg info		*/
    int type;			/* type of ending (currently ignored)	*/
{
#ifdef DEBUG
     ll_log (logptr, LLOGBTR, "mq_rkill (type=%d, mq_fd=%d, mq_fp=%o)",
		type, mq_fd, mq_fp);
#endif

    if (mq_fd != NOTOK)
	lk_close (mq_fd, (char *) 0, lckdfldir, mq_curmsg -> mg_mname);
    if (mq_fp != (FILE *) EOF && mq_fp != (FILE *) NULL)
	fclose (mq_fp);

    mq_fd = NOTOK;
    mq_fp = NULL;
}
/**/

mq_rinit (thechan, themsg, retadr)	/* gain access to info on the msg */
	Chan *thechan;			/* channel points to sub-queue */
	Msg  *themsg;
	char *retadr;			/* return addres */
{
    char msgname[LINESIZE];
    extern int errno;
    register short     len;
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "mq_rinit ()");
#endif

    mq_curmsg = themsg;		/* SEK not message for file locking */

    if (thechan == (Chan *) 0)	/* do the entire queue */
    {
	snprintf (msgname, sizeof(msgname), "%s%s", aquedir, themsg -> mg_mname);
    }
    else
    {
	snprintf (msgname, sizeof(msgname), "%s%s/%s",
		squepref, thechan -> ch_queue, themsg -> mg_mname);
    }

    /* Make stdio FILE capable of reading & writing */
    if ((mq_fd = lk_open (msgname, 2, lckdfldir, themsg -> mg_mname, 20)) < OK ||
	(mq_fp = fdopen (mq_fd, "r+")) < (FILE *)NULL)
    {				/* msg queued in file w/its name */
	switch (errno)
	{
	    case ENOENT:	/* another deliver probably did it */
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "%s no entry", msgname);
		break;
#endif
	    case EWOULDBLOCK:	/* another deliver probably using it */
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "%s busy", msgname);
#endif
		printx ("Message '%s': busy.\n", themsg -> mg_mname);
		break;

	    default:
		printx ("can't open address list '%s'\n", msgname);
		ll_err (logptr, LLOGTMP, "can't open '%s'", msgname);
	}
	mq_rkill (NOTOK);	/* Cleanup open files */
	return (NOTOK);
    }
    mq_lststrt = 0L;
    /*NOSTRICT*/
    themsg -> mg_time = (time_t) mq_gtnum ();
    themsg -> mg_stat = (mq_dlm == ADR_DONE ? ADR_WARNED : 0);
				/* whether warning been sent */
    mq_optstrt = mq_lststrt - 1;	/* Offset to option bits */
    themsg -> mg_stat |= (char) mq_gtnum ();
    if (fgets (retadr, ADDRSIZE, mq_fp) == NULL)
    {
	ll_err (logptr, LLOGTMP, "Can't read %s sender name", themsg -> mg_mname);
	mq_rkill (OK);
	retadr[0] = '\0';	/* eliminate old sender name */
	return (NOTOK);
    }
    len = strlen (retadr);
    mq_lststrt += len;
    if (retadr[len - 1] != '\n') {
	while (fgetc(mq_fp) != '\n')
	{
	    mq_lststrt++;
	    if (feof(mq_fp) || ferror(mq_fp)) {
		ll_err (logptr, LLOGTMP, "Can't read %s sender name", themsg -> mg_mname);
		mq_rkill (OK);
		retadr[0] = '\0';	/* eliminate old sender name */
		return (NOTOK);
	    }
	}
	mq_lststrt++;

	/*  The address was probably too long, substitute orphanage */
	sprintf (retadr, "%s (Orphanage)", supportaddr);
    }
    else
	retadr[len - 1] = '\0';		/* eliminate the newline */

    mq_curpos = mq_lststrt;
#ifdef DEBUG
   ll_log (logptr, LLOGFTR, "name='%s' ret='%s' (fd=%d)",
	       themsg -> mg_mname, retadr, fileno (mq_fp));
#endif
    return (OK);
}
/**/

/*
 *  mq_setpos() repositions the read pointer (mq_fp) but because mq_fp is
 *	just a dup of mq_fd, mq_fd is repositioned as well.  Be careful!
 */
mq_setpos (offset)
    long offset;
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "mq_setpos (%ld)", offset);
#endif
    if (offset == 0L) {
	fseek (mq_fp, mq_lststrt, 0);
    	mq_curpos = mq_lststrt;
    }
    else {
	fseek (mq_fp, offset, 0);
    	mq_curpos = offset;
    }
}

mq_radr (theadr)	/* obtain next address in msg's queue */
    register struct adr_struct *theadr;
{
    char *arglist[20];
    int argc;
    register int len;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "mq_radr ()");
    ll_log (logptr, LLOGBTR, "----> (pos=%d, cur=%ld, lseek=%ld)", 
           mq_curpos, ftell(mq_fp), lseek(mq_fd, 0, SEEK_CUR));
#endif

    theadr -> adr_que = 0;	/* null it to indicate empty entry */
    theadr -> adr_pos = mq_curpos;
    for (;;) {
	if (fgets (theadr->adr_buf, sizeof (theadr->adr_buf), mq_fp) == NULL)
	{
	    if (feof (mq_fp))
	    {
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "end of list");
#endif
		return (DONE);		/* simply an end of file */
	    }
	    ll_err (logptr, LLOGTMP, "Problem reading address");
	    return (NOTOK);
	}
	len = strlen (theadr -> adr_buf);
	if (theadr -> adr_buf[2] != ADR_DONE)
	    break;
	theadr -> adr_pos += len;
    }
    mq_curpos = theadr -> adr_pos + len;
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "adr: '%s'", theadr -> adr_buf);
#endif

    theadr -> adr_buf[len - 1] = '\0';
				/* so entry can be handled as string */
    argc = str2arg (theadr -> adr_buf, 20, arglist, (char *) 0);
    if (argc < 4)
    {
	ll_log (logptr, LLOGTMP, "error with queue address line");
	return (NOTOK);		/* error with address entry */
    }
    theadr -> adr_tmp   = arglist[0][0];
    theadr -> adr_delv  = arglist[1][0];
    theadr -> adr_fail  = arglist[2][0];
    theadr -> adr_que   = arglist[3];
    theadr -> adr_host  = arglist[4];
    theadr -> adr_local = arglist[5];

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "adr parts: '%c' '%c' '%c' '%s' '%s' '%s'",
            theadr -> adr_tmp, theadr -> adr_delv, theadr -> adr_fail,
            theadr -> adr_que, theadr -> adr_host, theadr -> adr_local);
#endif
    return (OK);
}
/**/

/*
 * If the message was successfully delivered, or if a permanent failure
 * resulted, indicate that this address has been handled by changing the
 * delimiter between the descriptor and address info.
 */
mq_wrply (rply, themsg, theadr)	/* modify addr's entry as per reply */
RP_Buf *rply;
Msg *themsg;
struct adr_struct *theadr;
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "mq_wrply (%s)", themsg -> mg_mname);
#endif

    fflush (stdout);
    switch (rp_gval (rply -> rp_val))
    {
	case RP_AOK:		/* name ok; text later */
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR,
		"AOK @ offset(%ld)", (long) (theadr -> adr_pos + ADR_TMOFF));
#endif
	    mq_changeflag ( (long)(theadr->adr_pos + ADR_TMOFF), ADR_CLR, ADR_AOK);
	    break;

	case RP_NDEL:		/* won't be able to deliver */
	case RP_DONE:		/* finished processing this addr */
#ifdef DEBUG
	    ll_log (logptr, LLOGFTR,
		"DONE @ offset(%ld)", (long) (theadr -> adr_pos + ADR_DLOFF));
#endif
	    mq_changeflag ( (long)(theadr->adr_pos + ADR_DLOFF), ADR_MAIL, ADR_DONE);
	    break;

	case RP_NOOP:		/* not processed, this time */
	case RP_NO:		/* no success, this time */
	default: 
	    if( theadr->adr_tmp == ADR_AOK )	/* We need to reset it */
	    {
#ifdef DEBUG
		ll_log (logptr, LLOGFTR,
		"CLR @ offset(%ld)", (long) (theadr -> adr_pos + ADR_TMOFF));
#endif
		mq_changeflag ( (long)(theadr->adr_pos + ADR_TMOFF), ADR_AOK, ADR_CLR);
	    }
    }
    fseek (mq_fp, mq_curpos, 0);	/* Back to where we were */
}
/**/

LOCFUN long
mq_gtnum ()			/* read a long number from adr queue	*/
{
    register long   thenum;
    register char   c;

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "mq_gtnum");
#endif
    for (thenum = 0; isdigit (c = getc (mq_fp));
	    thenum = thenum * 10 + (c - '0'), mq_lststrt++);
    mq_lststrt++;

    mq_dlm = c;			/* note the first non-number read */
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, c < ' ' ? "(%ld)'\\0%o'" : "(%ld)'%c'",
	thenum, c);
#endif
    return (thenum);
}

mq_rwarn (theadr)			/* note that warning has been sent	*/
struct adr_struct *theadr;
{
#ifdef OLD_WARN
#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "WARN @ offset(%ld)", (long) (mq_optstrt));
#endif

    mq_changeflag ( mq_optstrt, ADR_MAIL, ADR_DONE);
    fseek (mq_fp, mq_curpos, 0);	/* Back to where we were */
#else /* OLD_WARN */
#ifdef DEBUG
    ll_log (logptr, LLOGFTR,
            "WARN @ offset(%ld)", (long) (theadr -> adr_pos + ADR_WFOFF));
#endif
    mq_changeflag ( (long)(theadr->adr_pos + ADR_WFOFF), ADR_CLR, ADR_WARN);
    fseek (mq_fp, mq_curpos, 0);	/* Back to where we were */
#endif /* OLD_WARN */
}
