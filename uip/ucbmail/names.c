/*
 *  N A M E S . C 
 *
 *  EE/CIS Computer Lab
 *  Department of Computer and Information Sciences
 *  Department of Electrical Engineering
 *  University of Delaware
 *
 *  REVISION HISTORY:
 *
 *  $Revision: 1.4 $
 *
 *  $Log: names.c,v $
 *  Revision 1.4  1998/06/17 09:46:28  krueger
 *  Modified Files:
 *  	README configure.in h/config.h.in h/util.h lib/addr/ghost.c
 *  	lib/dial/d_parse.c lib/dial/d_script2.c lib/mmdf/ml_send.c
 *  	lib/mmdf/mq_rdmail.c lib/mmdf/qu_io.c lib/mmdf/qu_rdmail.c
 *  	lib/table/ch_tbdbm.c lib/table/dm_table.c lib/table/tb_io.c
 *  	lib/table/tb_ns.c lib/util/arg2str.c lib/util/gwdir.2.9.c
 *  	lib/util/gwdir.c lib/util/lk_lock.c src/badusers/lo_wtmail.c
 *  	src/badusers/qu2lo_send.c src/bboards/bb_wtmail.c
 *  	src/bboards/dropsbr.c src/bboards/getbbent.c
 *  	src/blockaddr/qu2ba_send.c src/delay/qu2ds_send.c
 *  	src/deliver/deliver.c src/ean/ch_ean.c src/ean/qu2en_send.c
 *  	src/list/qu2ls_send.c src/local/lo_wtmail.c
 *  	src/local/qu2lo_send.c src/niftp/hdr_proc.c
 *  	src/niftp/pn_wtmail.c src/niftp/qn_rdmail.c src/pop/dropsbr.c
 *  	src/pop/po_wtmail.c src/prog/pr2mm_send.c
 *  	src/smphone/ph2mm_send.c src/smtp/qu2sm_send.c
 *  	src/smtp/sm_wtmail.c src/smtp/smtpsrvr.c
 *  	src/submit/adr_submit.c src/submit/auth_submit.c
 *  	src/submit/mgt_submit.c src/tools/checkaddr.c
 *  	src/tools/checkque.c src/tools/checkup.c src/tools/nictable.c
 *  	src/tools/process-uucp.c src/uucp/qu2uu_send.c
 *  	src/uucp/rmail.c src/uucp/uu_wtmail.c uip/msg/msg.h
 *  	uip/msg/msg4.c uip/msg/msg5.c uip/msg/msg6.c
 *  	uip/other/emactovi.c uip/other/malias.c uip/other/mlist.c
 *  	uip/other/rcvfile.c uip/other/rcvtrip.c uip/other/resend.c
 *  	uip/other/v6mail.c uip/send/s_arginit.c uip/send/s_drfile.c
 *  	uip/send/s_externs.h uip/send/s_get.c uip/send/s_input.c
 *  	uip/send/s_main.c uip/snd/s_arginit.c uip/snd/s_drfile.c
 *  	uip/snd/s_externs.h uip/snd/s_get.c uip/snd/s_input.c
 *  	uip/snd/s_main.c uip/ucbmail/aux.c uip/ucbmail/dateparse.c
 *  	uip/ucbmail/def.h uip/ucbmail/fio.c uip/ucbmail/names.c
 *  	uip/ucbmail/optim.c uip/unsupported/autores.c
 *  	uip/unsupported/cvmbox.c
 *
 *  	 * 	Added changes from Ran Atkinson,
 *  	 * 	renamed index/rindex to strchr/strrchr
 *  	 * 	Added <unistd.h>
 *  	 * 	Added <string.h>
 *  	 *      first step of an ESMTP-base-code (still need to
 *  		implement the options, and feature)
 *
 *  Revision 1.3.2.1  1998/06/16 12:05:40  krueger
 *  Tue Jun 16 16:02:07 MET DST 1998 Kai Krueger <krueger@mathematik.uni-kl.de>
 *
 *  Modified Files:
 *   Tag: v2-44a1
 *  	README configure configure.in h/config.h.in h/util.h
 *  	lib/addr/ghost.c lib/dial/d_parse.c lib/dial/d_script2.c
 *  	lib/mmdf/ml_send.c lib/mmdf/mq_rdmail.c lib/mmdf/qu_io.c
 *  	lib/mmdf/qu_rdmail.c lib/table/ch_tbdbm.c lib/table/dm_table.c
 *  	lib/table/tb_io.c lib/table/tb_ns.c lib/util/arg2str.c
 *  	lib/util/gwdir.2.9.c lib/util/gwdir.c lib/util/lk_lock.c
 *  	src/badusers/lo_wtmail.c src/badusers/qu2lo_send.c
 *  	src/bboards/bb_wtmail.c src/bboards/dropsbr.c
 *  	src/bboards/getbbent.c src/blockaddr/qu2ba_send.c
 *  	src/delay/qu2ds_send.c src/deliver/deliver.c src/ean/ch_ean.c
 *  	src/ean/qu2en_send.c src/list/qu2ls_send.c
 *  	src/local/lo_wtmail.c src/local/qu2lo_send.c
 *  	src/niftp/hdr_proc.c src/niftp/pn_wtmail.c
 *  	src/niftp/qn_rdmail.c src/pop/dropsbr.c src/pop/po_wtmail.c
 *  	src/prog/pr2mm_send.c src/smphone/ph2mm_send.c
 *  	src/smtp/qu2sm_send.c src/smtp/sm_wtmail.c src/smtp/smtpsrvr.c
 *  	src/submit/adr_submit.c src/submit/auth_submit.c
 *  	src/submit/mgt_submit.c src/tools/checkaddr.c
 *  	src/tools/checkque.c src/tools/checkup.c src/tools/nictable.c
 *  	src/tools/process-uucp.c src/uucp/qu2uu_send.c
 *  	src/uucp/rmail.c src/uucp/uu_wtmail.c uip/msg/msg.h
 *  	uip/msg/msg4.c uip/msg/msg5.c uip/msg/msg6.c
 *  	uip/other/emactovi.c uip/other/malias.c uip/other/mlist.c
 *  	uip/other/rcvfile.c uip/other/rcvtrip.c uip/other/resend.c
 *  	uip/other/v6mail.c uip/send/s_arginit.c uip/send/s_drfile.c
 *  	uip/send/s_externs.h uip/send/s_get.c uip/send/s_input.c
 *  	uip/send/s_main.c uip/snd/s_arginit.c uip/snd/s_drfile.c
 *  	uip/snd/s_externs.h uip/snd/s_get.c uip/snd/s_input.c
 *  	uip/snd/s_main.c uip/ucbmail/aux.c uip/ucbmail/dateparse.c
 *  	uip/ucbmail/def.h uip/ucbmail/fio.c uip/ucbmail/names.c
 *  	uip/ucbmail/optim.c uip/unsupported/autores.c
 *  	uip/unsupported/cvmbox.c
 *  	Added changes from Ran Atkinson,
 *  	renamed index/rindex to strchr/strrchr
 *  	Added <unistd.h>
 *  	Added <string.h>
 *  ----------------------------------------------------------------------
 *
 *  Revision 1.3  1986/01/14 16:20:09  galvin
 *  Initial revision
 *
 * Revision 1.3  86/01/14  16:20:09  galvin
 * Throw away extract and rewrite it.  In order to provide some amount of
 * backward compatibility we make a decision about spaces in mboxes.
 * See extract() for details.
 * 
 * Force detract() to always delimit with ",".
 * 
 * Change outof() to insert a Date field and MMDF delimiters.
 * 
 * Revision 1.2  86/01/10  12:35:44  galvin
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
static char *sccsid = "@(#)names.c	5.3 (Berkeley) 11/10/85";
#endif not lint

/*
 * Mail -- a mail program
 *
 * Handle name lists.
 */

