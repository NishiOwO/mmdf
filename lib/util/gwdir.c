#include "util.h"
#include "conf.h"
#include <sys/stat.h>




#ifdef SYS5r3

/*
 * SysVr3 has getcwd() which does the same thing as gwdir().
 */

extern char *getcwd();

char *gwdir()
{
	char buf[BUFSIZ], *cwd;

	if ((cwd=getcwd(buf, BUFSIZ)) == NULL) {
		return((char *)NOTOK);
	}
	return(cwd);
}

#else /* SYS5r3 */




LOCVAR char wd_DOT[] = ".";
LOCVAR char wd_DOTDOT[] = "..";
LOCVAR char wd_read[] = "r";

LOCVAR char     wd_name[128];     /* max size of path string            */
LOCVAR jmp_buf  wd_retloc;
LOCVAR union wd                 /* unioned for making i/o cleaner       */
{
	char            io[sizeof (struct dirtype)];
	struct dirtype   dir;
} wd_entry;
LOCVAR FILE * wd_fp;

char *
	gwdir ()                    /* what is current working directory? */
{
    struct stat wd_stat;
    short     lastd_ino;            /* last inode num                     */
    char    buf[BUFSIZ];

    if (!setjmp (wd_retloc))
    {				  /* end due to eof or err              */
	if (wd_fp == (FILE *) EOF || wd_fp == (FILE *) NULL ||
	        fclose (wd_fp) == EOF ||
		wd_name[0] == '\0' ||
		chdir (wd_name) < OK)
	    return ((char *) NOTOK);

	return (wd_name);
    }

    wd_name[0] = '\0';
    lastd_ino = -1;		  /* force to be set on first pass      */

    if (stat (wd_DOT, &wd_stat) < 0 ||         /* get inode num for this dir */
	    (wd_fp = fopen (wd_DOTDOT, wd_read)) == NULL)
	longjmp (wd_retloc, TRUE);
				  /* set to compare DOTDOT inodes       */
    setbuf (wd_fp, buf);

    for (;;)
    {
	for (;;)
	{			  /* cycle thru dir's inodes            */
	    fread (wd_entry.io, sizeof (struct dirtype), 1, wd_fp);
	    if (ferror (wd_fp) || feof (wd_fp))
		longjmp (wd_retloc, TRUE);

	    if (wd_entry.dir.d_ino == wd_stat.st_ino)
		break;		  /* got a match                        */
	}
	if (wd_fp == (FILE *) EOF || wd_fp == (FILE *) NULL ||
	         fclose (wd_fp) == EOF || ferror (wd_fp) || feof (wd_fp))
	    longjmp (wd_retloc, TRUE);
	if (wd_entry.dir.d_ino == 1)  /* hit the root directory             */
	    wd_ckroot ();	  /* try to find its name               */

	if (wd_entry.dir.d_ino == lastd_ino)
				  /* we are cycling at top              */
	    longjmp (wd_retloc, TRUE);
	else			  /* remember for next level up         */
	    lastd_ino = wd_entry.dir.d_ino;

	wd_cat ();		  /* add current name to end            */

	if (chdir (wd_DOTDOT) < OK || /* set up for next pass this dir      */
	       stat (wd_DOT, &wd_stat) < OK ||
	       freopen (wd_DOTDOT, wd_read, wd_fp) == NULL)
	    longjmp (wd_retloc, TRUE);
    }
}

LOCFUN wd_ckroot ()               /* check root dir for filesys name    */
{
    static char wd_root[] = "/";
    struct stat wd_stat,
		owd_stat;         /* to save device info                */

    if (stat (wd_entry.dir.d_name, &wd_stat) < 0)
	longjmp (wd_retloc, TRUE);

    owd_stat.st_dev = wd_stat.st_dev;

    if (chdir (wd_root) < OK)
	longjmp (wd_retloc, TRUE);

    if (freopen (wd_root, wd_read, wd_fp) == NULL)
	longjmp (wd_retloc, TRUE);

    for (;;)
    {				  /* read thru "/" dir                  */
	fread (wd_entry.io, sizeof (struct dirtype), 1, wd_fp);
	if (ferror (wd_fp) || feof (wd_fp))
	    longjmp (wd_retloc, TRUE);

	if (wd_entry.dir.d_ino == 0)  /* not allocated                      */
	    continue;

	if (stat (wd_entry.dir.d_name, &wd_stat) < 0)
	    longjmp (wd_retloc, TRUE);

	if (wd_stat.st_dev == owd_stat.st_dev)
	{			  /* device types match                 */
	    wd_stat.st_mode &= S_IFMT;
	    if (wd_stat.st_mode == S_IFDIR)
		break;		  /* and it is a directory              */
	}
    }
    if (strcmp (wd_entry.dir.d_name, wd_DOT) != 0 &&
	    strcmp (wd_entry.dir.d_name, wd_DOTDOT) != 0)
	wd_cat ();		  /* it has a real text name, too       */

    wd_entry.dir.d_name[0] = '\0';    /* hack, to force "/" at front        */
    wd_cat ();
    longjmp (wd_retloc, TRUE);
}

LOCFUN wd_cat ()
{
    extern char *strcpy (),
		*strcat ();

    if (wd_name[0] == 0)
	(void) strcpy (wd_name, wd_entry.dir.d_name);
    else
    {
	strcat (wd_name, "/");
	strcat (wd_name, wd_entry.dir.d_name);
    }
}

#endif /* SYS5r3 */
