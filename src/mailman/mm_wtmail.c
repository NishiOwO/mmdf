/*
 *	Deliver to the users mailbox
 *	Does most of the interesting, non-mechanical work. It tries hard
 *	not to parse the message unless needed.
 *
 *	Severe hacking done this file reconstructed from bits of old
 *	qu2lo_send.c and a lot of new stuff.
 *
 *	Julian Onions <jpo@cs.nott.ac.uk>	August 1985
 */

#include "util.h"
#include "mmdf.h"
#include "phs.h"
#include "ap.h"
#include "ch.h"
#include <pwd.h>
#include <sys/stat.h>
#include <signal.h>
#ifdef HAVE_SGTTY_H
#  include <sgtty.h>
#endif  /* HAVE_SGTTY_H */
#include "adr_queue.h"
#include "hdr.h"
#include "cnvtdate.h"
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#else /* HAVE_UNISTD_H */
#  ifdef HAVE_SYS_UNISTD_H
#    include <sys/unistd.h>
#  endif /* HAVE_SYS_UNISTD_H */
#endif /* HAVE_UNISTD_H */


extern Chan	*chanptr;
extern LLog	*logptr;

extern int	errno;

extern int	sentprotect;
extern int	flgtrest;
extern int	numfds;
extern char     *qu_msgfile;          /* name of file containing msg text   */
extern long	qu_msglen;
extern jmp_buf  timerest;

extern char *lckdfldir;		/* directory to lock in			*/

LOCVAR int	mbx_fd = -1;	/* handle on recipient mailbox		*/

extern char lo_info[];		/* type of delivery		*/
extern char lo_sender[];	/* return address for message	*/
extern char lo_adr[];		/* destination address from deliver */
extern char lo_replyto[];	/* the reply address		*/
extern char lo_size[];
extern char *lo_parmv[];		/* parameter portion of address */
extern int  lo_parmc;
extern struct passwd *lo_pw;	/* passwd struct for recipient  */

extern	char	*expand();

int	sigpipe;		/* has pipe gone bad? */
RETSIGTYPE	onpipe();		/* catch that pipe write failure */
LOCVAR char mbx_wasclear;	/* this message the first in mbox?	*/

LOCFUN qu2lo_txtcpy(), lo_pwait();
LOCFUN lo_dopipe();
LOCFUN int write_from_line();

/*
 *	Give the user something nice in the environment.
 */
LOCVAR char *envp[] = {
	(char *) 0,		/* HOME=xxx  */
	(char *) 0,		/* SHELL=xxx */
	(char *) 0,		/* USER=xxx  */
	0
};

/*
 * characters that need to be padded to pass to a shell
 */

LOCVAR char *padchar = " \\\"'%$@#?!*|<>()&^~=+-;";


/*  */

mm_slave ()
{
	char	buf[LINESIZE];
	register int	  result;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "mm_slave()");
#endif
	printx ("\n");
    
	/* The production system of delivery attempts */

	/* FIRST Attempt: alias file specified file or pipe delivery */
    return ( lo_dopipe() );
}
/**************** PIPE DELIVERY ROUTINES *************** */

LOCFUN
lo_dopipe()
{
	int	childid;		/* child does the real work */
	int	ofd;			/* what to write on */
    char tmpbuf[BUFSIZ];
	char	buffer[BUFSIZ];
	RETSIGTYPE	(*savepipe)();
	Pip	pipdes;
	int	result;
	int	len;
    int rc;

#ifdef	DEBUG
	ll_log (logptr, LLOGBTR, "lo_dopipe(%s)", chanptr->ch_confstr);
#endif /* DEBUG */

	if( pipe(pipdes.pipcall) == NOTOK)
      return (RP_LIO);

	ll_close(logptr);
	switch (childid = fork ()) {
        case NOTOK:
          (void) close(pipdes.pip.prd);
          (void) close(pipdes.pip.pwrt);
          return (RP_LIO);	/* problem with system */

        case OK:
          /* Child */
#ifdef DEBUG
          ll_log (logptr, LLOGBTR, "lo_dopipe() child (%s %s %s)",
                  chanptr->ch_confstr, lo_parmv[0], lo_parmv[1]);
#endif
          printx ("\tdelivering to pipe '%s %s %s'",
                  chanptr->ch_confstr, lo_parmv[0], lo_parmv[1]);
          fflush (stdout);
          (void) close(0);
          dup (pipdes.pip.prd);

          setupenv();

          execl (chanptr->ch_confstr, "wrapper",
                 lo_parmv[0], lo_parmv[1], (char *)0, envp);
          ll_log( logptr, LLOGFAT, "Can't execl %s (%d)",
                  chanptr->ch_confstr, errno);
          exit (RP_MECH);
	}

	/* Parent */
	/* our job is to pass on the message to the process */
	(void) close (pipdes.pip.prd);
	ofd = pipdes.pip.pwrt;  /* nicer name */
    if(1) write_from_line(ofd);
	qu_rtinit(0L);		/* init the message */
	savepipe = signal(SIGPIPE, onpipe);
	sigpipe = FALSE;
	len = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));
	while (rp_gval(result = qu_rtxt(buffer, &len)) == RP_OK)
	{
      if( sigpipe )	/* childs had enough */
			break;
      if( write(ofd, buffer, len) != len)
      {
        if( errno != EPIPE)
          ll_err (logptr, LLOGTMP, "error on pipe %d",
                  errno);
        break;
      }
      len = sizeof(buffer);
      memset(buffer, 0, sizeof(buffer));
	}
	/*(void)*/rc = close(ofd);	/* finished with pipe */
	signal(SIGPIPE, savepipe);	/* restore the dying signal */
	if(rp_gval(result) != RP_OK && rp_gval(result) != RP_DONE)
		return result;
	return (lo_pwait (childid, chanptr->ch_confstr)); /* .... and wait */
}

