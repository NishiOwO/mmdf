/*
 *	MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *	Department of Electrical Engineering
 *	University of Delaware
 *	Newark, Delaware  19711
 *
 *
 *	Program Channel (inbound): Pass message into MMDF
 *
 *
 *	P R _ E R R M S G . C
 *	=====================
 *
 *	send message to report error
 *
 *	J.B.D.Pardoe
 *	University of Cambridge Computer Laboratory
 *	October 1985
 *	
 *	July 86 - Added #ifdef V 4_2BSD around <sys/file.h>	
 *		ECB	
 */


#include <stdio.h>
#include "util.h"
#include "mmdf.h"
#if defined(HAVE_FCNTL_H)
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#if defined(HAVE_SYS_FILE_H)
#  include <sys/file.h>
#else /* HAVE_SYS_FILE_H */
#  include <fcntl.h>
#endif /* HAVE_SYS_FILE_H */
#include "ml_send.h"


#define EX_OK	0 /* everything successful */
#define EX_ADDR	1 /* bad addresses */
#define EX_MECH	2 /* interaction with MMDF failed */

#define PR_COMM '#'
#define PR_ERR  '*'


FILE *errmsg_file;
long msg_start;

extern char *sender;
extern char *sitesignature, *mmdflogin, *supportaddr, *locfullname;
extern errno, prog_debug;

static mail_support();

static char sender_buf[256];


errmsg_open () 
{
    extern long getpid ();
    char buf[80]; 
    
    rewindable_msg ();

    if (!sender) {
	/* find a return address: order of preference	*/
	/* is Resent_From > Sender > From		*/

	int prio = 0;
	long msg_pos = ftell (stdin);

	rewind_msg ();

	while (fgets (buf, sizeof (buf), stdin) != NULL && buf[0] != '\0') {
	    int newprio; register char *p;

	    if (lexnequ (buf, "From:", 5)) {
		p = &buf[5]; newprio = 1;
	    } else if (lexnequ (buf, "Sender:", 7)) {
		p = &buf[7]; newprio = 2;
	    } else if (lexnequ (buf, "Resent-From:", 12)) {
		p = &buf[12]; newprio = 3;
	    } else {
		newprio = 0;
	    }
	    
	    if (newprio > prio) {
		while (*p == ' ' || *p == '\t') p++;
		strncpy (sender_buf, p, sizeof(sender_buf));
		sender = sender_buf;
		prio = newprio;
	    }
	}
	fseek (stdin, msg_pos, 0);
    }

    snprintf (buf, sizeof(buf), "/tmp/rcvprg.eXXXXXX");
    mktemp(buf);
    errmsg_file = fopen (buf, "w+");
    if (errmsg_file == NULL) {
	printf ("%ccouldn't open errmsg\n", PR_ERR);
	exit (EX_MECH);
    }
    unlink (buf);
}


errmsg_send ()
{
    int rc;
#ifdef DEBUG
    if (prog_debug) {
	printf ("%csending error message to %s\n", PR_COMM,
				sender?sender:"MMDF support");
    }
#endif

    if (!sender) {
	mail_support ("(unknown sender)");
	return;
    }    

    mopen (sender, "Failed Mail");
    mprintf ("It was not possible to fully deliver ");
    mprintf ("your message\n(included below) for the ");
    mprintf ("reason(s) given below\n\n");
    minclude_errmsg ();
    rc = mclose ();

    if (rc != 0) {
#ifdef DEBUG
	if (prog_debug)
	    printf ("%cfailed: trying MMDF support\n", PR_ERR);
#endif /* DEBUG */
	mail_support (sender);
    }
}


static mail_support (sender)
    char *sender;
{
    mopen (mmdflogin, "Unreturned Failed Mail");
    mprintf ("Unable to return message to %s\n\n", sender);
    minclude_errmsg ();
    if (mclose () != 0) {
	printf ("%cCouldn't mail %s about mail from %s\n",
	    PR_ERR, supportaddr, sender);
	exit (EX_MECH);
    }
}


rewindable_msg ()
{
    long getpid ();
    char buf[1024];
    int f, n;

    if (fseek (stdin, msg_start, 0) != -1) return; /* rewindable! */

    snprintf (buf, sizeof(buf), "/tmp/rcvprg.mXXXXXX");
    mktemp(buf);
    if ((f = open (buf, O_RDWR|O_CREAT|O_EXCL, 0640)) < 0) {
	printf ("%cfailed to open message file %s\n", PR_ERR, buf);
	exit (EX_MECH);
    }
    unlink (buf);

    while ((n = read (0, buf, 1024)) > 0) {
	if ((n = write (f, buf, n)) < 0) break;
    }
    if (n < 0) {
	printf ("%ci/o error on message\n", PR_ERR);
	exit (EX_MECH);
    }
    close(0);
    dup(f);
    close(f);
}


rewind_msg ()
{
    if (fseek (stdin, msg_start, 0) < 0) {
	printf ("%cfailed to rewind message: %d\n", PR_ERR, errno);
	exit (EX_MECH);
    }
    return (OK);
}	    


lexnequ (str1, str2, n)
    register char *str1, *str2;
{
    extern char chrcnv[];
    while (chrcnv[*str1] == chrcnv[*str2++])
	if (--n == 0 || *str1++ == 0) return (TRUE);
    return (FALSE);
}



/* *
 * Simple Message Sending
 */

static msuccess;

mopen (to, subj)
    char *to, *subj;
{
    char buff[64];
    snprintf (buff, sizeof(buff), "%s <%s@%s>", sitesignature, mmdflogin, locfullname);
    msuccess = (ml_1adr (NO, YES, buff, subj, to) == OK);
}


mprintf (f, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)
    char *f;
{
    char buff[128];
    if (!msuccess) return;
    snprintf (buff, sizeof(buff), f, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
    msuccess = (ml_txt (buff) == OK);
}


minclude (f)
    FILE *f;
{
    if (!msuccess) return;
    msuccess = (ml_file (f) == OK);
}


mclose ()
{
    if (msuccess) {
	msuccess = (ml_end (OK) == OK);
    } else {
	ml_end (NOTOK);
    }
    return (msuccess? 0 : -1);
}



minclude_errmsg ()
{
    FILE *msg;
    char *divider = 
	"\n-------------------------------------------------------\n\n";

	/*
	 * rewind() doesn't return anything so I removed it from
	 * the msuccess statement. ECB
	rewind (errmsg_file);
	 */
    msuccess = (
 	fseek (errmsg_file,0L,0)	!= -1   &&
	ml_file (errmsg_file)		== OK   &&
	ml_txt (divider)		== OK   &&
	rewind_msg ()			!= -1   &&
	(msg = fdopen (0, "r"))		!= NULL &&
	ml_file (msg) 			== OK   &&
	ml_txt (divider)	 	== OK
    );
}
