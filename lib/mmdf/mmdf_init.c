/***********************************************************************/
/** 
 *
 * $Id: mmdf_init.c,v 1.8 2002/10/12 17:54:01 krueger Exp $
 *
 * @author Kai Krueger
 * (C) 2001 ITWM Kaiserslautern
 * Email: krueger@itwm.uni-kl.de
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 *
 * $Log: mmdf_init.c,v $
 * Revision 1.8  2002/10/12 17:54:01  krueger
 * *** empty log message ***
 *
 * Revision 1.2  2002/07/02 12:35:39  krueger
 * *** empty log message ***
 *
 * Revision 1.1  2002/01/28 08:38:48  krueger
 * *** empty log message ***
 *
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Feature test switches
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
//#define _POSIX_SOURCE 1

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
#include "ch.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Macros
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#define MAXARG 100

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Structures and unions
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * File scope Variables (Variables share by several functions in
 *                       the same file )
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
static	char	version[] = "$@(#)MMDFII, Release B, Update 37";

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * External Variables
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern LLog  *logptr;
extern char  *mmtailor;         /* where the tailoring file is          */
extern char  *logdfldir;
       char  *locfullname;
       char  *locfullmachine;
extern char  *locmachine;
extern char  *locname;
extern char  *locdomain;
extern int   mid_enable;
extern Table tb_mailids;
extern Table tb_users;
extern Table tb_mc;
extern Table **tb_list;
extern int   tb_numtables;
extern int   tb_maxtables;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Extern Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
extern char *multcat();
extern char *ap_dmflip();

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 * Functions declarations
 *
 *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/** 
 * unsetenv()
 * @param  char  * name
 * @return NONE
 * 
 **/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if UWE
#if defined(sun) || defined(__svr4__)
void unsetenv (name)
     char  * name;
{
  extern char **environ;
  register char   ** p = environ;
  register int    len = strlen(name);
  
  while (*p && strncmp(*p, name, len))
    p++;
  if (*p == (char *) 0)
    return;
  p++;
  while (*p) {
    *(p-1) = *p;
    p++;
  }
  *--p = (char *) 0;
}
#endif
#endif
 
void mmdf_init (pgmargc, pgmargv)  /* initialize an mmdf process */
int  pgmargc;
char *pgmargv;
{
    extern char *dupfpath ();
    char *argv[MAXARG];
    int  argc;
    char *savelog;      /* SEK hold log file name                       */
    char *pgmname = pgmargv;
#ifdef DEBUG
    int dbind;
#endif

#if UWE
    unsetenv ("TZ");
#endif
#ifdef HAVE_TZSET
	tzset();
#endif /* HAVE_TZSET */
    pgmname = (pgmname ? pgmname : "???");	/* Paranoid */
    ll_log (logptr, LLOGPTR, "mmdf_init (%s)", pgmname);

    if (!isstr (mmtailor))      /* tailor info is compiled in           */
	return;

    mmdf_fdinit();

    /* SEK save initial part of log name    */
    savelog = strdup (logptr  -> ll_file);

    ll_hdinit (logptr, pgmname);  /* make header unique                 */

    if (logdfldir != (char *) 0)
	logptr -> ll_file = dupfpath (logptr -> ll_file, logdfldir);

#ifdef HAVE_VIRTUAL_DOMAINS
    /* we need to find the right mmtailor file for our virtual domain */
    mmdf_init_mmtailor(NULL);
#endif /* HAVE_VIRTUAL_DOMAINS */

    if (tai_init (mmtailor) != OK)
	err_abrt (RP_FIO, "Can't access tailoring file '%s'", mmtailor);

    while ((argc = tai_get (MAXARG, argv)) > 0)
    {                           /* test for mmdf or dial info   */
#ifdef DEBUG
	ll_log (logptr, LLOGBTR, "argc = %d", argc);
	for (dbind = 0; dbind < argc; dbind++)
	    ll_log (logptr, LLOGFTR, "(%d) '%s'", dbind, argv[dbind]);
#endif
	if (argv[0] == 0 || argv[0][0] == '\0')
		continue;               /* noop  */

	switch (uip_tai (argc, argv))
	{                       /* user program tailoring info? */
	    case YES:
	    case NOTOK:
		continue;
	}

	switch (mm_tai (argc, argv))
	{                       /* mmdf tailoring info? */
	    case YES:
	    case NOTOK:
		continue;
	}

#ifdef HAVE_DIAL
	switch (d_tai (argc, argv))
	{                       /* dialing package info? */
	    case YES:
	    case NOTOK:
		continue;
	}
#endif /* HAVE_DIAL */

	post_tai (argc, argv);
    }

    if (argc == NOTOK)
	err_abrt (RP_MECH, "Error processing tailoring file '%s'", mmtailor);

    tai_end (0); /* we really don't want to clear and free the memory ! */

    /*
     * These are provided pre-flipped for backwards walkers as
     * a convenience.  But ... Does this always make sense?  Also,
     * what about for when these are specified in mmdftailor?  Should
     * they be flipped in mm_tai.c, or should the administrator 
     * flip them by hand?  -- DSH
     */
    if (!isstr(locfullname)) {
	locfullname = multcat(locname, ".", locdomain, (char *)0);
#ifdef JNTMAIL
	{ 
	    char *cp = ap_dmflip(locfullname);
	    free(locfullname);
	    locfullname = cp;
	}
#endif
    }

    if (!isstr(locfullmachine)) {
	if (isstr(locmachine))
	    locfullmachine = multcat(
		locmachine, ".", locname, ".", locdomain, (char *)0);
	else
	    locfullmachine = multcat(locname, ".", locdomain, (char *)0);
#ifdef JNTMAIL
	{
	    char *cp = ap_dmflip(locfullmachine);
	    free(locfullmachine);
	    locfullmachine = cp;
	}
#endif
    }

    if (mid_enable) {
	if ((tb_numtables+1) >= tb_maxtables)
	    err_abrt (RP_MECH, "No space in tb_list for tb_users/mailids");
	tb_list[tb_numtables++] = &tb_users;
	tb_list[tb_numtables++] = &tb_mailids;
	tb_list[tb_numtables] = (Table *) 0;
    }

    if (logdfldir != (char *) 0)
    {                           /* we know of malloc in code, above */
      /* free (logptr -> ll_file); */
	logptr -> ll_file = dupfpath (savelog, logdfldir);
	free (savelog);
    }
    return;
}


void tai_error (error, errp, argc, argv)
char *error, *errp;
int argc;
char **argv;
{
	char    errline[LINESIZE];

	argv[argc] = (char *) 0;
	arg2vstr (0, sizeof errline, errline, argv);
	ll_log (logptr, LLOGFAT, "%s (%s) in '%s'", error, errp, errline);
}
