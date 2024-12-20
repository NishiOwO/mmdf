/***********************************************************************/
/** 
 *
 * $Id: cleanque.c,v 1.18 2003/03/02 21:07:06 krueger Exp $
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/* $Header: /tmp/cvsroot_mmdf/mmdf/devsrc/src/tools/cleanque.c,v 1.18 2003/03/02 21:07:06 krueger Exp $ */
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
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Feature test switches
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
//#define _POSIX_SOURCE 1

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * System headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#include <sys/stat.h>

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Local headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#include "util.h"
#include "mmdf.h"
#include "msg.h"
#include "adr_queue.h"
#include "ch.h"
#include "ml_send.h"

#if HAVE_DIRENT_H  /* XXX rja, krueger */
#  include <dirent.h>
#  define NAMLEN(dirent) strlen((dirent)->d_name)
#  define dirtype dirent
#else /* HAVE_DIRENT_H */
#  define dirtype direct
#  define dirent direct
#  define NAMLEN(dirent) (dirent)->d_namlen
#  if HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  if HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  if HAVE_NDIR_H
#    include <ndir.h>
#  endif
#endif /* HAVE_DIRENT_H */

/*  QUECLEAN:  Clean up the mail queue directories                      */

/*  Jul 80 Dave Crocker     noaddr, correct the 2d multcpy
 *  Aug 80 Dave Crocker     fix orphaned
 *  Aug 80 Dave Crocker     fix name-handling for mclean
 *  Aug 81 Dave Crocker     test for chdir failures
 *  Mar 83 Doug Kingston    modified to use format independent directory
 *			    access routines a la 4.2BSD.  (libndir.a)
 */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Macros
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/*#define RUNALON */

#define MINAGE  (long) (60 * 60 * 4)
				  /* at least four hours old             */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Structures and unions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * File scope Variables (Variables share by several functions in
 *                       the same file )
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
DIR *quep;                     /* for reading directory entries      */


struct stat    testnode;

time_t  curtime;              /* Current time in hours              */
int  	elaphour;	      /* Message waiting time in hours      */

char    aname[LINESIZE];
char	mname[LINESIZE];

int     effecid;                  /* system number of pgm/file's owner  */
int	    callerid;                 /* who invoked me?                    */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External Variables
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern int warntime;    /* hours to wait before notification  */
extern int failtime;    /* hours to wait before returning msg */
extern int errno;
extern LLog msglog;
LLog *logptr = &msglog;

extern char *quedfldir;
extern char *aquedir;
extern char *squepref;
extern char *tquedir;
extern char *mquedir;
extern char *supportaddr;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Extern Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN void aclean_new ();
LOCFUN void dowarn_new ();
LOCFUN ismsg(), msg_dequeue(), mn_mmdf();
LOCFUN void dowarn ();

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 *      M A I N
 * 
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
main (argc, argv)		  /* remove old queue files             */
int       argc;
char   *argv[];
{
  extern char *dupfpath ();
  extern time_t time ();

  mmdf_init (argc, argv);
  nice (-5);                 /* try to run faster, if root           */

  getwho (&callerid, &effecid); /* who am I and who is running me?    */
  mn_mmdf();			/* set up effective and group id's properly */

  if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'w')
	domsg = TRUE;

  time (&curtime);

  if (chdir (quedfldir) < OK || chdir (tquedir) < OK) {
	printx ("couldn't chdir to tquedir\n");
	ll_err (logptr, LLOGFAT, "couldn't chdir tquedir");
	exit (-1);
  }

  tclean ();			  /* clean out temporary files          */

  if (chdir (quedfldir) < OK) {
	printx ("couldn't chdir to quedfldir\n");
	ll_log (logptr, LLOGFAT, "couldn't chdir to quedfldir");
	exit (-1);
  }

  mclean ();			  /* get rid of orphaned text files     */

  if (chdir (quedfldir) < OK) {
	printx ("couldn't chdir to quedfldir\n");
	ll_log (logptr, LLOGFAT, "couldn't chdir to quedfldir");
	exit (-1);
  }

  aclean ();                    /* get rid of old queued messages     */
}

/* ***************  GET RID OF OLD TEMPORARY FILES  ****************  */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
tclean ()			  /* clean out temporary files          */
{
  register struct dirtype *dp;
	
  if ((quep = opendir (".")) == NULL) {
	printx ("couldn't open tquedir\n");
	ll_log (logptr, LLOGFAT, "couldn't open tquedir");
	exit (-1);
  }

  while ((dp = readdir (quep)) != NULL)
	if (ismsg (dp) && minage (dp->d_name)) {
      printx ("removing temp file: %s\n", dp->d_name);
      ll_log (logptr, LLOGGEN, "removing temp file: %s", dp->d_name);

#ifndef RUNALON
      (void)  unlink (dp->d_name);
#endif
	}

  closedir (quep);
}