#include "./rcv.h"
#include "./mmdf.h"
#include <ctype.h>

/*
 * Allocate a single element of a name list,
 * initialize its name field to the passed
 * name and return it.
 */

struct name *
nalloc(str)
	char str[];
{
	register struct name *np;

	np = (struct name *) salloc(sizeof *np);
	np->n_flink = NIL;
	np->n_blink = NIL;
	np->n_type = -1;
	np->n_name = savestr(str);
	return(np);
}

/*
 * Find the tail of a list and return it.
 */

struct name *
tailof(name)
	struct name *name;
{
	register struct name *np;

	np = name;
	if (np == NIL)
		return(NIL);
	while (np->n_flink != NIL)
		np = np->n_flink;
	return(np);
}

/*
 * Extract a list of names from a line, and make a list of names from it.
 * Return the list or NIL if none found.
 *
 * We take this from LMCL and use some MMDF routines.  We have to make
 * a probably bad decision about spaces.  In order to provide some
 * amount of backward compatibility, we do not allow spaces in "mbox"
 * as returned by parsadr().  If there are spaces we assume that spaces
 * delimit addresses and parse appropriately.  Ditto if parsadr()
 * does not find an mbox and spaces exist in the address being parsed.
 */

extern char *adrptr;		/* this is for next_addr() */

