/*
 * cnvtdate.c - date conversion functions
 *
 *
 *	10/88	Edward C. Bennett <edward@engr.uky.edu>
 *		Converted to standard timezone library.
 */
#include "util.h"
#include "conf.h"
#include "cnvtdate.h"

static char *day[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char *
cnvtdate(flag, datbuf)		/* produce a date/time string		*/
int flag;			/* date format option value		*/
char *datbuf;
{
	register	struct	tm	*i;
			time_t		tsec;

	time(&tsec);
	i = localtime(&tsec);

	switch (flag) {
	case TIMCOM:
		sprintf(datbuf, "%02d%02d%02d%02d%02d",
			i->tm_year, i->tm_mon + 1, i->tm_mday,
			i->tm_hour, i->tm_min);
		break;

	case TIMSECS:		/* w/seconds dd mon yy hh:mm:ss		*/
		sprintf(datbuf, "%d %s %02d %02d:%02d:%02d",
			i->tm_mday, month[i->tm_mon], i->tm_year,
			i->tm_hour, i->tm_min, i->tm_sec);
		break;

	case TIMREG:		/* RFC 822 standard time string		*/
	default:		/* "Wed, 21 Jan 76 14:30 PDT"		*/
		sprintf(datbuf, "%s, %d %s %02d %02d:%02d:%02d %s",
			day[i->tm_wday], i->tm_mday, month[i->tm_mon],
			i->tm_year, i->tm_hour, i->tm_min, i->tm_sec,
#ifdef HAVE_TZNAME
			tzname[i->tm_isdst]
#else /* HAVE_TZNAME */
			i->tm_zone
#endif /* HAVE_TZNAME */
			);
		break;

	case TIMSHRT:		/* w/out day of week			*/
		sprintf(datbuf, "%d %s %02d %02d:%02d %s",
			i->tm_mday, month[i->tm_mon], i->tm_year,
			i->tm_hour, i->tm_min,
#ifdef HAVE_TZNAME
			tzname[i->tm_isdst]
#else /* HAVE_TZNAME */
			i->tm_zone
#endif /* HAVE_TZNAME */
			);
		break;
	}
	return(datbuf);
}
