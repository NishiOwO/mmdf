#include "util.h"

#if defined(HAVE_LOCK_FILES) || \
    (!defined(HAVE_FLOCK) && !defined(HAVE_F_SETLK)) || \
    (defined(HAVE_FLOCK) && defined(HAVE_F_SETLK))
#  ifndef HAVE_LOCK_FILES
#    define HAVE_LOCK_FILES
#  endif
#undef HAVE_FLOCK
#undef HAVE_F_SETLK
#endif

#ifdef HAVE_LOCK_FILES
#  include <sys/stat.h>

/*
 *	Standardized file-locking package  (using links)
 *
 *  This version presumes no o/s support of locking, with
 *  link(2) being the only available atomic o/s operation.
 */

extern int errno;               /* simulate system error problems */
#  ifdef DEBUG
#  include "ll_log.h"
extern LLog *logptr;
#  endif

extern	char *lckdfldir;

LOCVAR	int breaktime = 120;   /* amount to sleep after breaking lock */
LOCVAR	char *NIL = "NIL";

LOCFUN lk_unlock ();
LOCFUN lk_name ();
LOCFUN lk_test ();
/**/

LOCFUN
lk_lock (fd, file, lockdir, lockfile, maxtime)
int	fd;			/* file to be locked */
char	*file;			/* file to be locked */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
int	maxtime;		/* maybe break lock after it is this old */
{
    int sleepval;
    int retval;
    int tries;
    char lkname[100];           /* full name of locking file */
    char tmpname[100];          /* name of tmp file to link from */

#  ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_lock (%s,%s,%s,%d)", file,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#  endif

    if (lk_name (lkname, fd, file, lockdir, lockfile) < OK)
	return (NOTOK);

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "lkname '%s'", lkname);
#endif

    for (tries = 0; tries < 2; tries++)
	switch (lk_test (file, lkname, maxtime))
	{
	    case OK:                /* lock exists & is ok */
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "already locked & ok");
#endif
		errno = ETXTBSY;
		return (NOTOK);

	    case NOTOK:             /* lock exists and is old */
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "old lock");
#endif
		if (lk_unlock (fd, file, lockdir, lockfile) == NOTOK)
		    return (NOTOK); /* hmmmmmm */

/*
 *  the amount of time to sleep is at least breaktime.
 *  the random number generation is used to try to separate
 *  two processes that may be close to syncrony in checking the lock.
 */
		srand (getpid ());
		sleepval = breaktime + (rand ()%100);
#ifdef DEBUG
		ll_log (logptr, LLOGFTR, "sleeping (%d)", sleepval);
#endif
		sleep (sleepval);
		continue;

	    default:
		goto lockit;
	}

/* below here, try to lock the guy */

lockit:
    getfpath (",tmp.XXXXXX", lockdir ? lockdir : lckdfldir, tmpname);
    mktemp (tmpname);           /* have to have something to link from */

    if (close (creat (tmpname, 0666)) < OK)
    {
#ifdef DEBUG
	ll_err (logptr, LLOGFTR, "problem w/lock base file '%s'", tmpname);
#endif
	(void) unlink (tmpname);
	return (NOTOK);
    }

    retval = link (tmpname, lkname);

#ifdef DEBUG
    if (retval < 0)
	ll_err (logptr, LLOGFTR,
		    "problem linking '%s' to '%s'", tmpname, lkname);
    else
	ll_log (logptr, LLOGFTR,
		    "linked '%s' to '%s'", tmpname, lkname);
#endif

    (void) unlink (tmpname);

    return (retval);
}
/**/

LOCFUN
lk_unlock (fd, file, lockdir, lockfile)
int	fd;		/* file to be locked */
char	*file;		/* file to be locked */
char	*lockdir;	/* directory to put parallel file into */
char	*lockfile;	/* file to lock against */
{
    int retval;
    char lkname[100];           /* full name of locking file */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_unlock (%s,%s,%s)", file,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif

    if (lk_name (lkname, fd, file, lockdir, lockfile) < OK)
	return (NOTOK);

    retval = unlink (lkname);

#ifdef DEBUG
    if (retval < 0)
	ll_err (logptr, LLOGFTR, "problem unlinking '%s'", lkname);
#endif

    return (retval);
}

