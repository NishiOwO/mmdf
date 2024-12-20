#include "util.h"
#include <sys/stat.h>

/*              create a directory
 *
 *      build intervening directories, if nonexistant
 *      chown new directories to specified uid
 *
 *      some of the system calls are not checked because they can
 *      fail benignly, as when an intermediate directory is executable,
 *      but not readable or writeable.  the only test for ultimate
 *      success is being able to create and stat the final directory.
 */

int creatdir (dirptr, mode, owner, group)
    register char *dirptr;     /* pathname to directory                */
    int mode;
    int owner,                  /* non-zero uid of for dir owner        */
	group;
{
    extern int errno;
    struct stat statbuf;
    int realid,
	effecid;
    int uid,
	gid;
    char shcmd[128];
    register char *partpath;
    register char *nptr;       /* last char in partial pathname        */

    if (dirptr == (char *) 0 || isnull (*dirptr))
	 return (NOTOK);      /* programming error                      */

    if (owner != 0)             /* coerced ownship requested            */
    {                           /* noop it, if under requested id's     */
	getwho (&realid, &effecid);
	uid = realid;
	getgroup (&realid, &effecid);
	gid = realid;
	if( uid && (uid != owner || gid != group ))
	    return( NOTOK );
    }

    (void) strncpy (shcmd, "mkdir ", sizeof(shcmd));   /* initialize string with command */
    partpath = &shcmd[strlen (shcmd)];

    for (nptr = partpath, *nptr++ = *dirptr++; ; *nptr++ = *dirptr++)
	switch (*dirptr)
	{
	    case '\0':
	    case '/':
		*nptr = '\0';
		if (stat (partpath, &statbuf) < 0)
		{               /* should we try to creat it?           */
#ifdef HAVE_MKDIR
#if defined(MKDIR_HAVE_SECOND_ARG)
		    mkdir (partpath, 777);
#else /* MKDIR_HAVE_SECOND_ARG */
		    mkdir (partpath);
#endif /* MKDIR_HAVE_SECOND_ARG */
#else /* HAVE_MKDIR */
		    system (shcmd);
				/* don't check if it succeeded          */
#endif /* HAVE_MKDIR */
		    if (owner != 0)
			chown (partpath, owner, group);

		    chmod (partpath, mode);
		}
		if (isnull (*dirptr))
		    return ((stat (partpath, &statbuf) < 0) ? NOTOK : OK);
	}
}
