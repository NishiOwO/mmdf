/*
 *	MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *	Department of Electrical Engineering
 *	University of Delaware
 *	Newark, Delaware  19711
 *
 *
 *	Program Channel: Take message and feed a request to a program
 *
 *
 *	P R _ W T M A I L . C
 *	=====================
 *
 *	?
 *
 *	J.B.D.Pardoe
 *	University of Cambridge Computer Laboratory
 *	October 1985
 *	
 *	based on the UUCP channel by Doug Kingston (US Army Ballistics 
 *	Research Lab, Aberdeen, Maryland: <dpk@brl>)
 *
 */

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include "ap.h"

extern struct ll_struct *logptr;
extern Chan *chan;
extern int errno;
extern int pipebroken;
extern char *locfullname;

extern int ap_outtype;
extern AP_ptr ap_s2tree ();

/*
 * local variables
 */

static int pipefd;
static char hostadr[ADDRSIZE];

pr_wtadr (host, adr, from)
    char *host, *adr, *from;
{
    char        *index (), *rindex ();
    FILE        *popen();
    char        *confstr;
    char	local[ADDRSIZE];
    AP_ptr      ap, ap_local, ap_domain, ap_route;
    int		ap_outtype_save;
    int         pipefds[2], n, i, childpid;
    char        *argv[30], *envp[1];


    ap_outtype = chan->ch_apout;

#ifdef DEBUG
    printx (
	"pr_wtadr:\n  host: %s\n  addr: %s\n  from: %s\n  ap: 0%o\n",
		host, adr, from, ap_outtype);
#endif /* DEBUG */


/*  get the host address from the channel table  */

    strncpy (hostadr, "", sizeof(hostadr));
    if (isstr (host)) {
	if (tb_k2val (chan->ch_table, TRUE, host, hostadr) != OK)
	    return (RP_USER);       /* No such host */
    }


/*  flip the host name if necessary  */

    if (ap_outtype & AP_BIG) { /* is this the correct test? (JBDP) */
	host = ap_dmflip (host);
#ifdef DEBUG
	printx ("  host flipped: %s\n", host);
#endif /* DEBUG */
    }


/*  reformat the `from' address  */

    ap_outtype_save = ap_outtype;
    if ((ap = ap_s2tree (from)) == (AP_ptr) NOTOK) {
	ll_log (logptr, LLOGTMP, "Failure to parse address `%s'", from);
	return (RP_PARM);
    }
    ap_t2parts (ap, (AP_ptr *)0, (AP_ptr *)0, &ap_local, &ap_domain, &ap_route);
    from = ap_p2s ((AP_ptr)0, (AP_ptr)0, ap_local, ap_domain, ap_route);
    ap_outtype = ap_outtype_save;
#ifdef DEBUG
    printx ("  reformatted From: %s\n", from);
#endif /* DEBUG */


/*  extract the `local' part of the address  */

    if ((ap = ap_s2tree (adr)) == (AP_ptr) NOTOK) {
	ll_log (logptr, LLOGTMP, "Failure to parse address `%s'", adr);
	return (RP_PARM);
    }
    ap_t2parts (ap,
	/* group  */ (AP_ptr *) 0,
	/* name   */ (AP_ptr *) 0,
	/* local  */ &ap_local,
	/* domain */ (AP_ptr *) 0,
	/* route  */ (AP_ptr *) 0);

/* who are we ? */

    if (isstr(chan->ch_lname) && isstr(chan->ch_ldomain))
	snprintf(local, sizeof(local), "%s.%s", chan->ch_lname, chan->ch_ldomain);
    else
	strncpy(local, locfullname, sizeof(local));

/*  set the variables; generate the command and execute  */

    set_var ("from", from);
    set_var ("local", (ap_outtype & AP_BIG ? ap_dmflip (local) : local));
    set_var ("to", adr);
    set_var ("to.user", ap_local->ap_obvalue);
    set_var ("to.host", host);

    if (!isstr(chan->ch_confstr)) {
	confstr = hostadr;
    } else {
      confstr = (char *)malloc(strlen(chan->ch_confstr+1));
      memset(confstr, '\0', strlen(chan->ch_confstr+1));
      memcpy(confstr, chan->ch_confstr, strlen(chan->ch_confstr));
	set_var ("to.address", hostadr);
    }
/*    n = sstr2arg (confstr, sizeof(argv[])/sizeof(argv[0]), argv, " \t");*/
    n = sstr2arg (confstr, 30, argv, " \t");
    argv[n] = NULL;
    expand_vars (argv);

    printx ("queuing mail for %s via %s\n\t[%s]:\n\t",
		adr, host, hostadr);
    for (i=0; i<n; i++)
        printx ("%s ", argv[i]);
    printx ("\n");

    if (pipe(pipefds) == -1) {
        ll_log (logptr, LLOGFAT, "pipe failed: %d", errno);
#ifdef DEBUG
	printx ("*** pipe failed: %d\n", errno);
#endif /* DEBUG */
        return (RP_AGN);
    }
    if ((childpid = fork()) == -1) {
        close (pipefds[0]);
        close (pipefds[1]);
        ll_log (logptr, LLOGFAT, "fork failed: %d", errno);
#ifdef DEBUG
	printx ("*** fork failed: %d\n", errno);
#endif /* DEBUG */
        return (RP_AGN);
    } else if (childpid == 0) {
    /* CHILD */
        close (pipefds[1]);
        close (0);
        dup (pipefds[0]);
        envp[0] = NULL;
        execve (argv[0], argv, envp);
        ll_log (logptr, LLOGFAT, "execve failed: %d (%s)", errno, argv[0]);
#ifdef DEBUG
	printx ("*** execve failed: %d (%s)\n", errno, argv[0]);
#endif /* DEBUG */
        exit (1);
    }
    /* PARENT */
    pipefd = pipefds[1];
    close (pipefds[0]);

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "Done pr_wtadr().");
#endif
    return (RP_OK);
}


