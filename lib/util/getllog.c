#include "util.h"
#ifdef HAVE_UTMP_H
#  include <pwd.h>
#  include <utmp.h>
#  ifdef HAVE_LASTLOG_H
#    include <lastlog.h>

/* get informtion about who is logged in */

extern char *blt();

LOCVAR char LLPATH[]  = "/usr/adm/lastlog";
LOCVAR FILE *llogfp = NULL;
LOCVAR struct utmp llogentry;
LOCVAR int lloguid;       /* keep track of current uid */

int setllog()
{
	if( llogfp == NULL )
		{
		llogfp = fopen( LLPATH, "r" );
		if ( llogfp == NULL )
			return(NOTOK);
		}
	else
		rewind( llogfp );

	lloguid = 0;
	return (OK);
}

struct utmp *
getllog()
{
	union tmpunion
	{
	    char        io[sizeof (struct lastlog)];
	    struct lastlog entry;
	}       llogbuf;
	struct passwd *pwdptr,
		      *getpwuid ();

	if (llogfp == NULL) {
		if( (llogfp = fopen( LLPATH, "r" )) == NULL )
			return(0);

		lloguid = 0;
	}

	if (fread (llogbuf.io, sizeof (struct lastlog), 1, llogfp) != 1)
		return(0);

	llogentry.ut_time = (long) llogbuf.entry.ll_time;
	blt (llogbuf.entry.ll_line, llogentry.ut_line, 8);

	if ((pwdptr = getpwuid (lloguid)) == 0)
	    strcpy (llogentry.ut_name, "????");
	else
	    blt (pwdptr -> pw_name, llogentry.ut_name, 8);

	lloguid++;
	return(&llogentry);
}

struct utmp *
getllnam(name)
char name[];
{
	struct passwd *pwdptr,
		      *gpwlnam ();

	if ((pwdptr = gpwlnam (name)) == 0)
	    return (0);

	if (setllog () == NOTOK)
		return(NULL);

	lloguid = pwdptr->pw_uid;
	fseek(llogfp, (long)pwdptr->pw_uid * sizeof (struct lastlog), 0);

	return (getllog ());
}
#  else HAVE_LASTLOG_H
LOCVAR char LLPATH[]  = "/usr/adm/lastlog";
LOCVAR FILE *llogfp = NULL;
LOCVAR union tmpunion
{
    char        io[sizeof (struct utmp)];
    struct utmp entry;
}       llogentry;

setllog()
{
	if( llogfp == NULL )
		llogfp = fopen( LLPATH, "r" );
	else
		rewind( llogfp );
}

struct utmp *
getllog()
{
	if (llogfp == NULL) {
		if( (llogfp = fopen( LLPATH, "r" )) == NULL )
			return(0);
	}
	if (fread (llogentry.io, sizeof (struct utmp), 1, llogfp) != 1)
		return(0);
	return(&llogentry.entry);
}

struct utmp *
getllnam(name)
char name[];
{
	register struct utmp *p;

	while( (p = getllog()) && !equal(name,p->ut_name, strlen(name)));

	return(p);
}

#  endif HAVE_LASTLOG_H

endllog()
{
	if( llogfp != NULL ){
		fclose( llogfp );
		llogfp = NULL;
	}
}

#else HAVE_UTMP_H
/* For systems that have no easy way to find last login time */
/* For example: SYSVR2  --  "Consider it stranded." */

setllog()
{
}

endllog()
{
}

getllog()
{
	return (0);
}

getllnam(name)
char *name;
{
	return (0);
}
#endif HAVE_UTMP_H