struct name *
extract(line, ntype)
	char line[];
{
	register char *cp;
	register struct name *top, *np, *t;
	register int ret;
	char nbuf[BUFSIZ], abuf[BUFSIZ], buf[BUFSIZ], thisaddr[BUFSIZ];

	if (line == NOSTR || strlen(line) == 0)
		return(NIL);
	top = NIL;
	np = NIL;
	cp = line;

#ifdef DEBUG
	if (debug)
		printf("extracting from: \"%s\"\n", line);
#endif DEBUG
	for (adrptr = line, ret = next_address(buf);
			ret >= 0; ret = next_address(buf)) {
		if (ret == 0)
			continue; /* no address found */
		strcpy(thisaddr, buf);
		parsadr(thisaddr, NOSTR, nbuf, abuf);
		if (!*nbuf && !*abuf || strchr(nbuf, ' '))
			goto spaces;
		strcpy(nbuf, buf);

		/* Perform a simple UUCP rewrite into ARPA */
		/* We could probably be smarter here by calling parsadr */
		if (strchr(nbuf, '@') == NOSTR && strchr(nbuf, '<') == NOSTR
				&& strchr(nbuf, ' ') == NOSTR
				&& strchr(nbuf, '\t') == NOSTR
				&& (cp = strchr(nbuf, '!')) != NOSTR) {
			*cp = '\0';
			sprintf(buf, "%s@%s.UUCP", cp+1, nbuf);
			strcpy(nbuf, buf);
		}
#ifdef DEBUG
		if (debug)
			printf("extracted: \"%s\"\n", nbuf);
#endif DEBUG
		t = nalloc(nbuf);
		t->n_type = ntype;
		if (top == NIL)
			top = t;
		else
			np->n_flink = t;
		t->n_blink = np;
		np = t;
	}
	return(top);
spaces:
#ifdef DEBUG
	if (debug)
		printf("Assume addresses are delimited by spaces.\n");
#endif DEBUG
	top = NIL;
	np = NIL;
	ret = 0;

	for (cp = line; *cp; ++cp) {
		if (!isspace(*cp)) {
			nbuf[ret++] = *cp;
			continue;
		}
		if (ret == 0)
			continue;
		nbuf[ret] = '\0';
		t = nalloc(nbuf);
		t->n_type = ntype;
		if (top == NIL)
			top = t;
		else
			np->n_flink = t;
		t->n_blink = np;
		np = t;
		ret = 0;
	}
	if (ret) {
		nbuf[ret] = '\0';
		t = nalloc(nbuf);
		t->n_type = ntype;
		if (top == NIL)
			top = t;
		else
			np->n_flink = t;
		t->n_blink = np;
	}
	return(top);
}

/*
 * Turn a list of names into a string of the same names.
 */

char *
detract(np, ntype)
	register struct name *np;
{
	register int s;
	register char *cp, *top;
	register struct name *p;
	register int comma;

	comma = 1;
	if (np == NIL)
		return(NOSTR);
	ntype &= ~GCOMMA;
	s = 0;
	if (debug && comma)
		fprintf(stderr, "detract asked to insert commas\n");
	for (p = np; p != NIL; p = p->n_flink) {
		if (ntype && (p->n_type & GMASK) != ntype)
			continue;
		s += strlen(p->n_name) + 1;
		if (comma)
			s++;
	}
	if (s == 0)
		return(NOSTR);
	s += 2;
	top = salloc(s);
	cp = top;
	for (p = np; p != NIL; p = p->n_flink) {
		if (ntype && (p->n_type & GMASK) != ntype)
			continue;
		cp = copy(p->n_name, cp);
		if (comma && p->n_flink != NIL)
			*cp++ = ',';
		*cp++ = ' ';
	}
	*--cp = 0;
	if (comma && *--cp == ',')
		*cp = 0;
	return(top);
}

