#include "util.h"
#include "sys/stat.h"
#ifdef V4_2BSD
#include <sys/file.h>
#endif /* V4_2BSD */
#include "d_lock.h"
#include "d_returns.h"

extern	char	*lckdfldir;
extern	int	d_errno;

/*
 *	D_LOCK
 *
 *	Routine which attempts to open a file exclusively, and thereby lock
 *	the associated port or direct line.
 *
 *	Apr 89	E. Bennett -- finally straightened out line locking.
 *		see conf/site/d_lock.h
 *	Apr 87	E. Bennett -- added ifdef's for ASCIILOCKS and ATTSVKILL
 *
 *	Feb 84	D. Long -- modified to stat the lock rather than d_verifying it
 *
 *	Mar 84	D. Rockwell & D. Long -- use flock instead of UUCP convention
 *		D. Rockwell & D. Long -- fixed bug with d_lckfd in d_lock.
 *
 *	Jul 85	D. Rockwell & D. Long -- add UUCP-compatible locking (yuck)
 *		This assumes that the phone line is being locked with
 *		"/usr/spool/uucp/LCK..tty*"
 *
 *	lockfile -- lock file name
 */

d_lock(lockfile)
register char *lockfile;
{
	extern	int	d_lckfd;
		char	lockername[1024];	/* full path of lockfile */
#ifdef UUCPLOCK
		char	tempername[1024];	/* full path of tmp file */
		int	fd;
#ifdef ASCIILOCKS
		char	pid[SIZEOFPID+2];
#else /* ASCIILOCKS */
		int	pid;
#endif /* ASCIILOCKS */

	d_lckfd = -1;
	sprintf(tempername, "%s/MLTMP.%d", UUCPLOCK, getpid());
	sprintf(lockername, "%s/%s", UUCPLOCK, lockfile);
#ifdef ASCIILOCKS
	sprintf(pid, "%*d\n", SIZEOFPID, getpid());
#else /* ASCIILOCKS */
	pid = getpid();
#endif /* ASCIILOCKS */
	(void) unlink(tempername);
	if ((fd = creat(tempername, 0644)) < 0) {
#ifdef D_LOG
		d_log("d_lock", "can't create locker file '%s'", tempername);
#endif /* D_LOG */
		(void) unlink(tempername);	/* never needed? */
		d_errno = D_LOCKERR;
		return(D_FATAL);
	}
#ifdef ASCIILOCKS
	write(fd, pid, SIZEOFPID+1);
#else /* ASCIILOCKS */
	write(fd, (char *)&pid, sizeof(pid));
#endif /* ASCIILOCKS */
	(void) close(fd);
	if (link(tempername, lockername) < 0) {
#define	NEED_D_VERIFY	1
		if (d_verify(lockername) >= D_OK) {
			(void) unlink(tempername);
			return(D_NO);
		}
		(void) unlink(lockername);
		if (link(tempername, lockername) < 0) {
#ifdef D_LOG
			d_log("d_lock", "can't link lock file '%s'", lockername);
#endif /* D_LOG */
			(void) unlink(tempername);
			return(D_NO);
		}
	}
	(void) unlink(tempername);

#else /* UUCPLOCK */
	sprintf(lockername, "%s/%s", lckdfldir, lockfile);
#ifndef V4_2BSD
#define	NEED_D_VERIFY	1
	if (d_verify(lockername) >= D_OK)
		return(D_NO);		/* it's really busy */

	(void) unlink(lockername);	/* clean it out, if necessary */
	if ((d_lckfd = creat(lockername, 0644)) < 0) {
#else /* V4_2BSD */
	if (d_lckfd < 0 && (d_lckfd = open(lockername, O_RDONLY|O_CREAT, 0644)) < 0) {
#endif /* V4_2BSD */
#ifdef D_LOG
		d_log("d_lock", "can't create lock file '%s'", lockername);
#endif /* D_LOG */
		d_errno = D_LOCKERR;
		return(D_FATAL);
	}

#ifdef V4_2BSD
	if (flock(d_lckfd, LOCK_EX|LOCK_NB) < 0) {
		(void) close(d_lckfd);
		d_lckfd = -1;
		return(D_NO);
	}
#endif /* V4_2BSD */
#endif /* UUCPLOCK */
	return(D_OK);
}

#ifdef	NEED_D_VERIFY
/*
 * d_verify - is the port REALLY active?
 */
d_verify(port)
register char *port;
{
#ifdef ATTSVKILL
	int	fd, pid;
#ifdef ASCIILOCKS
	char	alphapid[SIZEOFPID+2];
#endif /* ASCIILOCKS */

	if ((fd = open(port, 0)) == -1) {
#ifdef D_LOG
		d_log("d_lock", "can't read locker file '%s'", port);
#endif /* D_LOG */
		return(D_NO);
	}

#ifdef ASCIILOCKS
	read(fd, alphapid, SIZEOFPID+1);
	pid = atoi(alphapid);
#else /* ASCIILOCKS */
	read(fd, (char *)&pid, sizeof(pid));
#endif /* ASCIILOCKS */

	if (kill(pid, 0) == -1 && errno == ESRCH) {
#ifdef D_LOG
		d_log("d_lock", "process %d no longer active", pid);
		d_log("d_lock", "OK to remove %s", port);
#endif /* D_LOG */
		return(D_NO);
	}
#else /* ATTSVKILL */
	struct	stat	statbuf;
	time_t		curtime;

	if (stat(port, &statbuf) < 0)
		return(D_NO);		/* really is inaccessable */

	time(&curtime);

	if (curtime > (statbuf.st_atime + (time_t)(15 * 60))) {
#ifdef D_LOG
		d_log("d_lock", "%s inactive for over 15 minutes, OK to unlock", port);
#endif /* D_LOG */
		return(D_NO);		/* inactive for over 15 minutes */
	}
#endif /* ATTSVKILL */
#ifdef D_LOG
	d_log("d_lock", "%s still active, not removed", port);
#endif /* D_LOG */
	return(D_OK);
}
#endif /* NEED_D_VERIFY */

d_unlock(lockfile)
{
	extern	int	d_lckfd;
		char	lockername[1024];

	if (d_lckfd >= 0) {
#ifdef V4_2BSD
		flock(d_lckfd, LOCK_UN);
#endif /* V4_2BSD */
		(void) close(d_lckfd);
		d_lckfd = -1;
	}
	sprintf(lockername, "%s/%s", lckdfldir, lockfile);
	(void) unlink(lockername);
}
