#include "util.h"
#ifdef HAVE_UTMPX_H
#  include <utmpx.h>
#endif /* HAVE_UTMPX_H */
#include <utmp.h>

/* get informtion about who is logged in */

/* this is modeled after the getpw v7 set of routines */

LOCVAR char UTPATH[]  = "/etc/utmp";
LOCVAR FILE *utmpfp = NULL;
LOCVAR union tmpunion
{
    char        io[sizeof (struct utmp)];
    struct utmp entry;
}       utmp;

setutmp()
{
	if( utmpfp == NULL )
		utmpfp = fopen( UTPATH, "r" );
	else
		rewind( utmpfp );
}

endutmp()
{
	if( utmpfp != NULL ){
		fclose( utmpfp );
		utmpfp = NULL;
	}
}

struct utmp *
_getutmp()
{
	if (utmpfp == NULL) {
		if( (utmpfp = fopen( UTPATH, "r" )) == NULL )
			return(0);
	}
	if (fread (utmp.io, sizeof (struct utmp), 1, utmpfp) != 1)
		return(0);
	return(&utmp.entry);
}

struct utmp *
getutnam(name)
char name[];
{
	register struct utmp *p;

	/*
	 * Both Dynix and Ultrix has the '8' hardwired into <utmp.h>.
	 * Hope this never breaks ;-) -- DSH
	 */
	while( (p = _getutmp()) && !equal(name, p->ut_name, 8))
			;

	return(p);
}
