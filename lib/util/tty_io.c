#include "util.h"
#include <pwd.h>
#include <sys/stat.h>
#include <utmp.h>

extern int errno;
extern jmp_buf timerest;
extern char *blt();

LOCFUN tty_open();

FILE *tty_fp;                     /* user sends tty output to this      */

/*  ****************  (tty_) ACCESS USERS' TTYS  ********************** */

tty_init ()                       /* setup for ttyecipient's tty        */
{
    setutmp ();                   /* get the utmp file                  */
    return (OK);
}

tty_end ()                       /* done with tty access               */
{
    endutmp ();                   /* done with utmp package             */
    return (OK);
}

tty_find (usrname)                /* find & open recipient's tty        */
char    usrname[];
{
    extern struct utmp *getutnam ();
    static char tty_num[25] = "/dev/";
				  /* number of tty receiver is on       */
				  /* name is filled in from getutnam    */
    struct utmp *utptr;

retry:
    if ((utptr = getutnam (usrname)) == 0)
    {
	errno = ENOENT;           /* not logged in                      */
	return (NOTOK);
    }
#ifdef V4_2BSD
    /*
     * do not return the ttyname when it is a pty and not used for a
     * real login session (like windows)
     */
#ifndef nonuser
#define nonuser(ut)	((ut).ut_host[0] == 0 && \
	strncmp((ut).ut_line, "tty", 3) == 0 && ((ut).ut_line[3] == 'p' \
	|| (ut).ut_line[3] == 'q' || (ut).ut_line[3] == 'r'))
#endif
    if (nonuser(*utptr))
	goto retry;
#endif
    strncpy (&tty_num[5], utptr->ut_line, 10);

    return (tty_open (tty_num));
}
/**/

LOCFUN
	tty_open (tty_num)        /* open the named tty                 */
char tty_num[];
{
    struct stat statbuf;
    int tty_fd;

    if (setjmp (timerest))   /* signal, for timerest, must be      */
    {                             /*    set by user program             */
	errno = EBUSY;            /* allow a later try                  */
	return (NOTOK);           /* timeout during tty open            */
    }

/*
 * Vanilla 4.3BSD uses setgid-to-group-"tty" programs to do secure
 * writing on peoples' terminals.
 *
 * Oh, for #elif's in /lib/cpp.
 * -- David Herron <david@ms.uky.csnet>, 18-Mar-87
 */
#if !defined(HAVE_SECURETTY)
    if (stat (tty_num, &statbuf) < OK)
	return (NOTOK);

    if (!(statbuf.st_mode & (S_IWRITE >> 6)))
#else
#ifdef HAVE_SECURETTY
    if (access (tty_num, 01) != 0)
#endif /* HAVE_SECURETTY */
#if !defined(HAVE_SECURETTY)
    if (stat (tty_num, &statbuf) < OK)
	return (NOTOK);

    if (!(statbuf.st_mode & (S_IWRITE >> 3)))
#endif
#endif
    {                             /* check write(I) permissions      */
	errno = EBUSY;
	return (NOTOK);
    }

    s_alarm (15);
    tty_fd = open (tty_num, 1);
    s_alarm (0);

    if (tty_fd < 0)
	return (NOTOK);

    tty_fp = fdopen (tty_fd, "w");
    return (OK);
}


tty_close ()			  /* done writing to tty                */
{
    if (ferror (tty_fp) || fclose (tty_fp) == EOF)
	return (NOTOK);
    return (OK);
}