#ifdef NOT_USED
/*
 * Grab a single word (liberal word)
 * Throw away things between ()'s.
 */

char *
yankword(ap, wbuf)
	char *ap, wbuf[];
{
	register char *cp, *cp2;

	cp = ap;
	do {
		while (*cp && any(*cp, " \t,"))
			cp++;
		if (*cp == '(') {
			register int nesting = 0;

			while (*cp != '\0') {
				switch (*cp++) {
				case '(':
					nesting++;
					break;
				case ')':
					--nesting;
					break;
				}
				if (nesting <= 0)
					break;
			}
		}
		if (*cp == '\0')
			return(NOSTR);
	} while (any(*cp, " \t,("));
	for (cp2 = wbuf; *cp && !any(*cp, " \t,("); *cp2++ = *cp++)
		;
	*cp2 = '\0';
	return(cp);
}

/*
 * Verify that all the users in the list of names are
 * legitimate.  Bitch about and delink those who aren't.
 */

struct name *
verify(names)
	struct name *names;
{
	register struct name *np, *top, *t, *x;
	register char *cp;

#ifdef SENDMAIL
	return(names);
#else
	top = names;
	np = names;
	while (np != NIL) {
		if (np->n_type & GDEL) {
			np = np->n_flink;
			continue;
		}
		for (cp = "!:@^"; *cp; cp++)
			if (any(*cp, np->n_name))
				break;
		if (*cp != 0) {
			np = np->n_flink;
			continue;
		}
		cp = np->n_name;
		while (*cp == '\\')
			cp++;
		if (equal(cp, "msgs") ||
		    getuserid(cp) != -1) {
			np = np->n_flink;
			continue;
		}
		fprintf(stderr, "Can't send to %s\n", np->n_name);
		senderr++;
		if (np == top) {
			top = np->n_flink;
			if (top != NIL)
				top->n_blink = NIL;
			np = top;
			continue;
		}
		x = np->n_blink;
		t = np->n_flink;
		x->n_flink = t;
		if (t != NIL)
			t->n_blink = x;
		np = t;
	}
	return(top);
#endif
}
#endif NOT_USED

/*
 * For each recipient in the passed name list with a /
 * in the name, append the message to the end of the named file
 * and remove him from the recipient list.
 *
 * Recipients whose name begins with | are piped through the given
 * program and removed.
 */

struct name *
outof(names, fo, hp)
	struct name *names;
	FILE *fo;
	struct header *hp;
{
	register int c;
	register struct name *np, *top;
	time_t time();
	time_t now;
	char *date, *fname, *shell, *ctime();
	FILE *fout, *fin;
	int ispipe, s;
	extern char tempEdit[];

