/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 *
 * $Id: adrparse.c,v 1.5 2001/10/03 18:08:23 krueger Exp $
 *
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*
**
**	R E V I S I O N  H I S T O R Y
**
**	03/08/85	LDK	Added compress() in this module to remove
**			VAK	leading and trailing space.
**
*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * System headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Local headers
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#include "util.h"
#include "mmdf.h"
#include "ap.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Macros
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Structures and unions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * File scope Variables (Variables share by several functions in
 *                       the same file )
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External Variables
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern	int	ap_outtype;
extern	LLog	*logptr;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Extern Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern	char	*ap_s2p();
extern	char	*ap_p2s();
extern	char	*compress();

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#ifdef __STDC__
void parsadr(register char *, char *, char *, char *);
#endif


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * parsadr()
 * @param  char *thestr  [IN]  string with an address
 * @param  char *name    [OUT] where to put name part
 * @param  char *mbox    [OUT] where to put mailbox part
 * @param  char *host    [OUT] where to put hostname part
 * @return void
 *
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
void parsadr(thestr, name, mbox, host)
register char *thestr;		/* string with an address	*/
char	*name,			/* where to put name part	*/
	*mbox,			/* where to put mailbox part	*/
	*host;			/* where to put hostname part	*/
{
	register	char	*cp;
			AP_ptr	treep, namep, local, domain, routep;
			char	*route;
			char	tbuf[LINESIZE];

	ap_outtype = AP_822;

	if (name != (char *)0)
		*name = '\0';
	if (mbox != (char *)0)
		*mbox = '\0';
	if (host != (char *)0)
		*host = '\0';

	cp = ap_s2p (thestr, &treep, (AP_ptr *)0, &namep,
				&local, &domain, &routep);
	if (cp == (char *)DONE || cp == (char *)NOTOK)
		return;
	else if (local->ap_obvalue == NULL)
		return;		/* Bad format */
	if (name && namep != 0 && namep->ap_obvalue != 0)
		(void) strcpy(name, namep->ap_obvalue);
	if (mbox && local != 0 && local->ap_obvalue != 0)
		(void) strcpy(mbox, local->ap_obvalue);
	if (host && domain != 0 && domain->ap_obvalue != 0)
		(void) strcpy(host, domain->ap_obvalue);
	if (mbox && routep != (AP_ptr)0 &&
	    (route = ap_p2s((AP_ptr)0, (AP_ptr)0,
	    (AP_ptr)0, (AP_ptr)0, routep)) != (char *)NOTOK)
	{
		(void) strncpy(tbuf, route, sizeof(tbuf));
		(void) strncat(tbuf, mbox, sizeof(tbuf)-strlen(route));
		free(route);
		(void) strcpy(mbox, tbuf);
	}
	ap_free(treep);
	return;
}
