#include "util.h"
#include <fcntl.h>

/*
 *	Standardized file-locking package (System 5 Rel 2)
 *
 * Apr 88	Added use of sentprotect in open() calls.
 *		Removed use of O_CREAT in lk_open().
 *			Edward C. Bennett <edward@engr.uky.edu>
 *
 * Dec 86	Rewritten to directly use fcntl() rather than lockf().
 *		Added support for "a", "a+", "r+" and "w+".
 *		Adapted by Edward C. Bennett <edward@engr.uky.edu>
 *
 * This version assumes the existence of the System 5 lockf() system call
 * Adapted by Mark Vasoll <vasoll@a.cs.okstate.edu> from the 4.2 BSD version
 */
extern int errno;		/* simulate system error problems */
extern int sentprotect;		/* default file protection */

#ifdef DEBUG
#include "ll_log.h"
extern LLog *logptr;
#endif

LOCVAR	char *NIL = "NIL";

/**/

lk_open (file, access, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
int	access;			/* read-write permissions */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
int	maxtime;		/* maybe break lock after it is this old */
{
	register	int	fd;
	struct		flock	l;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "lk_open (%s,%d,%s,%s,%d)", file, access,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#endif

	l.l_len = 0L;
	l.l_start = 0L;
	l.l_whence = 1;		/* lockf.c did this and it doesn't seem to */
				/* matter since start and len are both zero */

	if (access == O_RDONLY)
		l.l_type = F_RDLCK;
	else
		l.l_type = F_WRLCK;

	if ((fd = open (file, access, sentprotect)) < 0
		|| fcntl (fd, F_SETLK, &l) < 0)
	{
#ifdef DEBUG
		ll_err (logptr, LLOGFAT, "open of %s notok (err %d)",
			file, errno);
#endif
		if (fd >= 0)
			(void) close (fd);
		return (NOTOK);
	}
	return (fd);
}

lk_close (fd, file, lockdir, lockfile)
int	fd;
char	*file;			/* file to be locked */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
{
	register	int	retval;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "lk_close (%d,%s,%s,%s)", fd,
		file ? file : NIL, lockdir ? lockdir : NIL,
		lockfile ? lockfile : NIL);
#endif

	if (fd < 0)
		return (OK);
	retval = close (fd);
	return (retval);
}
/**/
FILE *
lk_fopen (file, access, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
char	*access;		/* read-write permissions */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
int	maxtime;		/* maybe break lock after it is this old */
{
	register	int	fd, oflag, plus;
	register	FILE	*fp;
	struct		flock	l;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "lk_fopen (%s,%s,%s,%s,%d)", file, access,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#endif

	l.l_len = 0L;
	l.l_start = 0L;
	l.l_whence = 1;

	plus = (access[1] == '+');
	oflag = O_CREAT;	/* fopen() creates a non-existent file */
	switch (access[0]) {
	case 'a':
		oflag |= (plus ? O_RDWR : O_WRONLY) | O_APPEND;
		l.l_type = F_WRLCK;
		break;

	case 'r':
		oflag |= (plus ? O_RDWR : O_RDONLY);
		l.l_type = F_RDLCK;
		break;

	case 'w':
		oflag |= (plus ? O_RDWR : O_WRONLY) | O_TRUNC;
		l.l_type = F_WRLCK;
		break;

	default:
		ll_err (logptr, LLOGFAT, "lk_fopen(): bad mode '%s'\n", access);
		return(NULL);
	}

	if ((fd = open(file, oflag, sentprotect)) < 0 || fcntl(fd, F_SETLK, &l) < 0) {
#ifdef DEBUG
		ll_err (logptr, LLOGFAT, "open of %s notok (err %d)",
			file, errno);
#endif
		if (fd >= 0)
			(void) close (fd);
		return (NULL);
	}

	if ((fp = fdopen (fd, access)) == NULL) {
		(void) close (fd);
		return ((FILE *) NULL);
	}
	return (fp);
}

lk_fclose (fp, file, lockdir, lockfile)
FILE	*fp;
char	*file;			/* --Ignored-- */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
{
	register	int	retval;

#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "lk_fclose (%s,%s,%s)", file ? file : NIL,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif

	switch (fp) {
	case EOF:
	case NULL:
		return (OK);
	}
	retval = fclose (fp);
	return (retval);
}