	top = names;
	np = names;
	time(&now);
	date = ctime(&now);
	while (np != NIL) {
		if (!isfileaddr(np->n_name) && np->n_name[0] != '|') {
			np = np->n_flink;
			continue;
		}
		ispipe = np->n_name[0] == '|';
		if (ispipe)
			fname = np->n_name+1;
		else
			fname = expand(np->n_name);

		/*
		 * See if we have copied the complete message out yet.
		 * If not, do so.
		 */

		if (image < 0) {
			if ((fout = fopen(tempEdit, "a")) == NULL) {
				perror(tempEdit);
				senderr++;
				goto cant;
			}
			image = open(tempEdit, 2);
			unlink(tempEdit);
			if (image < 0) {
				perror(tempEdit);
				senderr++;
				goto cant;
			}
			else {
				rewind(fo);
				fprintf(fout, "Date: %s\n", date);
				puthead(hp, fout, GTO|GSUBJECT|GCC|GNL);
				while ((c = getc(fo)) != EOF)
					putc(c, fout);
				rewind(fo);
				putc('\n', fout);
				fflush(fout);
				if (ferror(fout))
					perror(tempEdit);
				fclose(fout);
			}
		}

		/*
		 * Now either copy "image" to the desired file
		 * or give it as the standard input to the desired
		 * program as appropriate.
		 */

		if (ispipe) {
			wait(&s);
			switch (fork()) {
			case 0:
				sigchild();
				sigsys(SIGHUP, SIG_IGN);
				sigsys(SIGINT, SIG_IGN);
				sigsys(SIGQUIT, SIG_IGN);
				close(0);
				dup(image);
				close(image);
				if ((shell = value("SHELL")) == NOSTR)
					shell = SHELL;
				execl(shell, shell, "-c", fname, 0);
				perror(shell);
				exit(1);
				break;

			case -1:
				perror("fork");
				senderr++;
				goto cant;
			}
		}
		else {
			if ((fout = fopen(fname, "a")) == NULL) {
				perror(fname);
				senderr++;
				goto cant;
			}
			fin = Fdopen(image, "r");
			if (fin == NULL) {
				fprintf(stderr, "Can't reopen image\n");
				fclose(fout);
				senderr++;
				goto cant;
			}
			rewind(fin);
			fprintf(fout, "%s", delim1);
			while ((c = getc(fin)) != EOF)
				putc(c, fout);
			fprintf(fout, "%s", delim2);
			if (ferror(fout))
				senderr++, perror(fname);
			fclose(fout);
			fclose(fin);
		}

cant:

		/*
		 * In days of old we removed the entry from the
		 * the list; now for sake of header expansion
		 * we leave it in and mark it as deleted.
		 */

#ifdef CRAZYWOW
		if (np == top) {
			top = np->n_flink;
			if (top != NIL)
				top->n_blink = NIL;
			np = top;
			continue;
		}
		x = np->n_blink;
		t = np->n_flink;
		x->n_flink = t;
		if (t != NIL)
			t->n_blink = x;
		np = t;
#endif

		np->n_type |= GDEL;
		np = np->n_flink;
	}
	if (image >= 0) {
		close(image);
		image = -1;
	}
	return(top);
}

/*
 * Determine if the passed address is a local "send to file" address.
 * If any of the network metacharacters precedes any slashes, it can't
 * be a filename.  We cheat with .'s to allow path names like ./...
 */
isfileaddr(name)
	char *name;
{
	register char *cp;
	extern char *metanet;

	if (any('@', name))
		return(0);
	if (*name == '+')
		return(1);
	for (cp = name; *cp; cp++) {
		if (*cp == '.')
			continue;
		if (any(*cp, metanet))
			return(0);
		if (*cp == '/')
			return(1);
	}
	return(0);
}

/*
 * Map all of the aliased users in the invoker's mailrc
 * file and insert them into the list.
 * Changed after all these months of service to recursively
 * expand names (2/14/80).
 */

struct name *
usermap(names)
	struct name *names;
{
	register struct name *new, *np, *cp;
	struct grouphead *gh;
	register int metoo;

	new = NIL;
	np = names;
	metoo = (value("metoo") != NOSTR);
	while (np != NIL) {
		if (np->n_name[0] == '\\') {
			cp = np->n_flink;
			new = put(new, np);
			np = cp;
			continue;
		}
		gh = findgroup(np->n_name);
		cp = np->n_flink;
		if (gh != NOGRP)
			new = gexpand(new, gh, metoo, np->n_type);
		else
			new = put(new, np);
		np = cp;
	}
	return(new);
}

/*
 * Recursively expand a group name.  We limit the expansion to some
 * fixed level to keep things from going haywire.
 * Direct recursion is not expanded for convenience.
 */

struct name *
gexpand(nlist, gh, metoo, ntype)
	struct name *nlist;
	struct grouphead *gh;
{
	struct group *gp;
	struct grouphead *ngh;
	struct name *np;
	static int depth;
	char *cp;

