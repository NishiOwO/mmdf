/*
 *  L E X . C 
 *
 *  EE/CIS Computer Lab
 *  Department of Computer and Information Sciences
 *  Department of Electrical Engineering
 *  University of Delaware
 *
 *  REVISION HISTORY:
 *
 *  $Revision: 1.7 $
 *
 *  $Log: lex.c,v $
 *  Revision 1.7  1998/10/07 13:13:43  krueger
 *  Added changes from v44a8 to v44a9
 *
 *  Revision 1.6.2.1  1998/10/06 14:21:07  krueger
 *  first cleanup, is now compiling and running under linux
 *
 *  Revision 1.6  1998/05/25 20:11:33  krueger
 *  *** empty log message ***
 *
 *  Revision 1.5  1986/01/14 16:15:18  galvin
 *  Change the "mmdf.h" include line.
 *
 * Revision 1.5  86/01/14  16:15:18  galvin
 * Change the "mmdf.h" include line.
 * 
 * Revision 1.4  85/11/18  16:09:27  galvin
 * Change name of command table to CmdTab so as not
 * to conflict with cmdtab of MMDF.
 * 
 * Revision 1.3  85/11/18  14:34:21  galvin
 * Teach setfile how to MMDF lock files.
 * Have it use the access system call rather than opening and closing files.
 * 
 * Revision 1.2  85/11/18  12:35:29  galvin
 * Added comment header for revision history.
 * 
 *
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)lex.c	5.4 (Berkeley) 11/2/85";
#endif not lint

#include "./rcv.h"
#include "./mmdf.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include "ml_send.h"

/*
 * Mail -- a mail program
 *
 * Lexical processing of commands.
 */

char	*prompt = "& ";
extern char *version;

/*
 * Set up editing on the given file name.
 * If isedit is true, we are considered to be editing the file,
 * otherwise we are reading our mail which has signficance for
 * mbox and so forth.
 */

setfile(name, isedit)
	char *name;
{
	FILE *ibuf;
	struct stat stb;
	static int shudclob;
	static char efile[128];
	extern char tempMesg[];

	if ((ibuf = lk_fopen(name, "r", (char *)0, (char *)0, 5)) == NULL)
		return(-1);
	if (!edit) {
	if (fstat(fileno(ibuf), &stb) < 0) {
			perror(name);
			exit(1);
	}
		if (stb.st_size == 0) {
			lk_fclose(ibuf, name, (char *)0, (char *)0);
		return(-1);
	}
	}

	/*
	 * Looks like all will be well.  We must now relinquish our
	 * hold on the current set of stuff.  Must hold signals
	 * while we are reading the new file, else we will ruin
	 * the message[] data structure.
	 */

	holdsigs();
	if (shudclob)
		if (edit)
			edstop();
		else
			quit();

	/*
	 * Copy the messages into /tmp
	 * and set pointers.
	 */

	readonly = 0;
	if (access(name, 2) < 0)
		readonly++;
	if (shudclob) {
		fclose(itf);
		fclose(otf);
	}
	shudclob = 1;
	edit = isedit;
	strncpy(efile, name, 128);
	editfile = efile;
	if (name != mailname)
		strcpy(mailname, name);
	mailsize = fsize(ibuf);
	if ((otf = fopen(tempMesg, "w")) == NULL) {
		perror(tempMesg);
		lk_fclose(ibuf, name, (char *) 0, (char *) 0);
		exit(1);
	}
	if ((itf = fopen(tempMesg, "r")) == NULL) {
		perror(tempMesg);
		lk_fclose(ibuf, name, (char *) 0, (char *) 0);
		exit(1);
	}
	myremove(tempMesg);
	setptr(ibuf);
	setmsize(msgCount);
	lk_fclose(ibuf, name, (char *)0, (char *)0);
	relsesigs();
	sawcom = 0;
	return(0);
}

/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */

int	*msgvec;

commands()
{
	int eofloop, shudprompt, stop();
	register int n;
	char linebuf[LINESIZE];
	int hangup(), contin();

# ifdef VMUNIX
	sigset(SIGCONT, SIG_DFL);
# endif VMUNIX
	if (rcvmode && !sourcing) {
		if (sigset(SIGINT, SIG_IGN) != SIG_IGN)
			sigset(SIGINT, stop);
		if (sigset(SIGHUP, SIG_IGN) != SIG_IGN)
			sigset(SIGHUP, hangup);
	}
	shudprompt = intty && !sourcing;
	for (;;) {
		setexit();

		/*
		 * Print the prompt, if needed.  Clear out
		 * string space, and flush the output.
		 */

		if (!rcvmode && !sourcing)
			return;
		eofloop = 0;
top:
		if (shudprompt) {
			printf(prompt);
			fflush(stdout);
# ifdef VMUNIX
			sigset(SIGCONT, contin);
# endif VMUNIX
		} else
			fflush(stdout);
		sreset();

		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */

		n = 0;
		for (;;) {
			if (readline(input, &linebuf[n]) <= 0) {
				if (n != 0)
					break;
				if (loading)
					return;
				if (sourcing) {
					unstack();
					goto more;
				}
				if (value("ignoreeof") != NOSTR && shudprompt) {
					if (++eofloop < 25) {
						printf("Use \"quit\" to quit.\n");
						goto top;
					}
				}
				if (edit)
					edstop();
				return;
			}
			if ((n = strlen(linebuf)) == 0)
				break;
			n--;
			if (linebuf[n] != '\\')
				break;
			linebuf[n++] = ' ';
		}
# ifdef VMUNIX
		sigset(SIGCONT, SIG_DFL);
# endif VMUNIX
		if (execute(linebuf, 0))
			return;
more:		;
	}
}

