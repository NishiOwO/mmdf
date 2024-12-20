/*
 *  C M D 1 . C 
 *
 *  EE/CIS Computer Lab
 *  Department of Computer and Information Sciences
 *  Department of Electrical Engineering
 *  University of Delaware
 *
 *  REVISION HISTORY:
 *
 *  $Revision: 1.8 $
 *
 *  $Log: cmd1.c,v $
 *  Revision 1.8  2001/02/04 09:53:11  krueger
 *  Type-casting
 *
 *  Revision 1.7  1998/10/07 13:13:38  krueger
 *  Added changes from v44a8 to v44a9
 *
 *  Revision 1.6.2.1  1998/10/06 14:21:02  krueger
 *  first cleanup, is now compiling and running under linux
 *
 *  Revision 1.6  1986/01/14 14:09:45  galvin
 *  Let's not be lazy and use MMDF's smtpdate and makedate to parse the
 *  date of message into something nice (if we can).  Since the new
 *  something nice is smaller than the older mess, allow more room for the
 *  subject line to print.
 *
 * Revision 1.6  86/01/14  14:09:45  galvin
 * Let's not be lazy and use MMDF's smtpdate and makedate to parse the
 * date of message into something nice (if we can).  Since the new
 * something nice is smaller than the older mess, allow more room for the
 * subject line to print.
 * 
 * Revision 1.5  86/01/07  13:42:35  galvin
 * Change printhead to use the new parse return value.  Be
 * lazy and if parse couldn't figure out the date, then use
 * the contents of the "Date" header field as is.  It probably
 * should be parsed into something nice, but ... .
 * 
 * Revision 1.4  85/12/18  13:19:14  galvin
 * Add another argument to send to indicate whether or not this
 * message should be delimited by MMDF message delimiters.
 * 
 * Revision 1.3  85/11/20  14:31:24  galvin
 * Changed the name of the command table (cmdtab) to CmdTab so as not
 * to conflict with the command table of MMDF.
 * 
 * Revision 1.2  85/11/20  14:24:51  galvin
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
static char *sccsid = "@(#)cmd1.c	5.2 (Berkeley) 6/21/85";
#endif not lint

#include "./rcv.h"
#include <sys/stat.h>

/*
 * Mail -- a mail program
 *
 * User commands.
 */

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */

static int screen;

headers(msgvec)
	int *msgvec;
{
	register int n, mesg, flag;
	register struct message *mp;
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0)
		screen = (n-1)/size;
	if (screen < 0)
		screen = 0;
	mp = &message[screen * size];
	if (mp >= &message[msgCount])
		mp = &message[msgCount - size];
	if (mp < &message[0])
		mp = &message[0];
	flag = 0;
	mesg = mp - &message[0];
	if (dot != &message[n-1])
		dot = mp;
	for (; mp < &message[msgCount]; mp++) {
		mesg++;
		if (mp->m_flag & MDELETED)
			continue;
		if (flag++ >= size)
			break;
		printhead(mesg);
		sreset();
	}
	if (flag == 0) {
		printf("No more mail.\n");
		return(1);
	}
	return(0);
}

/*
 * Set the list of alternate names for out host.
 */
local(namelist)
	char **namelist;
{
	register int c;
	register char **ap, **ap2, *cp;

	c = argcount(namelist) + 1;
	if (c == 1) {
		if (localnames == 0)
			return(0);
		for (ap = localnames; *ap; ap++)
			printf("%s ", *ap);
		printf("\n");
		return(0);
	}
	if (localnames != 0)
		cfree((char *) localnames);
	localnames = (char **) calloc((unsigned) c, sizeof (char *));
	for (ap = namelist, ap2 = localnames; *ap; ap++, ap2++) {
		cp = (char *) calloc((unsigned) (strlen(*ap) + 1),
				     sizeof (char));
		strcpy(cp, *ap);
		*ap2 = cp;
	}
	*ap2 = 0;
	return(0);
}

/*
 * Scroll to the next/previous screen
 */

