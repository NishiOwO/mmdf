#include "util.h"
#include <sys/file.h>
#ifdef	HAVE_FCNTL_H
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */
/*
 *	Standardized file-locking package  (4.2BSD)
 *
 *	Feb 88	Added support for "a+", "r+" and "w+". Also, improved logging
 *		and added attention to "sentprotect".
 *			Edward C. Bennett <edward@engr.uky.edu>
 *
 *  This version assumes the existence of the 4.2BSD flock() system call.
 */
extern int errno;               /* simulate system error problems */
extern int sentprotect;		/* default file protection */

#ifdef DEBUG
#include "ll_log.h"
extern LLog *logptr;
#endif

LOCVAR	char *NIL = "NIL";

/**/
lk_open(file, access, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
int	access;			/* read-write permissions */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
int	maxtime;		/* maybe break lock after it is this old */
{
	register	int	fd;

#ifdef DEBUG
	ll_log(logptr, LLOGBTR, "lk_open(%s,%d,%s,%s,%d)", file, access,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#endif

	if ((fd = open(file, access | O_NDELAY)) < 0
		|| flock(fd, LOCK_EX|LOCK_NB) < 0) {
#ifdef DEBUG
		ll_err(logptr, LLOGBTR, "open notok (err %d)", errno);
#endif
		if (fd >= 0)
		    (void) close(fd);

		return(NOTOK);
	}
	return(fd);
}

lk_close(fd, file, lockdir, lockfile)
int	fd;
char	*file;			/* -- Ignored -- */
char	*lockdir;		/* -- Ignored -- */
char	*lockfile;		/* -- Ignored -- */
{
	register	int	retval;

#ifdef DEBUG
	ll_log(logptr, LLOGBTR, "lk_close(%d,%s,%s,%s)", fd, file,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif
	if (fd < 0)
		return(OK);

	retval = close(fd);
	return(retval);
}
/**/
FILE *
lk_fopen(file, access, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
char	*access;		/* read-write permissions */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
int	maxtime;		/* maybe break lock after it is this old */
{
	register	int	fd, oflag, plus;
	register	FILE	*fp;

#ifdef DEBUG
	ll_log(logptr, LLOGBTR, "lk_fopen(%s,%s,%s,%s,%d)", file, access,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#endif

	oflag = O_CREAT | O_NDELAY;
	plus = (access[1] == '+');
	switch (access[0]) {
	case 'a':
		oflag |= (plus ? O_RDWR : O_WRONLY) | O_APPEND;
		break;

	case 'r':
		oflag |= (plus ? O_RDWR : O_RDONLY);
		break;

	case 'w':
		oflag |= (plus ? O_RDWR : O_WRONLY) | O_TRUNC;
		break;

	default:
		ll_err(logptr, LLOGFAT, "lk_fopen(): bad mode '%s'\n", access);
		return(NULL);
	}

	if ((fd = open(file, oflag, sentprotect)) < 0 ||
		flock(fd, LOCK_EX|LOCK_NB) < 0) {
#ifdef DEBUG
		ll_err(logptr, LLOGFAT, "open of %s notok (err %d)",
			file, errno);
#endif
		if (fd >= 0)
			(void) close(fd);

		return(NULL);
	}

	if ((fp = fdopen(fd, access)) == NULL) {
		(void) close(fd);
		return((FILE *)NULL);
	}
	return(fp);
}

lk_fclose(fp, file, lockdir, lockfile)
FILE	*fp;
char	*file;			/* --Ignored-- */
char	*lockdir;		/* --Ignored-- */
char	*lockfile;		/* --Ignored-- */
{
#ifdef DEBUG
	ll_log(logptr, LLOGBTR, "lk_fclose(0%o,%s,%s,%s)", fp, file,
		lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif
	switch ((int)fp) {
	case EOF:
	case NULL:
		return(OK);
	}

	return(fclose(fp));
}