/* ***************  GET RID OF ORPHANED TEXT FILES  ****************  */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
mclean ()			  /* get rid of orphaned text files     */
{
  register struct dirtype *dp;
	
  if ((quep = opendir (mquedir)) == NULL) {
	printx ("couldn't open quedfldir\n");
	ll_log (logptr, LLOGFAT, "couldn't open quedfldir");
	exit (-1);
  }

  while ((dp = readdir (quep)) != NULL)
	if (ismsg (dp))
      mproc (dp->d_name);

  closedir (quep);
  quep = (DIR *) EOF;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
mproc (filename)                /* check for & remove orphaned msgs   */
char    filename[];
{
  (void) snprintf (mname, sizeof(mname), "%s%s", mquedir, filename);
  (void) snprintf (aname, sizeof(aname), "%s%s", aquedir, filename);

  /* old & no aquedir association       */
  if (minage (mname) && stat (aname, &testnode) == -1) {
	printx ("removing old orphaned text file: %s\n", filename);
	ll_log (logptr, LLOGGEN, "removing text file: %s", filename);

#ifndef RUNALON
	(void)  unlink (mname);
#endif
  }
}

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
minage (filename)                /* is file more than x hours old?      */
char    filename[];
{
  if (stat (filename, &testnode) == -1)
	return (FALSE);           /* doesn't really exist               */

#ifdef RUNALON
  printx ("O? %s\n", filename);
#endif

  return (((curtime - testnode.st_mtime) > MINAGE) ? TRUE : FALSE);
  /* two hours since modified?          */
}

/* **************  GET RID OF MESSAGES IN QUEUE TOO LONG **********  */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
aclean ()
{
  Msg  themsg;
  struct adr_struct theadr;
  Chan *curchan;
  char retadr[LINESIZE];
  register struct dirtype *dp;
  int curwarntime, curfailtime, warning_init=0;

  if ((quep = opendir (aquedir)) == NULL)
    err_abrt (RP_FOPN, "can't open address queue");

  themsg.mg_null = '\0';
  while ((dp = readdir (quep)) != NULL) {
    if (ismsg (dp)) {
      /* get queue entry name (msg name)   */
      (void) strncpy (themsg.mg_mname, dp->d_name, sizeof(themsg.mg_mname));
      if (mq_rinit ((Chan *) 0, &themsg, retadr) != OK)
        continue;
      elaphour = (int) ((curtime - themsg.mg_time) / (time_t) 3600);
#ifdef DEBUG
      ll_log (logptr, LLOGFTR, "%s (%d hrs)", themsg.mg_mname, elaphour);
#endif
      if (elaphour > warntime) { /* message is old                   */
		if (elaphour > failtime) {
          /* old enough to return?            */
          doreturn (&themsg, retadr);
          deque (&themsg);
		} else {
          if (!msg_warned(themsg.mg_stat))
		    dowarn (&themsg, retadr);
	    }
      }
      
      mq_rkill (OK);
	}
  }

  closedir (quep);
  quep = (DIR *) EOF;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   GET RID OF MESSAGES IN QUEUE TOO LONG, channel depend
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN void aclean_new ()
{
  Msg  themsg;
  struct adr_struct theadr;
  Chan *curchan;
  char retadr[LINESIZE];
  register struct dirtype *dp;
  int curwarntime, curfailtime, warning_init=0;

  if ((quep = opendir (aquedir)) == NULL)
    err_abrt (RP_FOPN, "can't open address queue");

  themsg.mg_null = '\0';
  while ((dp = readdir (quep)) != NULL)
	if (ismsg (dp)) {
      /* get queue entry name (msg name)   */
      (void) strncpy (themsg.mg_mname, dp->d_name, sizeof(themsg.mg_mname));
      if (mq_rinit ((Chan *) 0, &themsg, retadr) != OK) continue;

      printx("MSG:%s, %ld, %d\n", themsg.mg_mname,
             themsg.mg_time, themsg.mg_stat);
      /* read controlfile */
      while(mq_radr (&theadr) == OK) {
        curfailtime = failtime;
        curwarntime = warntime;
        curchan = ch_qu2struct(theadr.adr_que);
        if(curchan!=(Chan *)NOTOK) {
          printx("%s: (%c)%s, %d %d\n", curchan->ch_name, theadr.adr_delv,
                 theadr.adr_que, curchan->ch_warntime, curchan->ch_failtime);
          if(curchan->ch_failtime>=0) curfailtime = curchan->ch_failtime;
          if(curchan->ch_warntime>=0) curwarntime = curchan->ch_warntime;
        } else
          printx("(%c)%s\n", theadr.adr_delv, theadr.adr_que);

        elaphour = (int) ((curtime - themsg.mg_time) / (time_t) 3600);
#ifdef DEBUG
        ll_log (logptr, LLOGFTR, "%s (%d hrs)", themsg.mg_mname, elaphour);
#endif
        printx("=>%d>%d\n", elaphour, curwarntime);
        if (elaphour > curwarntime) { /* message is old */
          printx("Warntime/Failtime exceeded\n");
          if(!warning_init && theadr.adr_fail==ADR_CLR) {
            rtn_warn_init(&themsg, retadr);
            warning_init=1;
          }
          if (!msg_warned(themsg.mg_stat) && theadr.adr_fail==ADR_CLR)
            dowarn_new (&themsg, &theadr, retadr, curfailtime,
                        (elaphour > curfailtime));
        }
      }
      if(warning_init) ml_end(OK);
      warning_init=0;
      mq_rkill (OK);
    }
  closedir (quep);
  quep = (DIR *) EOF;
}

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN ismsg (theentry)         /* a processable message?             */
register struct dirtype *theentry;
{
#ifdef DEBUG
  ll_log (logptr, LLOGFTR, "ovr_ismsg (name='%s')", theentry -> d_name);
#endif

  /*  valid message:  entry allocated & name begins with "msg." */

  return (( (NAMLEN(theentry) <= MSGNSIZE)
            && equal (theentry -> d_name, "msg.", 4)) ? TRUE : FALSE);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
deque (themsg)
Msg *themsg;
{
  struct adr_struct theadr;
  char curque[ADDRSIZE];

#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "deque");
#endif

  curque[0] = '\0';   /* no queue in effect */

  mq_setpos (0L);	/* start at the beginning of the queue */
  while (mq_radr (&theadr) == OK) {
#ifdef DEBUG
	ll_log (logptr, LLOGFTR,
		    "(%c)%s:%s", theadr.adr_delv, theadr.adr_que, curque);
#endif
    /*
     *  We have to dequeue from every queue since the return
     *  function already has marked all the addresses as "DONE".
     *  The msg_dequeue function was modified to ignore ENOENT
     *  so if the file is already gone, it is not an error.
     */
	if (!lexequ (curque, theadr.adr_que)) {
      (void) strncpy (curque, theadr.adr_que, sizeof(curque));
      msg_dequeue (theadr.adr_que, themsg);
	}
  }
  msg_dequeue ((char *) 0, themsg);
}

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN msg_dequeue (theque, themsg) /* remove message from queue          */
char *theque;
Msg *themsg;
{
  char thename[LINESIZE];
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "msg_deque (%s,%s)", 
          (theque == (char *) 0) ? "(Base entry)" : theque,
          themsg -> mg_mname);
#endif
#ifdef RUNALON
  return;
#endif
  
  if (theque == (char *) 0)
    (void) snprintf (thename, sizeof(thename), "%s%s", aquedir,
                     themsg -> mg_mname);
  else
	(void) snprintf (thename, sizeof(thename), "%s%s/%s",
                     squepref, theque, themsg -> mg_mname);

  if (unlink (thename) < OK && errno != ENOENT) {
    /* this is real queue handle  */
    ll_err (logptr, LLOGTMP, "Problem unlinking '%s' address: %s",
		    themsg -> mg_mname, thename);
  }

  /* get rid of ALL the message */
  if (theque == (char *) 0) {
	(void) snprintf (thename, sizeof(thename), "%s%s", mquedir,
                     themsg -> mg_mname);
	if (unlink (thename) < OK) /* the text is just "baggage"         */
      ll_err (logptr, LLOGTMP, "Problem unlinking %s text: '%s'",
              themsg -> mg_mname, thename);
  }
}

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   send notification about warn-/fail-time exceeded
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN void dowarn_new (themsg, theadr, retadr, curfailtime, failed)
Msg *themsg;
struct adr_struct *theadr;
char retadr[];
int curfailtime;
int failed;
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "dowarn_new (%s, %s)", themsg -> mg_mname, retadr);
#endif

  printx ("%s:  delivery overdue for %s; ", themsg -> mg_mname,
          theadr->adr_local);
  (void) fflush (stdout);

  if (msg_nowarn (themsg -> mg_stat)) {
    printx ("warning not wanted for %s\n", theadr->adr_local);
  }
  else {
    if (rtn_warn_per_adr (themsg, theadr, retadr, curfailtime, failed) == OK)
    {                     /* flag as already warned               */
      printx ("warning sent\n");
      ll_log (logptr, LLOGGEN, "warn *** Time warning (%s, %s)",
              themsg -> mg_mname, retadr);
    } else {
      printx ("couldn't send warning\n");
      ll_err (logptr, LLOGTMP, "warn *** Couldn't time warn (%s, %s)",
              themsg -> mg_mname, retadr);
    }
  }
  
  (void) fflush (stdout);

  mq_rwarn (theadr);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN void dowarn (themsg, retadr)
Msg *themsg;
char retadr[];
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "dowarn (%s, %s)", themsg -> mg_mname, retadr);
#endif

  printx ("%s:  delivery overdue; ", themsg -> mg_mname);
  (void) fflush (stdout);

  if (msg_nowarn (themsg -> mg_stat)) {
    printx ("warning not wanted\n");
  } else {
    /* flag as already warned               */
    if (rtn_warn (themsg, retadr) == OK) {
      printx ("warning sent\n");
      ll_log (logptr, LLOGGEN, "warn *** Time warning (%s, %s)",
              themsg -> mg_mname, retadr);
    } else {
      printx ("couldn't send warning\n");
      ll_err (logptr, LLOGTMP, "warn *** Couldn't time warn (%s, %s)",
              themsg -> mg_mname, retadr);
    }
  }

  (void) fflush (stdout);

  mq_rwarn ();
}