scroll(arg)
	char arg[];
{
	register int s, size;
	int cur[1];

	cur[0] = 0;
	size = screensize();
	s = screen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size > msgCount) {
			printf("On last screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	case '-':
		if (--s < 0) {
			printf("On first screenful of messages\n");
			return(0);
		}
		screen = s;
		break;

	default:
		printf("Unrecognized scrolling command \"%s\"\n", arg);
		return(1);
	}
	return(headers(cur));
}

/*
 * Compute what the screen size should be.
 * We use the following algorithm:
 *	If user specifies with screen option, use that.
 *	If baud rate < 1200, use  5
 *	If baud rate = 1200, use 10
 *	If baud rate > 1200, use 20
 */
screensize()
{
	register char *cp;
	register int s;
#ifdef	TIOCGWINSZ
	struct winsize ws;
#endif

	if ((cp = value("screen")) != NOSTR) {
		s = atoi(cp);
		if (s > 0)
			return(s);
	}
	if (baud < B1200)
		s = 5;
	else if (baud == B1200)
		s = 10;
#ifdef	TIOCGWINSZ
	else if (ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0 && ws.ws_row != 0)
		s = ws.ws_row - 4;
#endif
	else
		s = 20;
	return(s);
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */

from(msgvec)
	int *msgvec;
{
	register int *ip;

	for (ip = msgvec; *ip != (int *)0; ip++) {
		printhead(*ip);
		sreset();
	}
	if (--ip >= msgvec)
		dot = &message[*ip - 1];
	return(0);
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */

printhead(mesg)
{
	struct message *mp;
	FILE *ibuf;
	char headline[LINESIZE], wcount[10], *subjline, dispc, curind;
	char pbuf[BUFSIZ];
	int s;
	long smtpdate();
	long date;
	struct headline hl;
	register char *cp;

	mp = &message[mesg-1];
	ibuf = setinput(mp);
	readline(ibuf, headline);
	subjline = hfield("subject", mp);
	if (subjline == NOSTR)
		subjline = hfield("subj", mp);

	/*
	 * Bletch!
	 */

	if (subjline != NOSTR && strlen(subjline) > 34)
		subjline[35] = '\0';
	curind = dot == mp ? '>' : ' ';
	dispc = ' ';
	if (mp->m_flag & MSAVED)
		dispc = '*';
	if (mp->m_flag & MPRESERVE)
		dispc = 'P';
	if ((mp->m_flag & (MREAD|MNEW)) == MNEW)
		dispc = 'N';
	if ((mp->m_flag & (MREAD|MNEW)) == 0)
		dispc = 'U';
	if (mp->m_flag & MBOX)
		dispc = 'M';
	if (parse(headline, &hl, pbuf))
		if ((cp = hfield("date", mp)) != NOSTR)
			if ((date = smtpdate(cp)) != -1L) {
				makedate(&date, headline);
				headline[9] = '\0';
				hl.l_date = savestr(headline);
			}
			else
				hl.l_date = cp;
	sprintf(wcount, " %d/%ld", mp->m_lines, mp->m_size);
	s = strlen(wcount);
	cp = wcount + s;
	while (s < 7)
		s++, *cp++ = ' ';
	*cp = '\0';
	if (subjline != NOSTR)
		printf("%c%c%3d %-8.8s %9.9s %s \"%s\"\n", curind, dispc,
		    mesg, nameof(mp, 0), hl.l_date, wcount, subjline);
	else
		printf("%c%c%3d %-8.8s %9.9s %s\n", curind, dispc, mesg,
		    nameof(mp, 0), hl.l_date, wcount);
}

/*
 * Print out the value of dot.
 */

pdot()
{
	printf("%d\n", dot - &message[0] + 1);
	return(0);
}

/*
 * Print out all the possible commands.
 */

pcmdlist()
{
	register struct cmd *cp;
	register int cc;
	extern struct cmd CmdTab[];

	printf("Commands are:\n");
	for (cc = 0, cp = CmdTab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp+1)->c_name != NOSTR)
			printf("%s, ", cp->c_name);
		else
			printf("%s\n", cp->c_name);
	}
	return(0);
}

