/*
 * $Id: tb_check.c,v 1.3 2001/10/26 13:41:42 krueger Exp $
 */

#include "util.h"
#include "mmdf.h"
#include "tb_check.h"

#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#if !HAVE_SYS_ERRLIST_DECL
extern	int	sys_nerr;
extern	char	*sys_errlist[];
#endif /* HAVE_SYS_ERRLIST_DECL */

/*  maximum number of output lines to be queued at once  */
#define         QUESIZ          20

/*  the maximum number of path names that can be remembered as
 *  having been checked already.  See chkpath()
*/
#define         MAXPATH         15

struct S_prtque  {
    int q_level;
    char q_text[126];
};

int verbosity = BACKGROUND;

struct stat statbuf;

int prtmax;
struct S_prtque prtque[QUESIZ];

static char probfmt[] = "%-18s: %s\n";

void qflush (control)
int control;
{
    register int index, prtany;

    /*  determine if any of the queue should be output */
    for (prtany = index = 0;  index < prtmax;  index++)
	if (prtque[index].q_level <= verbosity)
	{
	    prtany = 1;
	    break;
	}

    if (prtany != 0)        /*  go through and print appropriate lines  */
    {
	for (index = 0;  index < prtmax;  index++)
		printf ("%s", prtque[index].q_text);
	prtmax = 0;            /* start over */
    }
    else
    {
	for (index = 0;     /* remove excess only */
		(index < prtmax) && (prtque[index].q_level < control);
		index++)
	    ;
	prtmax = index;
    }
    return;
}

/*VARARGS1*/
void que (control, msg, arga, argb, argc, argd)
int control;
char *msg, *arga, *argb, *argc, *argd;
{
    char tmp[128];

    if (prtmax < (QUESIZ-1))
	format (prtque[prtmax].q_text, control, msg, arga, argb, argc, argd);
    else
    {
	qflush (LEVEL0);
	printf ("Recompile this program with larger QUESIZ.\n");
	printf ("Queue overflow on:\n  ");
	format (tmp, control, msg, arga, argb, argc, argd);
	printf (tmp);
	exit (NOTOK);
    }
    prtque[prtmax].q_level = control & ~PERROR;
    if (prtque[prtmax].q_level <= verbosity)
	qflush (prtque[prtmax++].q_level);
    else
	prtmax++;
    return;
}

/*VARARGS2*/
void format (buff, control, msg, arga, argb, argc, argd)
char buff[];
int control;
char *msg, *arga, *argb, *argc, *argd;
{
    register int n;

    switch (control&~PERROR)
    {
	case LEVEL1:
	    (void) strncpy (buff, "**    ", 6);
	    n = 6;
	    break;

	default:
	    n = 0;
    }

    (void) sprintf (&buff[n], msg, arga, argb, argc, argd);

    if ((control & PERROR) && (errno > 0)) {
    	n = strlen(buff);
	(void) sprintf(&buff[n], " (%s)\n", xerrstr());
    }

    return;
}

/**/

int chkfile (dirptr, maxprot, minprot, owner, group, ownname)
register char *dirptr;
int maxprot, minprot, owner, group;
char ownname[];
{
  register char *nptr;
  char partpath[128];

  /*  verify that I have been given a real path name  */
  if (!isstr(dirptr))
  {
	que (LEVEL1, "useless filename\n");
	return (NOTOK);
  }

  /*  step through the path, stopping at /'s to check the
   *  directory.
   */
  nptr = partpath;
  while ((*nptr++ = *dirptr++) != '\0')
	/*  Note that this checks the NEXT char, not the one just
	 *  transfered.
	 */
	if (*dirptr == '/')
	{
      *nptr = '\0';
      if (chkpath (PARTIAL, partpath, maxprot, minprot,
                   owner, group, ownname) != OK)
		return (NOTOK);
	}

  /*  At this point the entire pathname has been transfered.
   *  Check it (errors here are more serious than above.)
   */
  if (chkpath(FINAL, partpath, maxprot, minprot,
              owner, group, ownname) != OK)
    return (NOTOK);
  return (OK);
}
/**/

/*  Note that a return of OK from this routine indicates only
 */
/*ARGSUSED*/
int chkpath (control, name, maxprot, minprot, owner, group, ownname)
int control;
char *name;
int maxprot, minprot, owner, group;
char *ownname;
{
  struct passwd *pwdptr;
  static char pathdone[MAXPATH][50];
  register int n;

  /*  First, must check to see if the dir is statable.
   *  A failure here must return NOTOK even if this dir
   *  has been checked before.
   */
  if (stat (name, &statbuf) < OK)
  {
	que (LEVEL1, probfmt, "unable to stat", xerrstr());
    if (control == PARTIAL)
      que (LEVEL1, "Unable to proceed checking this path.\n");
	return (NOTOK);
  }

  /*  If this path has already been checked, then don't
   *  do it again.  The repeated error msgs are annoying.
   */
  for (n = 0;  ((n < MAXPATH) && (pathdone[n][0] != '\0'));  n++)
	if (strcmp (name, pathdone[n]) == 0)
      return (OK);

  /*  save this name as one that has been checked  */
  if (n < MAXPATH)
	(void) strcpy (pathdone[n], name);

  /*  verify that the owner is correct  */
  if (statbuf.st_uid != owner)
  {
	if ((pwdptr = getpwuid (statbuf.st_uid)) != NULL)
	{
      if (control == FINAL)
		que (LEVEL1, "Wrong owner       : (%s) for '%s'; should be %s\n",
             pwdptr -> pw_name, name, ownname);
      else
		que (LEVEL6_5, "\t[ Wrong owner   : (%s) for '%s'; should be %s ]\n",
             pwdptr -> pw_name, name, ownname);
	}
	else
	{
      if (control == FINAL)
		que (LEVEL1, "Wrong owner      : (uid %d) for '%s'; should be %s\n",
             statbuf.st_uid, name, ownname);
      else
		que (LEVEL6_5, "\t[ Wrong owner  : (uid %d) for '%s'; should be %s ]\n",
             statbuf.st_uid, name, ownname);
	}
	qflush (LEVEL6_5);
  }

    /* 
     *  Ingore bits that are site choice and not critical to operation.
     */
    if (control == PARTIAL) {
    	minprot |= 0111;
    	maxprot |= 0111;
    }
    statbuf.st_mode &= 06707;
    minprot &= 06707;
    maxprot &= 06707;
    if ((statbuf.st_mode&(~minprot)) || 	/* Any extra bit on? */
        (statbuf.st_mode&maxprot) != maxprot)	/* Any bits missing? */
    {
	if (control == FINAL)
	    que (LEVEL1, "Wrong mode        : (0%o) for '%s'; should be (0%o to 0%o)\n",
			statbuf.st_mode, name, maxprot, minprot);
	else
	    que (LEVEL6_5, "\t[ Wrong mode    : (0%o) for '%s'; should be (0%o to 0%o) ]\n",
			statbuf.st_mode, name, maxprot, minprot);
	qflush (LEVEL6_5);
    }
    return (OK);
}
/**/

char *
xerrstr()
{
    static char buff[64];

    if (errno > sys_nerr || errno < 0)
	(void) snprintf (buff, sizeof(buff), "Errno %d", errno);
    else
	(void) strncpy(buff, sys_errlist[errno], sizeof(buff));
    return (buff);
}