#ifdef NEW_CLEAN
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
doreturn (themsg, theadr, retadr)
Msg *themsg;
struct adr_struct *theadr;
char retadr[];
#else
doreturn (themsg, retadr)
Msg *themsg;
char retadr[];
#endif
{
#ifdef DEBUG
  ll_log (logptr, LLOGBTR, "doreturn (%s, %s)", themsg -> mg_mname, retadr);
#endif

  if(themsg==NULL) return;
  printx ("%s:  not delivered in time; waiting %d hrs;",
          themsg -> mg_mname, elaphour);
  (void) fflush (stdout);
  if (msg_noret (themsg -> mg_stat)) {
	printx (" error return not wanted\n");
  } else {
    /* dequeue if couldn't notify   */
    if (rtn_time (themsg, retadr) == OK) {
      printx (" returned\n");
      (void) fflush (stdout);
      ll_log (logptr, LLOGTMP, "ret *** Timeout return (%s, %s)",
              themsg -> mg_mname, retadr);
    } else {
      /* dequeue if couldn't notify   */
      char orphanage[ADDRSIZE];

      (void) snprintf (orphanage, sizeof(orphanage), "Orphanage <%s>",
                       supportaddr);
      printx (" couldn't return,\ntrying orphanage...");
      (void) fflush (stdout);
      if (rtn_time (themsg, orphanage) == OK) {
	    printx (" returned to orphanage.\n");
	    (void) fflush (stdout);
      } else {
	    ll_err (logptr, LLOGTMP, "ret *** Timeout couldn't return (%s,%s)",
                themsg -> mg_mname, retadr);
	    printx (" returned to dead_letter.\n");
	    dead_letter (themsg->mg_mname, "Timeout on delivery");
      }
    }
  }
}