/*
 * Paginate messages, honor ignored fields.
 */
more(msgvec)
	int *msgvec;
{
	return (type1(msgvec, 1, 1));
}

/*
 * Paginate messages, even printing ignored fields.
 */
More(msgvec)
	int *msgvec;
{

	return (type1(msgvec, 0, 1));
}

/*
 * Type out messages, honor ignored fields.
 */
type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 1, 0));
}

/*
 * Type out messages, even printing ignored fields.
 */
Type(msgvec)
	int *msgvec;
{

	return(type1(msgvec, 0, 0));
}

/*
 * Type out the messages requested.
 */
jmp_buf	pipestop;

type1(msgvec, doign, page)
	int *msgvec;
{
	register *ip;
	register struct message *mp;
	register int mesg;
	register char *cp;
	int nlines;
	int brokpipe();
	FILE *obuf;

	obuf = stdout;
	if (setjmp(pipestop)) {
		if (obuf != stdout) {
			pipef = NULL;
			pclose(obuf);
		}
		sigset(SIGPIPE, SIG_DFL);
		return(0);
	}
	if (intty && outtty && (page || (cp = value("crt")) != NOSTR)) {
		if (!page) {
			nlines = 0;
			for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++)
				nlines += message[*ip - 1].m_lines;
		}
		if (page || nlines > atoi(cp)) {
			obuf = mypopen(MORE, "w");
			if (obuf == NULL) {
				perror(MORE);
				obuf = stdout;
			}
			else {
				pipef = obuf;
				sigset(SIGPIPE, brokpipe);
			}
		}
	}
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		print(mp, obuf, doign);
	}
	if (obuf != stdout) {
		pipef = NULL;
		pclose(obuf);
	}
	sigset(SIGPIPE, SIG_DFL);
	return(0);
}

/*
 * Respond to a broken pipe signal --
 * probably caused by using quitting more.
 */

brokpipe()
{
# ifndef VMUNIX
	signal(SIGPIPE, brokpipe);
# endif
	longjmp(pipestop, 1);
}

/*
 * Print the indicated message on standard output.
 */

print(mp, obuf, doign)
	register struct message *mp;
	FILE *obuf;
{

	if (value("quiet") == NOSTR)
		fprintf(obuf, "Message %2d:\n", mp - &message[0] + 1);
	touch(mp - &message[0] + 1);
	send(mp, obuf, doign, 0);
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */

top(msgvec)
	int *msgvec;
{
	register int *ip;
	register struct message *mp;
	register int mesg;
	int c, topl, lines, lineb;
	char *valtop, linebuf[LINESIZE];
	FILE *ibuf;

	topl = 5;
	valtop = value("toplines");
	if (valtop != NOSTR) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	lineb = 1;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		if (value("quiet") == NOSTR)
			printf("Message %2d:\n", mesg);
		ibuf = setinput(mp);
		c = mp->m_lines;
		if (!lineb)
			printf("\n");
		for (lines = 0; lines < c && lines <= topl; lines++) {
			if (readline(ibuf, linebuf) <= 0)
				break;
			puts(linebuf);
			lineb = blankline(linebuf);
		}
	}
	return(0);
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */

stouch(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * Make sure all passed messages get mboxed.
 */

mboxit(msgvec)
	int msgvec[];
{
	register int *ip;

	for (ip = msgvec; *ip != 0; ip++) {
		dot = &message[*ip-1];
		dot->m_flag |= MTOUCH|MBOX;
		dot->m_flag &= ~MPRESERVE;
	}
	return(0);
}

/*
 * List the folders the user currently has.
 */
folders()
{
	char dirname[BUFSIZ];
	int pid, s, e;

	if (getfold(dirname) < 0) {
		printf("No value set for \"folder\"\n");
		return(-1);
	}
	switch ((pid = fork())) {
	case 0:
		sigchild();
		execlp("ls", "ls", dirname, 0);
		_exit(1);

	case -1:
		perror("fork");
		return(-1);

	default:
		while ((e = wait(&s)) != -1 && e != pid)
			;
	}
	return(0);
}