	if (depth > MAXEXP) {
		printf("Expanding alias to depth larger than %d\n", MAXEXP);
		return(nlist);
	}
	depth++;
	for (gp = gh->g_list; gp != NOGE; gp = gp->ge_link) {
		cp = gp->ge_name;
		if (*cp == '\\')
			goto quote;
		if (strcmp(cp, gh->g_name) == 0)
			goto quote;
		if ((ngh = findgroup(cp)) != NOGRP) {
			nlist = gexpand(nlist, ngh, metoo, ntype);
			continue;
		}
quote:
		np = nalloc(cp);
		np->n_type = ntype;
		/*
		 * At this point should allow to expand
		 * to self if only person in group
		 */
		if (gp == gh->g_list && gp->ge_link == NOGE)
			goto skip;
		if (!metoo && strcmp(cp, myname) == 0)
			np->n_type |= GDEL;
skip:
		nlist = put(nlist, np);
	}
	depth--;
	return(nlist);
}

#ifdef NOT_USED
/*
 * Compute the length of the passed name list and
 * return it.
 */

lengthof(name)
	struct name *name;
{
	register struct name *np;
	register int c;

	for (c = 0, np = name; np != NIL; c++, np = np->n_flink)
		;
	return(c);
}
#endif NOT_USED

/*
 * Concatenate the two passed name lists, return the result.
 */

struct name *
cat(n1, n2)
	struct name *n1, *n2;
{
	register struct name *tail;

	if (n1 == NIL)
		return(n2);
	if (n2 == NIL)
		return(n1);
	tail = tailof(n1);
	tail->n_flink = n2;
	n2->n_blink = tail;
	return(n1);
}

#ifdef NOT_USED
/*
 * Unpack the name list onto a vector of strings.
 * Return an error if the name list won't fit.
 */

char **
unpack(np)
	struct name *np;
{
	register char **ap, **top;
	register struct name *n;
	char *cp;
	char hbuf[10];
	int t, extra;

	n = np;
	if ((t = lengthof(n)) == 0)
		panic("No names to unpack");

	/*
	 * Compute the number of extra arguments we will need.
	 * We need at least two extra -- one for "mail" and one for
	 * the terminating 0 pointer.  Additional spots may be needed
	 * to pass along -r and -f to the host mailer.
	 */

	extra = 2;
	if (rflag != NOSTR)
		extra += 2;
#ifdef SENDMAIL
	extra++;
	metoo = value("metoo") != NOSTR;
	if (metoo)
		extra++;
	verbose = value("verbose") != NOSTR;
	if (verbose)
		extra++;
#endif SENDMAIL
	if (hflag)
		extra += 2;
	top = (char **) salloc((t + extra) * sizeof cp);
	ap = top;
	*ap++ = "send-mail";
	if (rflag != NOSTR) {
		*ap++ = "-r";
		*ap++ = rflag;
	}
#ifdef SENDMAIL
	*ap++ = "-i";
	if (metoo)
		*ap++ = "-m";
	if (verbose)
		*ap++ = "-v";
#endif SENDMAIL
	if (hflag) {
		*ap++ = "-h";
		sprintf(hbuf, "%d", hflag);
		*ap++ = savestr(hbuf);
	}
	while (n != NIL) {
		if (n->n_type & GDEL) {
			n = n->n_flink;
			continue;
		}
		*ap++ = n->n_name;
		n = n->n_flink;
	}
	*ap = NOSTR;
	return(top);
}

/*
 * See if the user named himself as a destination
 * for outgoing mail.  If so, set the global flag
 * selfsent so that we avoid removing his mailbox.
 */

mechk(names)
	struct name *names;
{
	register struct name *np;

	for (np = names; np != NIL; np = np->n_flink)
		if ((np->n_type & GDEL) == 0 && equal(np->n_name, myname)) {
			selfsent++;
			return;
		}
}
#endif NOT_USED

/*
 * Remove all of the duplicates from the passed name list by
 * insertion sorting them, then checking for dups.
 * Return the head of the new list.
 */

struct name *
elide(names)
	struct name *names;
{
	register struct name *np, *t, *new;
	struct name *x;