/**/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * @ingroup xyz
 * @param   
 * @return  0 on success, -1 on failure
 * @brief   
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN mn_mmdf ()		  /* setuid to mmdf: bypass being root  */
{				  /* get sys id for mmdf; setuid to it  */
  extern char *pathsubmit;     /* submit command file name           */
  extern char *cmddfldir;      /* directory w/mmdf commands          */
  char    temppath[LINESIZE];
  struct stat    statbuf;

#ifdef DEBUG
  ll_log (logptr, LLOGPTR, "mn_mmdf(); effec==%d",
          effecid );
#endif

  /*  the following is a little strange, doing a stat on the object
   *  file, because setuid-on-execute does not work when the caller
   *  is root, as will happen when this is started by the rc file.
   *  hence, the effective id, from a getuid, will show root & not mmdf.
   *
   *  the goal is to have this process be name-independent of the caller,
   *  so that returned mail comes from mmdf and not the invoker.
   *
   *  in pickup mode, the id of the caller has to be retained, since
   *  pobox channels use that to determine access rights to mail.
   *
   *  All sets gid to mmdf's gid  --  <DPK@BRL>
   */

  if (effecid == 0) {
	getfpath (pathsubmit, cmddfldir, temppath);

	if (stat (temppath, &statbuf) < OK)
      err_abrt (RP_LIO, "Unable to stat %s", temppath);
				  /* use "submit" to get mmdf id        */

	if (setgid (statbuf.st_gid) == NOTOK)
      err_abrt (RP_LIO, "Can't setgid to mmdf (%d)", statbuf.st_gid);
	if (setuid (statbuf.st_uid) == NOTOK)
      err_abrt (RP_LIO, "Can't setuid to mmdf (%d)", statbuf.st_uid);

	effecid = statbuf.st_uid; /* mostly needed for return mail      */
  }
}
