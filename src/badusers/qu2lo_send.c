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
 *     version  -1    Kai Krueger         July    1992
 *^L
 */
/*                  SEND FROM DELIVER TO LOCAL MACHINE
 *
 *  Jul 81 Dave Crocker     change to have a child handle actual delivery,
 *          + BRL crew      setuid'ing to recipient.
 *  Aug 81 Steve Bellovin   mb_unlock() added to x2y_each
 *  Apr 84 Doug Kingston    Major rewrite.
 *  Aug 84 Julian Onions    Extended the delivery file syntax quite a bit
 *  Feb 85 Julian Onions    Reworked to parse only if really really necessary
 */

#include "util.h"
#include "mmdf.h"
#include "phs.h"
#include "ap.h"
#include "ch.h"
#include <pwd.h>
#include <sys/stat.h>
#include <signal.h>
#ifndef LINUX
#include <sgtty.h>
#endif  /* Not LINUX */
#include "adr_queue.h"
#include "msg.h"

char    ba_info[2 * LINESIZE],
      ba_sender[2 * LINESIZE],
      ba_adr[2 * LINESIZE],
      ba_adr_orig[2 * LINESIZE],
      ba_replyto[2 * LINESIZE],
      ba_size[16],
      *ba_parm;

struct passwd *ba_pw;
extern char *supportaddr;

extern Chan     *chanptr;
extern LLog     *logptr;

extern int      errno;

extern char     *qu_msgfile;          /* name of file containing msg text   */
extern long     qu_msglen;

extern char *index();
extern struct passwd *getpwmid();

LOCFUN qu2ba_each(), ba_verify(), ba_master();

extern FILE *mq_rfp;
LOCVAR struct rp_construct
rp_aend  =
{
      RP_OK, 'b', 'a', 'd', ' ', 'e', 'n', 'd', ' ', 'o', 'f', ' ', 'a',
      'd', 'd', 'r', ' ', 'l', 'i', 's', 't', '\0'
},
rp_hend  =
{
      RP_NOOP, 'e', 'n', 'd', ' ', 'o', 'f', ' ', 'h', 'o', 's', 't', ' ',
      'i', 'g', 'n', 'o', 'r', 'e', 'd', '\0'
};

/*^L*/

qu2ba_send ()                       /* overall mngmt for batch of msgs    */
{
      short     result;

#ifdef DEBUG
      ll_log (logptr, LLOGBTR, "qu2ba_send ()");
#endif

        if (rp_isbad (result = qu_pkinit ()))
              return (result);

      /* While there are messages to process... */
      for(;;){
              result = qu_rinit (ba_info, ba_sender, chanptr -> ch_apout);
              if(rp_gval(result) == RP_NS){
                      qu_rend();
                      continue;
              }
              else if(rp_gval(result) != RP_OK)
                      break;
              phs_note (chanptr, PHS_WRSTRT);

              sprintf (ba_size, "%ld", qu_msglen);
              result = qu2ba_each ();
              qu_rend();
              if (rp_isbad (result))
                      return (result);
      }

      if (rp_gval (result) != RP_DONE)
      {
              ll_log (logptr, LLOGTMP, "not DONE (%s)", rp_valstr (result));
              return (RP_RPLY);         /* catch protocol errors      */
      }

      qu_pkend ();                  /* done getting messages          */
      phs_note (chanptr, PHS_WREND);

      return (result);
}
/*^L*/