/**/

LOCFUN
lk_name (lkname, fd, file, lockdir, lockfile)
char	*lkname;		/* put full name of file to use as lock */
int	fd;			/* file to be locked */
char	*file;			/* resource to be locked */
char	*lockdir;		/* dir containing locking file, if any */
char	*lockfile;		/* file to use ask lock */
{
    struct stat statbuf;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_name (%s,%s,%s)", file,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif

    if (lockdir == (char *) 0)
	lockdir = lckdfldir;

    if (lockfile && *lockfile )
	getfpath (lockfile, lockdir, lkname);
    else {                      /* make out own file name */
	if (fstat (fd, &statbuf) < OK) /* no way to create a fixed name */
	    return (NOTOK);
	sprintf (lkname, "%s/LCK%05d.%05d",
			lockdir, statbuf.st_dev, statbuf.st_ino);
    }

    return (OK);
}
/**/

LOCFUN
lk_test (file, lkfile, maxtime)
char	*file;
char	*lkfile;
int	maxtime;
{
    time_t curtime;
    struct stat statbuf;

    time (&curtime);

    if (stat (lkfile, &statbuf) < OK)
    {
#ifdef DEBUG
	ll_err (logptr, LLOGBTR, "lk_test couldn't stat lockfile '%s'", lkfile);
#endif
	return (DONE);          /* assume that it does not exist */
    }
    if (maxtime <= 0)
	return (OK);            /* no time limit */

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lockfile (%ld:%ld) minutes",
		(((curtime - statbuf.st_mtime)+59L)/60L), (long) (maxtime));
#endif
    if ((((curtime - statbuf.st_mtime)+59L)/60L) > (long) (maxtime))
    {                           /* well, the lock is too old */
	if (file != (char *) 0) /* is the resource inactive, also? */
	    if (stat (file, &statbuf) >= OK)
	    {
#ifdef DEBUG
		ll_log (logptr, LLOGBTR, "resource access (%ld:%ld) minutes",
		    (((curtime - statbuf.st_mtime)+59L)/60L), (long) (maxtime));
#endif
		if ((((curtime - statbuf.st_atime)+59L)/60L) <= (long) (maxtime))
		    return (OK); /* let them go */
	    }
	return (NOTOK);
    }

    return (OK);        /* lock still has time */
}
/**/

LOCFUN
lk_creat (file, mode, lockdir, lockfile, maxtime)
char	*file;			/* file to be created */
int	mode;			/* creation mode */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
int	maxtime;		/* maybe break lock after it is this old */
{
    register fd;
    register char *p;
    char tempname[100];

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_creat (%s,0%o,%s,%s)", file, mode,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif

retry:
    if ((fd = lk_open (file, 1, lockdir, lockfile, maxtime)) >= 0) {
	close (creat (file, mode));
	return (fd);
    }
    if (errno != ENOENT)
	return (NOTOK);
    /* file didn't exist */
    if (p = strrchr (file, '/')) {
	*p = '\0';
	(void) strncpy (tempname, file, sizeof(tempname));
	*p = '/';
	(void) strcat (tempname, "/");
    } else
	tempname[0] = '\0';
    (void) strcat (tempname, ",tmpXXXXXX");
    fd = creat (tempname, mode);
    if (fd < 0)
	return (NOTOK);
    if (lk_lock (fd, tempname, lockdir, lockfile, maxtime) < 0) {
	unlink (tempname);
	close (fd);
	return (NOTOK);
    }
    if (link (tempname, file) < 0) {
	lk_unlock (fd, tempname, lockdir, lockfile);
	close (fd);
	unlink (tempname);
	if (errno == EEXIST)
	    goto retry;
	return (NOTOK);
    }
    unlink (tempname);
    return (fd);
}