/*
 * Execute a single command.  If the command executed
 * is "quit," then return non-zero so that the caller
 * will know to return back to main, if he cares.
 * Contxt is non-zero if called while composing mail.
 */

execute(linebuf, contxt)
	char linebuf[];
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	struct cmd *com;
	register char *cp, *cp2;
	register int c;
	int muvec[2];
	int edstop(), e;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */

	cp = linebuf;
	while (any(*cp, " \t"))
		cp++;
	if (*cp == '!') {
		if (sourcing) {
			printf("Can't \"!\" while sourcing\n");
			unstack();
			return(0);
		}
		shell(cp+1);
		return(0);
	}
	cp2 = word;
	while (*cp && !any(*cp, " \t0123456789$^.:/-+*'\""))
		*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, bitch.
	 * Normally, a blank command would map to the
	 * first command in the table; while sourcing,
	 * however, we ignore blank lines to eliminate
	 * confusion.
	 */

	if (sourcing && equal(word, ""))
		return(0);
	com = lex(word);
	if (com == NONE) {
		printf("Unknown command: \"%s\"\n", word);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0)
		if (cond == CRCV && !rcvmode || cond == CSEND && rcvmode)
			return(0);

	/*
	 * Special case so that quit causes a return to
	 * main, who will call the quit code directly.
	 * If we are in a source file, just unstack.
	 */

	if (com->c_func == edstop && sourcing) {
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (!edit && com->c_func == edstop) {
		sigset(SIGINT, SIG_IGN);
		return(1);
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	if (!rcvmode && (com->c_argtype & M) == 0) {
		printf("May not execute \"%s\" while sending\n",
		    com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (sourcing && com->c_argtype & I) {
		printf("May not execute \"%s\" while sourcing\n",
		    com->c_name);
		if (loading)
			return(1);
		unstack();
		return(0);
	}
	if (readonly && com->c_argtype & W) {
		printf("May not execute \"%s\" -- message file is read only\n",
		   com->c_name);
		if (loading)
			return(1);
		if (sourcing)
			unstack();
		return(0);
	}
	if (contxt && com->c_argtype & R) {
		printf("Cannot recursively invoke \"%s\"\n", com->c_name);
		return(0);
	}
	e = 1;
	switch (com->c_argtype & ~(F|P|I|M|T|W|R)) {
	case MSGLIST:
		/*
		 * A message list defaulting to nearest forward
		 * legal message.
		 */
		if (msgvec == 0) {
			printf("Illegal use of \"message list\"\n");
			return(-1);
		}
		if ((c = getmsglist(cp, msgvec, com->c_msgflag)) < 0)
			break;
		if (c  == 0) {
			*msgvec = first(com->c_msgflag,
				com->c_msgmask);
			msgvec[1] = NULL;
		}
		if (*msgvec == NULL) {
			printf("No applicable messages\n");
			break;
		}
		e = (*com->c_func)(msgvec);
		break;

	case NDMLIST:
		/*
		 * A message list with no defaults, but no error
		 * if none exist.
		 */
		if (msgvec == 0) {
			printf("Illegal use of \"message list\"\n");
			return(-1);
		}
		if (getmsglist(cp, msgvec, com->c_msgflag) < 0)
			break;
		e = (*com->c_func)(msgvec);
		break;

	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		while (any(*cp, " \t"))
			cp++;
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, arglist,
				sizeof arglist / sizeof *arglist)) < 0)
			break;
		if (c < com->c_minargs) {
			printf("%s requires at least %d arg(s)\n",
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			printf("%s takes no more than %d arg(s)\n",
				com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		panic("Unknown argtype");
	}

	/*
	 * Exit the current source file on
	 * error.
	 */

	if (e && loading)
		return(1);
	if (e && sourcing)
		unstack();
	if (com->c_func == edstop)
		return(1);
	if (value("autoprint") != NOSTR && com->c_argtype & P)
		if ((dot->m_flag & MDELETED) == 0) {
			muvec[0] = dot - &message[0] + 1;
			muvec[1] = 0;
			type(muvec);
		}
	if (!sourcing && (com->c_argtype & T) == 0)
		sawcom = 1;
	return(0);
}

/*
 * When we wake up after ^Z, reprint the prompt.
 */
contin()
{

	printf(prompt);
	fflush(stdout);
}

/*
 * Branch here on hangup signal and simulate quit.
 */
hangup()
{

	holdsigs();
	if (edit) {
		if (setexit())
			exit(0);
		edstop();
	}
	else
		quit();
	exit(0);
}

/*
 * Set the size of the message vector used to construct argument
 * lists to message list functions.
 */
 
setmsize(sz)
{

	if (msgvec != (int *) 0)
		cfree((char *) msgvec);
	msgvec = (int *) calloc((unsigned) (sz + 1), sizeof *msgvec);
}

/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

struct cmd *
lex(word)
	char word[];
{
	register struct cmd *cp;
	extern struct cmd CmdTab[];

	for (cp = &CmdTab[0]; cp->c_name != NOSTR; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NONE);
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */

isprefix(as1, as2)
	char *as1, *as2;
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * The following gets called on receipt of a rubout.  This is
 * to abort printout of a command, mainly.
 * Dispatching here when command() is inactive crashes rcv.
 * Close all open files except 0, 1, 2, and the temporary.
 * The special call to getuserid() is needed so it won't get
 * annoyed about losing its open file.
 * Also, unstack all source files.
 */

int	inithdr;			/* am printing startup headers */

#ifdef HAVE_NFILE
static
_fwalk(function)
	register int (*function)();
{
	register FILE *iop;

	for (iop = _iob; iop < _iob + _NFILE; iop++)
      (*function)(iop);
}
#endif /* HAVE_NFILE */

static
xclose(iop)
	register FILE *iop;
{
	if (iop == stdin || iop == stdout ||
	    iop == stderr || iop == itf || iop == otf)
		return;

	if (iop != pipef)
		fclose(iop);
	else {
		pclose(pipef);
		pipef = NULL;
	}
}

#ifndef VMUNIX
stop(s)
#else   VMUNIX
stop()
#endif  VMUNIX
{

# ifndef VMUNIX
	s = SIGINT;
# endif VMUNIX
	noreset = 0;
	if (!inithdr)
		sawcom++;
	inithdr = 0;
	while (sourcing)
		unstack();
	getuserid((char *) -1);


	/*
	 * Before we go closing all them open FILEs, we need to
         * kill off our friendly neighborhood submit process.
	 */
	ml_end(OK);
	/*
	 * Walk through all the open FILEs, applying xclose() to them
	 */
#ifdef HAVE__FILE
	_fwalk(xclose);
#endif

	if (image >= 0) {
		close(image);
		image = -1;
	}
	fprintf(stderr, "Interrupt\n");
# ifndef VMUNIX
	signal(s, stop);
# endif
	reset(0);
}

/*
 * Announce the presence of the current Mail version,
 * give the message count, and print a header listing.
 */

char	*greeting	= "Mail version %s.  Type ? for help.\n";

announce(pr)
{
	int vec[2], mdot;
	extern char *version;

	if (pr && value("quiet") == NOSTR)
		printf(greeting, version);
	mdot = newfileinfo();
	vec[0] = mdot;
	vec[1] = 0;
	dot = &message[mdot - 1];
	if (msgCount > 0 && !noheader) {
		inithdr++;
		headers(vec);
		inithdr = 0;
	}
}

/*
 * Announce information about the file we are editing.
 * Return a likely place to set dot.
 */
newfileinfo()
{
	register struct message *mp;
	register int u, n, mdot, d, s;
	char fname[BUFSIZ], zname[BUFSIZ], *ename;

	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MNEW)
			break;
	if (mp >= &message[msgCount])
		for (mp = &message[0]; mp < &message[msgCount]; mp++)
			if ((mp->m_flag & MREAD) == 0)
				break;
	if (mp < &message[msgCount])
		mdot = mp - &message[0] + 1;
	else
		mdot = 1;
	s = d = 0;
	for (mp = &message[0], n = 0, u = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW)
			n++;
		if ((mp->m_flag & MREAD) == 0)
			u++;
		if (mp->m_flag & MDELETED)
			d++;
		if (mp->m_flag & MSAVED)
			s++;
	}
	ename = mailname;
	if (getfold(fname) >= 0) {
		strcat(fname, "/");
		if (strncmp(fname, mailname, strlen(fname)) == 0) {
			sprintf(zname, "+%s", mailname + strlen(fname));
			ename = zname;
		}
	}
	printf("\"%s\": ", ename);
	if (msgCount == 1)
		printf("1 message");
	else
		printf("%d messages", msgCount);
	if (n > 0)
		printf(" %d new", n);
	if (u-n > 0)
		printf(" %d unread", u);
	if (d > 0)
		printf(" %d deleted", d);
	if (s > 0)
		printf(" %d saved", s);
	if (readonly)
		printf(" [Read only]");
	printf("\n");
	return(mdot);
}

/*
 * Print the current version number.
 */

pversion()
{
	printf("Version %s\n", version);
	return(0);
}

/*
 * Load a file of user definitions.
 */
load(name)
	char *name;
{
	register FILE *in, *oldin;

	if ((in = fopen(name, "r")) == NULL)
		return;
	oldin = input;
	input = in;
	loading = 1;
	sourcing = 1;
	commands();
	loading = 0;
	sourcing = 0;
	input = oldin;
	fclose(in);
}
