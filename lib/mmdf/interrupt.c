#include "util.h"
#include "mmdf.h"

/*  INTERRUPT TRAP SETTING AND CATCHING
 *
 *  Aug, 81 Dave Crocker    major addition to timeout, for conditionally
 *                          using timerest only when non-zero.
 */

#include <signal.h>

extern struct ll_struct   *logptr;

/* *******************  SOFTWARE INTERRUPT TRAPS  ********************* */

#ifndef DEBUG
#ifndef	__STDC__
RETSIGTYPE
sig10 ()			  /* signal 10 interrupts to here       */
#else
RETSIGTYPE
sig10 (int i)			  /* signal 10 interrupts to here       */
#endif
{
    signal (SIGBUS, SIG_DFL);
    ll_err (logptr, LLOGFAT, "Dying on sig10: bus error");
    sigabort ("sig10");
}

#ifndef	__STDC__
RETSIGTYPE
sig12 ()			  /* signal 12 interrupts to here       */
#else
RETSIGTYPE
sig12 (int i)			  /* signal 12 interrupts to here       */
#endif
{
    signal (SIGSYS, SIG_DFL);
    ll_err (logptr, LLOGFAT, "Dying on sig12: bad sys call arg");
    sigabort ("sig12");
}
#endif /* DEBUG */

#ifndef	__STDC__
RETSIGTYPE
sig13 ()			  /* signal 13 interrupts to here       */
#else
RETSIGTYPE
sig13 (int i)			  /* signal 13 interrupts to here       */
#endif
{
    signal (SIGPIPE, SIG_DFL);
    ll_err (logptr, LLOGFAT, "Dying on sig13: pipe write w/no reader");
    sigabort ("sig13");
}
/**/

siginit ()			  /* setup interrupt locations          */
{
    extern RETSIGTYPE timeout();

#ifndef DEBUG
    signal (SIGBUS, sig10);
    signal (SIGSYS, sig12);
#endif /* DEBUG */
    signal (SIGPIPE, sig13);
    signal (SIGALRM, timeout);    /* in case alarm is set & fires       */
}


/* ***********************  TIME-OUT TRAP  **************************** */

jmp_buf timerest;                 /* where to restore to, from timeout  */
				  /* not external, if not used outside  */
				  /* otherwise, set by return routine   */
int     flgtrest;                 /* TRUE, if timerest has been set     */

#ifndef	__STDC__
RETSIGTYPE
timeout ()			  /* alarm timeouts/interrupts to here  */
#else
RETSIGTYPE
timeout (int i)			  /* alarm timeouts/interrupts to here  */
#endif
{
    extern int errno;

    signal (SIGALRM, timeout);    /* reenable interrupt for here        */

    if (flgtrest) {
    	errno = EINTR;
    	flgtrest = 0;
	longjmp (timerest, TRUE);
    }

    ll_log (logptr, LLOGFAT, "ALARM signal");
    return;                       /* probably generate an i/o error     */
}

s_alarm(n)
unsigned int n;
{
/*  extern RETSIGTYPE timeout(); */

    flgtrest = ((n == 0) ? 0 : 1);
/* for masscomp til bugfix */
    if (n>0)
	signal(SIGALRM, timeout);
    alarm(n);
}

/* ARGSUSED */

sigabort (thesig)
    char thesig[];
{
    signal (SIGHUP, SIG_DFL);
    signal (SIGINT, SIG_DFL);
    signal (SIGILL, SIG_DFL);
    signal (SIGBUS, SIG_DFL);
    signal (SIGSYS, SIG_DFL);
    signal (SIGPIPE, SIG_DFL);
    signal (SIGALRM, SIG_DFL);

#ifdef DEBUG
    abort ();
#endif

    exit (NOTOK);
    /* NOTREACHED */
}