LOCFUN
qu2ba_each ()               /* send copy of text per address      */
{
      RP_Buf  replyval;
      short   result;
      char    host[LINESIZE];

#ifdef DEBUG
      ll_log (logptr, LLOGBTR, "qu2ba_each()");
#endif
      /* while there are addessses to process... */
      FOREVER
      {
              result = qu_radr (host, ba_adr);
              if (rp_isbad (result))
                      return (result);      /* get address from Deliver */

              if (rp_gval (result) == RP_HOK)
              {                       /* no-op the sub-list indication */
                      qu_wrply ((RP_Buf *) &rp_hend, rp_conlen (rp_hend));
                      continue;
              }

              if (rp_gval (result) == RP_DONE)
              {
                      qu_wrply ((RP_Buf *) &rp_aend, rp_conlen (rp_aend));
                      return (RP_OK);       /* end of address list */
              }

              replyval.rp_val = ba_verify ();
              if (rp_isbad (replyval.rp_val)) {
                qu_wbrply (&replyval, (sizeof replyval.rp_val), ba_sender, ba_adr);
                strcpy(ba_adr_orig, ba_adr);
                strcpy(ba_adr, "badusers" );
                replyval.rp_val = ba_verify ();
                if (rp_isbad (replyval.rp_val)) continue;
              }

              replyval.rp_val = ba_master ();
              switch (rp_gval (replyval.rp_val)) {
              case RP_LIO:
              case RP_TIME:
                      /* RP_AGN prevents deliver from calling us dead */
                      replyval.rp_val = RP_AGN;
              case RP_LOCK:
              case RP_USER:         /* typicial valid responses */
              case RP_BHST:
              case RP_NOOP:
              case RP_NO:
              case RP_MOK:
              case RP_AOK:
              case RP_OK:
                      break;
              default:              /* not expected so may abort          */
                      if (rp_isgood (replyval.rp_val))
                      {
                              ll_log (logptr, LLOGFAT,
                              "ba_wadr = %s", rp_valstr (replyval.rp_val));
                              replyval.rp_val = RP_RPLY;  /* Conservative */
                      }
              }
              qu_wrply (&replyval, (sizeof replyval.rp_val));
      }
}

LOCFUN
ba_verify ()                    /* send one address spec to local     */
{
      char    mailid[MAILIDSIZ];      /* mailid of recipient */
      register char   *p;
      extern char     *strncpy();
#ifdef DEBUG
      ll_log (logptr, LLOGBTR, "ba_verify()");
#endif
      /* Strip any trailing host strings from address */
      if (p = index (ba_adr, '@'))
              *p = '\0';

      for (p = ba_adr; ; p++) {
              switch (*p) {
              case '|':
              case '/':
              case '=':
              case '\0':
                      ba_parm = p;
                      strncpy (mailid, ba_adr, p - ba_adr);
                      mailid[p - ba_adr] = '\0';
                      goto checkit;
              }
      }

checkit:
      if ((ba_pw = getpwmid (mailid)) == (struct passwd *) NULL) {
              ll_err (logptr, LLOGTMP, "user '%s' unknown", mailid);
              return (RP_USER);     /* can't deliver to non-persons       */
      }
      return (RP_OK);          /* Invalid addresss or real problems */
}

LOCFUN
ba_master()
{
      int   childid;                /* child does the real work           */

      switch (childid = fork ()) {
      case NOTOK:
        ll_log( logptr, LLOGTMP, "Cannot fork()");
        return (RP_LIO);        /* problem with system */

      case OK:
              ll_log( logptr, LLOGTMP, "fork() ok");
/*            ll_close (logptr);      /* since the process is splitting */

#ifdef V4_2BSD
              if (initgroups (ba_pw->pw_name, ba_pw->pw_gid) == NOTOK
                || setgid (ba_pw->pw_gid) == NOTOK
#else
              if (setgid (ba_pw->pw_gid) == NOTOK
#endif /* V4_2BSD */
                || setuid (ba_pw->pw_uid) == NOTOK) {
                      ll_err (logptr, LLOGTMP, "can't set id's (%d,%d)",
                              ba_pw->pw_uid, ba_pw->pw_gid);
                      exit (RP_BHST);
              }

              ll_log( logptr, LLOGTMP, "child uid=%d, gid=%d home=%s", ba_pw->pw_uid,
                     ba_pw->pw_gid, ba_pw->pw_dir);
                  if (chdir (ba_pw->pw_dir) == NOTOK) {
                      /* move out of MMDF queue space       */
                      ll_err (logptr, LLOGTMP, "can't chdir to '%s'",
                              ba_pw->pw_dir);
                      exit (RP_LIO);
              }
              ll_log( logptr, LLOGTMP, "fork() ok");

              exit (ba_slave());

      default:                  /* parent just waits  */
              return (pgmwait (childid));
      }
      /* NOTREACHED */
}