LOCFUN
lo_pwait (procid, prog)		/* wait for child to complete	      */
int	procid;			/* id of child process		      */
char	*prog;
{
	int status;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "lo_pwait ()");
#endif

	if (setjmp (timerest) == 0)
	{			/* alarm is in case user's pgm hangs	*/
		flgtrest = TRUE;
		/*NOSTRICT*/
		s_alarm ((unsigned) (qu_msglen / 20) + 300);
		errno = 0;
		status = pgmwait (procid);
		s_alarm (0);
		if (status == NOTOK) {
			/* system killed the process */
			ll_err (logptr, LLOGTMP, "bad return: (%s) %s [wait=NOTOK]",
				lo_pw->pw_name, prog);
			return (RP_LIO);
		}
		ll_log (logptr, LLOGBTR, "(%s) %s [%s]",
			lo_pw->pw_name, prog, rp_valstr (status));
		if( status == 0)	/* A-OK */
		{
			printx (", succeeded\r\n");
			return (RP_MOK);
		}
		else if(status == 1)	/* command not found - ECB */
		{
			printx (", failed (PATH)\r\n");
			return (RP_MECH);
		}
		else if( rp_gbval(status) == RP_BNO)	/* permanent error */
		{
			printx (", failed (perm)\r\n");
			return (RP_NO);
		}
		else	/* temp error */
		{
			printx (", failed (temp)\r\n");
			return (RP_MECH);
		}
		/* NOTREACHED */
	}
	else
	{
		flgtrest = FALSE;
		printx (", user program taking too long");
		ll_log (logptr, LLOGGEN, "user program taking too long - killing");
#ifdef HAVE_KILLPG
		killpg (procid, SIGKILL); /* we're superuser, so always works*/
#else /* HAVE_KILLPG */
		kill (procid, SIGKILL); /* we're superuser, so always works */
#endif /* HAVE_KILLPG */
		return (RP_TIME);
	}
}

/***************  (misc) MISCELLANEOUS ROUTINES  ******************** */

/*
 * setup the environment for the process to run in. This includes :-
 *	Environment variables
 *	file descriptors - close all but fd0
 *	process groups
 *	controlling ttys.
 *	umask
 */

LOCFUN setupenv()
{
	int	fd;
	static char	homestr[64];		/* environment $HOME param */
	static char	shellstr[64];		/* environment $SHELL param */
	static char	userstr[64];		/* environment $USER param */

#ifdef	DEBUG
	ll_log (logptr, LLOGBTR, "setupenv()");
#endif /* DEBUG */
	for (fd = 1; fd < numfds; fd++)
		(void) close (fd);
	fd = open ("/dev/null", 1);	/* stdout */
	dup (fd);			/* stderr */

	umask(077);	/* Restrictive umask */

#ifdef TIOCNOTTY
	if (setjmp (timerest) == 0)
	{
		flgtrest = TRUE;
		s_alarm(15);	/* should be enough */
		if( (fd = open("/dev/tty", 2)) != NOTOK) {
			(void) ioctl(fd, TIOCNOTTY, (char *)0);
			(void) close(fd);
		}
		s_alarm(0);
	}
	else
	{
		flgtrest = FALSE;
		ll_log (logptr, LLOGGEN, "TIOCNOTTY not available");
	}
#endif /* TIOCNOTTY */
#ifdef HAVE_SETPGID
	setpgid (0, getpid());
#else    
#  ifdef HAVE_SETPGRP
#    ifdef __SYSTYPE_BSD
    setpgrp(0, getpid());
#    else /* __SYSTYPE_BSD */
    setpgrp();
#    endif /* __SYSTYPE_BSD */
#  endif /* HAVE_SETPGRP */
#endif /* HAVE_SETPGID */
	snprintf (homestr, sizeof(homestr), "HOME=%s", lo_pw->pw_dir);
	snprintf (shellstr, sizeof(shellstr), "SHELL=%s",
			isstr(lo_pw->pw_shell) ? lo_pw->pw_shell : "/bin/sh");
	snprintf (userstr, sizeof(userstr), "USER=%s", lo_pw->pw_name);
	envp[0] = homestr;
	envp[1] = shellstr;
	envp[2] = userstr;
}

#ifndef	__STDC__
RETSIGTYPE
onpipe()
#else
RETSIGTYPE
onpipe(int i)
#endif
{
	signal(SIGPIPE, onpipe);
	sigpipe = TRUE;
}

char *wday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
char *mon[] = { "Jan", "Feb", "Mar", "Apr", "Mai", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

#include <time.h>

LOCFUN int write_from_line(ofd)
int ofd;
{
  struct tm *t;
  long ltime = 0L;
  int len;
  char buffer[LINESIZE];

  time(&ltime);
  t = localtime(&ltime);
  
  snprintf(buffer, sizeof(buffer), "From %s %s %s %d %02d:%02d:%02d %04d\n",
         lo_sender, wday[t->tm_wday], mon[t->tm_mon], t->tm_mday,
         t->tm_hour, t->tm_min, t->tm_sec, t->tm_year+1900);

  len = strlen(buffer);
  if( write(ofd, buffer, len) != len)
  {
    printx("write_from_line failed\n");
    if( errno != EPIPE)
      ll_err (logptr, LLOGTMP, "error on pipe %d",
              errno);
    return -1;
  }
}