	if (names == NIL)
		return(NIL);
	new = names;
	np = names;
	np = np->n_flink;
	if (np != NIL)
		np->n_blink = NIL;
	new->n_flink = NIL;
	while (np != NIL) {
		t = new;
		while (nstrcmp(t->n_name, np->n_name) < 0) {
			if (t->n_flink == NIL)
				break;
			t = t->n_flink;
		}

		/*
		 * If we ran out of t's, put the new entry after
		 * the current value of t.
		 */

		if (nstrcmp(t->n_name, np->n_name) < 0) {
			t->n_flink = np;
			np->n_blink = t;
			t = np;
			np = np->n_flink;
			t->n_flink = NIL;
			continue;
		}

		/*
		 * Otherwise, put the new entry in front of the
		 * current t.  If at the front of the list,
		 * the new guy becomes the new head of the list.
		 */

		if (t == new) {
			t = np;
			np = np->n_flink;
			t->n_flink = new;
			new->n_blink = t;
			t->n_blink = NIL;
			new = t;
			continue;
		}

		/*
		 * The normal case -- we are inserting into the
		 * middle of the list.
		 */

		x = np;
		np = np->n_flink;
		x->n_flink = t;
		x->n_blink = t->n_blink;
		t->n_blink->n_flink = x;
		t->n_blink = x;
	}

	/*
	 * Now the list headed up by new is sorted.
	 * Go through it and remove duplicates.
	 */

	np = new;
	while (np != NIL) {
		t = np;
		while (t->n_flink!=NIL &&
		    icequal(np->n_name,t->n_flink->n_name))
			t = t->n_flink;
		if (t == np || t == NIL) {
			np = np->n_flink;
			continue;
		}
		
		/*
		 * Now t points to the last entry with the same name
		 * as np.  Make np point beyond t.
		 */

		np->n_flink = t->n_flink;
		if (t->n_flink != NIL)
			t->n_flink->n_blink = np;
		np = np->n_flink;
	}
	return(new);
}

/*
 * Version of strcmp which ignores case differences.
 */

nstrcmp(s1, s2)
	register char *s1, *s2;
{
	register int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;
	} while (c1 && c1 == c2);
	return(c1 - c2);
}

/*
 * Put another node onto a list of names and return
 * the list.
 */

struct name *
put(list, node)
	struct name *list, *node;
{
	node->n_flink = list;
	node->n_blink = NIL;
	if (list != NIL)
		list->n_blink = node;
	return(node);
}

/*
 * Determine the number of elements in
 * a name list and return it.
 */

count(np)
	register struct name *np;
{
	register int c = 0;

	while (np != NIL) {
		c++;
		np = np->n_flink;
	}
	return(c);
}

#ifdef NOT_USED
cmpdomain(name, dname)
	register char *name, *dname;
{
	char buf[BUFSIZ];

	strcpy(buf, dname);
	buf[strlen(name)] = '\0';
	return(icequal(name, buf));
}
#endif NOT_USED

/*
 * Delete the given name from a namelist, using the passed
 * function to compare the names.
 */
struct name *
delname(np, name, cmpfun)
	register struct name *np;
	char name[];
	int (* cmpfun)();
{
	register struct name *p;

	for (p = np; p != NIL; p = p->n_flink)
		if ((* cmpfun)(p->n_name, name)) {
			if (p->n_blink == NIL) {
				if (p->n_flink != NIL)
					p->n_flink->n_blink = NIL;
				np = p->n_flink;
				continue;
			}
			if (p->n_flink == NIL) {
				if (p->n_blink != NIL)
					p->n_blink->n_flink = NIL;
				continue;
			}
			p->n_blink->n_flink = p->n_flink;
			p->n_flink->n_blink = p->n_blink;
		}
	return(np);
}

/*
 * Call the given routine on each element of the name
 * list, replacing said value if need be.
 */

mapf(np, from)
	register struct name *np;
	char *from;
{
	register struct name *p;

	for (p = np; p != NIL; p = p->n_flink)
		p->n_name = netmap(p->n_name, from);
}

#ifdef NOT_USED
/*
 * Pretty print a name list
 * Uncomment it if you need it.
 */

prettyprint(name)
	struct name *name;
{
	register struct name *np;

	np = name;
	while (np != NIL) {
		fprintf(stderr, "%s(%d) ", np->n_name, np->n_type);
		np = np->n_flink;
	}
	fprintf(stderr, "\n");
}
#endif NOT_USED
