#include "util.h"
#include "mmdf.h"
#include "ap.h"
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

/*                  ADDRESS BLOCKING CHANNEL                             */
/*#define RUNALON*/
#include <signal.h>
#include "phs.h"
#include "ch.h"

extern LLog *logptr;

Chan *chanptr;

/*      MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN     */

main (argc, argv)
int       argc;
char   *argv[];
{
  char *dupfpath ();
  short retval;
  int   realid;
  int   effecid;
  
  mmdf_init (argv[0]);
  siginit ();
  signal (SIGINT, SIG_IGN);     /* always ignore interrupts             */
  
  if ((chanptr = ch_nm2struct (argv[0])) == (Chan *) NOTOK)
  {
	ll_log (logptr, LLOGTMP, "mailman (%s) unknown channel", argv[0]);
	exit (RP_PARM);
  }

  getwho (&realid, &effecid);
  if (effecid != 0)              /* MUST run as superuser              */
	err_abrt (RP_BHST, "not running as superuser");

  retval = ch_mailman (argc, argv);
  ll_close (logptr);
  exit (retval);
}
/***************  (ch_) ADDRESS BLOCKING DELIVERY  ******************** */

ch_mailman (argc, argv)              /* send to internet                   */
int       argc;
char   *argv[];
{
  ch_llinit (chanptr);
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "ch_mailman()");
#endif

  if ((chanptr->ch_apout & AP_822) != AP_822) {
	ll_log (logptr, LLOGTMP, "mailman channel must have ap=822");
	ll_log (logptr, LLOGTMP, "Fix and then edit queued messages");
	return(RP_NO);
  }

  if( chanptr->ch_confstr == NULL) {
    ll_log(logptr, LLOGFST, "confstr not set");
    return(RP_NO);
  }
  
  if (rp_isbad (qu_init (argc, argv)))
	return (RP_NO);		  /* problem setting-up for deliver     */
    
  phs_note (chanptr, PHS_WRSTRT);

  if (rp_isbad (qu2mm_send ()))
	return (RP_NO);		  /* send the batch of outgoing mail    */
  
  qu_end (OK);                  /* done with Deliver function         */
  phs_note (chanptr, PHS_WREND);
    
  return (RP_OK);		  /* NORMAL RETURN                      */
}

/**/

/* VARARGS2 */
err_abrt (code, fmt, b, c, d)     /* terminate ourself                  */
short     code;
char    fmt[],
        b[],
        c[],
        d[];
{
#ifdef DEBUG
  char linebuf[LINESIZE];
#endif

  qu_end (NOTOK);

#ifdef DEBUG
  if (rp_gbval (code) == RP_BNO || logptr -> ll_level >= LLOGBTR)
  {                         /* don't worry about minor stuff      */
	snprintf (linebuf, sizeof(linebuf), "%s%s", "err [ ABEND (%s) ]\t", fmt);
	ll_log (logptr, LLOGFAT, linebuf, rp_valstr (code), b, c, d);
	abort ();
  }
#endif
  ll_close (logptr);            /* in case of cycling, close neatly   */
  exit (code);
}
