/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)getname.c	5.2 (Berkeley) 6/21/85";
#endif not lint

#include <pwd.h>

#if !defined(__STDC__) || defined(DECLARE_GETPWUID)
extern struct passwd *getpwuid ();
#endif /* DECLARE_GETPWUID */

/*
 * Getname / getuserid for those with
 * hashed passwd data base).
 *
 */

#include "./rcv.h"

/*
 * Search the passwd file for a uid.  Return name through ref parameter
 * if found, indicating success with 0 return.  Return -1 on error.
 * If -1 is passed as the user id, close the passwd file.
 */

getname(l_uid, namebuf)
	char namebuf[];
{
	struct passwd *pw;

	if (l_uid == -1) {
		return(0);
	}
	if ((pw = getpwuid(l_uid)) == NULL)
		return(-1);
	strcpy(namebuf, pw->pw_name);
	return 0;
}

/*
 * Convert the passed name to a user id and return it.  Return -1
 * on error.  Iff the name passed is -1 (yech) close the pwfile.
 */

getuserid(name)
	char name[];
{
	struct passwd *pw;

	if (name == (char *) -1) {
		return(0);
	}
	if ((pw = getpwnam(name)) == NULL)
		return 0;
	return pw->pw_uid;
}