/*
 * pr_txtcpy ()    --  copy the message body down pipefd
 * ============
 */
pr_txtcpy ()
{
    int  nread, exitcode;
    char buffer [BUFSIZ];

#ifdef DEBUG
    ll_log (logptr, LLOGFTR, "pr_txtcpy ()");
#endif

    qu_rtinit (0L);             /* ready to read the text */

    nread = sizeof(buffer);
    while (!pipebroken && (rp_gval (qu_rtxt (buffer, &nread)) == RP_OK)) {
	if (write (pipefd, buffer, nread) == -1) {
	    ll_log (logptr, LLOGFAT, "write on pipe error (errno %d)", errno);
            close (pipefd);
            wait (&exitcode);
	    ll_log (logptr, LLOGFAT, "child exited %d", exitcode);
	    return (RP_LIO);
	}
	nread = sizeof(buffer);
    }

    if (pipebroken) {
	close (pipefd);
	ll_log (logptr, LLOGFAT, "pipe broke -- probably bad host");
        wait (&exitcode);
	ll_log (logptr, LLOGFAT, "child exited %d", exitcode);
	return (RP_LIO);
    }

    return (RP_MOK); /* got the text out */
}

/*
 * pr_wttend ()  --  Cleans up after the program
 */
pr_wttend ()
{
    int exitcode;

    close (pipefd);
    wait (&exitcode);
    return ((exitcode & 0xff) != 0 ? RP_LIO : RP_MOK);
}


/*
 * variables 
 * =========
 */

struct var { char *var_name, *var_value; } variables [] = {
    { "from",		(char *)0 },
    { "local",		(char *)0 },
    { "to",		(char *)0 },
    { "to.user",	(char *)0 },
    { "to.host",	(char *)0 },
    { "to.address",	(char *)0 },
    { (char *)0,	(char *)0 }
};


set_var (name, value)
    char *name, *value;
{
    register struct var *v;
    
    for (v = &variables[0]; v->var_name != (char *)0; v++) {
	if (strcmp (v->var_name, name) == 0) {
	    v->var_value = value;
	    return;
	}
    }
#ifdef DEBUG
    printx ("*** invalid variable $(%s)\n", name);
#endif /* DEBUG */
    err_abrt (RP_MECH, "invalid variable $(%s)", name);
}


expand_vars (argv)
    char *argv[];
{
    register char ch;
    char **arg, *p;
    char name [10];
    register char *s;
    register struct var *v;

    arg = &argv[0];
    while (*arg++ != NULL) {
        p = *arg;
        if (*p++ != '$')
            continue;

        if (*p++ != '(') {
#ifdef DEBUG
            printx ("*** missing `('\n");
#endif /* DEBUG */
            err_abrt (RP_MECH, "missing `('");
        }
	s = &name[0];
	while ((ch = *p++) != ')') {
            if (ch == '\0') {
#ifdef DEBUG
                printx ("*** missing `)'\n");
#endif /* DEBUG */
                err_abrt (RP_MECH, "missing `)'");
            }
            *s++ = uptolow (ch);
        }
	*s = '\0';

	s = (char *)0;
	for (v = &variables[0]; v->var_name != (char *)0; v++) {
             if (strcmp (v->var_name, name) == 0) {
		    s = v->var_value;
		    if (s == (char *)0) {
#ifdef DEBUG
			printx ("*** variable $(%s) unset\n", name);
#endif
			err_abrt (RP_MECH, "variable $(%s) unset", name);
		    }
                    *arg = s;
		    break;
             }
        }
	if (s == (char *)0) {
#ifdef DEBUG
             printx ("*** invalid variable $(%s)\n", name);
#endif /* DEBUG */
             err_abrt (RP_MECH, "invalid variable $(%s)", name);
        }
    }
}




/*
 * lowerfy ()  -  convert string to lower case
 */
lowerfy (strp)
    char *strp;
{
    while (*strp = uptolow (*strp)) strp++;
}