lk_open (file, access, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
int	access;			/* read-write permissions */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
int	maxtime;		/* maybe break lock after it is this old */
{
    register fd;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_open (%s,%d,%s,%s,%d)", file, access,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL, maxtime);
#endif

    if ((fd = open (file, access)) < 0) {
#ifdef DEBUG
    	ll_err (logptr, LLOGBTR, "open not ok (errno %d)", errno);
#endif
	return (NOTOK);
    }
    if (lk_lock (fd, file, lockdir, lockfile, maxtime) < 0) {
#ifdef DEBUG
    	ll_err (logptr, LLOGBTR, "open lock not ok");
#endif
	close (fd);
	return (NOTOK);
    }
    return (fd);
}

lk_close (fd, file, lockdir, lockfile)
int fd;
char	*file;			/* file to be locked */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_close (%d,%s,%s,%s)", fd, file,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif
    if (fd < 0)
	return (OK);
    lk_unlock (fd, file, lockdir, lockfile);
    return (close (fd));
}

FILE *
lk_fopen (file, mode, lockdir, lockfile, maxtime)
char	*file;			/* file to be locked */
char	*mode;			/* read-write permissions */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
int	maxtime;		/* maybe break lock after it is this old */
{
    register fd;
    register FILE *f;

#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_fopen (%s,%s,%s,%s)", file, mode,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif

    if ((fd = open (file, 0)) < 0 && (fd = open (file, 1)) < 0) {
	if (*mode != 'a' && *mode != 'w')
	    return (NULL);
	fd = lk_creat (file, 0666, lockdir, lockfile, maxtime);
	if (fd < 0)
	    return (NULL);
    } else
	if (lk_lock (fd, file, lockdir, lockfile, maxtime) < 0) {
	    close (fd);
	    return (NULL);
	}
    /* file is opened and locked; now fopen it */
    if ((f = fopen (file, mode)) == 0) {
	lk_unlock (fd, file, lockfile, lockdir);
	close (fd);
	return (NULL);
    }
    close (fd);
    return (f);
}

lk_fclose (fp, file, lockdir, lockfile)
FILE	*fp;
char	*file;			/* file to be locked */
char	*lockdir;		/* directory to put parallel file into */
char	*lockfile;		/* file to lock against */
{
#ifdef DEBUG
    ll_log (logptr, LLOGBTR, "lk_fclose (%s,%s,%s)", file,
	    lockdir ? lockdir : NIL, lockfile ? lockfile : NIL);
#endif
    if (fp == NULL)
	return (EOF);
    fflush (fp);
    lk_unlock (fileno (fp), file, lockdir, lockfile);
    return (fclose (fp));
}
#endif  /* HAVE_LOCK_FILES */

#if defined(HAVE_FLOCK)
#include <sys/file.h>
#  ifdef	HAVE_FCNTL_H
#    include <fcntl.h>
#  endif /* HAVE_FCNTL_H */
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

#  ifdef DEBUG
#    include "ll_log.h"
extern LLog *logptr;
#  endif

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
#ifdef DEBUG
		ll_err(logptr, LLOGFAT, "lk_fopen(): bad mode '%s'\n", access);
#endif
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
        case 0 /*NULL*/:
		return(OK);
	}

	return(fclose(fp));
}
#endif  /* HAVE_FLOCK */

#if defined(HAVE_F_SETLK)
#  ifdef HAVE_FCNTL_H
#    include <fcntl.h>
#  endif /* HAVE_FCNTL_H */

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

#  ifdef DEBUG
#    include "ll_log.h"
extern LLog *logptr;
#  endif

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
#ifdef DEBUG
		ll_err (logptr, LLOGFAT, "lk_fopen(): bad mode '%s'\n", access);
#endif
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

	if (fp==(FILE *)EOF || fp==(FILE *)NULL) {
		return (OK);
	}
	retval = fclose (fp);
	return (retval);
}
#endif  /* HAVE_F_SETLK */
