/*
 *  F I O . C 
 *
 *  EE/CIS Computer Lab
 *  Department of Computer and Information Sciences
 *  Department of Electrical Engineering
 *  University of Delaware
 *
 *  REVISION HISTORY:
 *
 *  $Revision: 1.6 $
 *
 *  $Log: fio.c,v $
 *  Revision 1.6  1986/01/13 13:03:48  galvin
 *  Change format of debug output and delete one.
 *
 * Revision 1.6  86/01/13  13:03:48  galvin
 * Change format of debug output and delete one.
 * 
 * Revision 1.5  85/12/18  13:22:33  galvin
 * Add another argument to send to indicate whether or not this
 * message should be delimited by MMDF message delimiters.
 * 
 * Change all but "temp" file opens/closes to use MMDF locking routines.
 * 
 * Revision 1.4  85/11/20  14:34:38  galvin
 * Added some debugging code -- ifdef'ed with DEBUG and dependent on
 * debug -- and code to parse the MMDF message delimiters.  This code
 * should still support the old style Mail format but since I cannot
 * test this I don't know.
 * 
 * Revision 1.3  85/11/16  16:10:36  galvin
 * Added define for sigmask for backward compatibility from 4.3bsd to 4.2bsd.
 * 
 * Revision 1.2  85/11/16  14:33:45  galvin
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
static char *sccsid = "@(#)fio.c	5.2 (Berkeley) 6/21/85";
#endif not lint

#include "./rcv.h"
#include "./mmdf.h"
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include "./sigretro.h"

/*
 * Mail -- a mail program
 *
 * File I/O.
 */

/*
 * Set up the input pointers while copying the mail file into
 * /tmp.
 */

setptr(ibuf)
	FILE *ibuf;
{
	register int c;
	register char *cp, *cp2;
	register int count, l;
	long s;
	off_t offset;
	char linebuf[LINESIZE];
	char wbuf[LINESIZE];
	int maybe, mestmp, flag, inhead, newmsg,
		len1 = strlen(delim1) - 1, len2 = strlen(delim2) - 1;
	struct message this;
	extern char tempSet[];

	if ((mestmp = opentemp(tempSet)) < 0)
		exit(1);
	msgCount = 0;
	offset = 0;
	s = 0L;
	l = 0;
	maybe = 1;
	flag = MUSED|MNEW;
	for (;;) {
		cp = linebuf;
		c = getc(ibuf);
		while (c != EOF && c != '\n') {
			if (cp - linebuf >= LINESIZE - 1) {
				ungetc(c, ibuf);
				*cp = 0;
				break;
			}
			*cp++ = c;
			c = getc(ibuf);
		}
		*cp = 0;
		if (cp == linebuf && c == EOF) {
#ifdef DEBUG
			if (debug)
				printf("setptr: no more messages.\n");
#endif DEBUG
			this.m_flag = flag;
			flag = MUSED|MNEW;
			this.m_offset = offsetof(offset);
			this.m_block = blockof(offset);
			this.m_size = s;
			this.m_lines = l;
			if (append(&this, mestmp)) {
				perror(tempSet);
				exit(1);
			}
			makemessage(mestmp);
			close(mestmp);
			return;
		}
		count = cp - linebuf + 1;
		if (strncmp(linebuf, delim1, len1) == 0) {
			maybe = newmsg = 1;
			inhead = 0;
			continue;
		}
		if (strncmp(linebuf, delim2, len2) == 0) {
			inhead = maybe = newmsg = 0;
			continue;
		}
		for (cp = linebuf; *cp;)
			putc(*cp++, otf);
		putc('\n', otf);
		if (ferror(otf)) {
			perror("/tmp");
			exit(1);
		}
		if ((newmsg && !inhead) ||
			(maybe && linebuf[0] == 'F' && ishead(linebuf))) {
#ifdef DEBUG
			if (debug)
				printf("setptr: new message.\n");
#endif DEBUG
			msgCount++;
			this.m_flag = flag;
			flag = MUSED|MNEW;
			inhead = 1;
			this.m_block = blockof(offset);
			this.m_offset = offsetof(offset);
			this.m_size = s;
			this.m_lines = l;
			s = 0L;
			l = 0;
			if (append(&this, mestmp)) {
				perror(tempSet);
				exit(1);
			}
			newmsg = 0;
		}
		if (linebuf[0] == 0)
			inhead = 0;
		if (inhead && index(linebuf, ':')) {
			cp = linebuf;
			cp2 = wbuf;
			while (isalpha(*cp))
				*cp2++ = *cp++;
			*cp2 = 0;
			if (icequal(wbuf, "status")) {
				cp = index(linebuf, ':');
				if (index(cp, 'R'))
					flag |= MREAD;
				if (index(cp, 'O'))
					flag &= ~MNEW;
				inhead = 0;
			}
		}
		offset += count;
		s += (long) count;
		l++;
		maybe = 0;
		if (linebuf[0] == 0)
			maybe = 1;
	}
}

/*
 * Drop the passed line onto the passed output buffer.
 * If a write error occurs, return -1, else the count of
 * characters written, including the newline.
 */

putline(obuf, linebuf)
	FILE *obuf;
	char *linebuf;
{
	register int c;

	c = strlen(linebuf);
	fputs(linebuf, obuf);
	putc('\n', obuf);
	if (ferror(obuf))
		return(-1);
	return(c+1);
}

#ifdef NOT_USED
/*
 * Quickly read a line from the specified input into the line
 * buffer; return characters read.
 */

freadline(ibuf, linebuf)
	register FILE *ibuf;
	register char *linebuf;
{
	register int c;
	register char *cp;

	c = getc(ibuf);
	cp = linebuf;
	while (c != '\n' && c != EOF) {
		if (c == 0) {
			c = getc(ibuf);
			continue;
		}
		if (cp - linebuf >= BUFSIZ-1) {
			*cp = 0;
			return(cp - linebuf + 1);
		}
		*cp++ = c;
		c = getc(ibuf);
	}
	if (c == EOF && cp == linebuf)
		return(0);
	*cp = 0;
	return(cp - linebuf + 1);
}
#endif NOT_USED

/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register int n;

	clearerr(ibuf);
	if (fgets(linebuf, LINESIZE, ibuf) == NULL)
		return(0);
	n = strlen(linebuf);
	if (n >= 1 && linebuf[n-1] == '\n')
		linebuf[n-1] = '\0';
	return(n);
}

/*
 * Return a file buffer all ready to read up the
 * passed message pointer.
 */

FILE *
setinput(mp)
	register struct message *mp;
{
	off_t off;

	fflush(otf);
	off = mp->m_block;
	off <<= 9;
	off += mp->m_offset;
	if (fseek(itf, off, 0) < 0) {
		perror("fseek");
		panic("temporary file seek");
	}
	return(itf);
}

/*
 * Take the data out of the passed ghost file and toss it into
 * a dynamically allocated message structure.
 */

makemessage(f)
{
	register struct message *m;
	register char *mp;
	register count;
	long lseek();

	mp = calloc((unsigned) (msgCount + 1), sizeof *m);
	if (mp == NOSTR) {
		printf("Insufficient memory for %d messages\n", msgCount);
		exit(1);
	}
	if (message != (struct message *) 0)
		cfree((char *) message);
	message = (struct message *) mp;
	dot = message;
	lseek(f, 0L, 0);
	while (count = read(f, mp, BUFSIZ))
		mp += count;
	for (m = &message[0]; m < &message[msgCount]; m++) {
		m->m_size = (m+1)->m_size;
		m->m_lines = (m+1)->m_lines;
		m->m_flag = (m+1)->m_flag;
	}
	message[msgCount].m_size = 0L;
	message[msgCount].m_lines = 0;
}

/*
 * Append the passed message descriptor onto the temp file.
 * If the write fails, return 1, else 0
 */

append(mp, f)
	struct message *mp;
{
	if (write(f, (char *) mp, sizeof *mp) != sizeof *mp)
		return(1);
	return(0);
}

/*
 * Delete a file, but only if the file is a plain file.
 */

remove(name)
	char name[];
{
	struct stat statb;
	extern int errno;

	if (stat(name, &statb) < 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		return(-1);
	}
	return(unlink(name));
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
edstop()
{
	register int gotcha, c;
	register struct message *mp;
	FILE *obuf, *ibuf, *readstat;
	struct stat statb;
	char tempname[30], *id;

	if (readonly)
		return;
	holdsigs();
	if (Tflag != NOSTR) {
		if ((readstat = fopen(Tflag, "w")) == NULL)
			Tflag = NOSTR;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NOSTR && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			if ((id = hfield("article-id", mp)) != NOSTR)
				fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NOSTR)
		fclose(readstat);
	if (!gotcha || Tflag != NOSTR)
		goto done;
	ibuf = NULL;
	if (stat(editfile, &statb) >= 0 && statb.st_size > mailsize) {
		strcpy(tempname, "/tmp/mboxXXXXXX");
		mktemp(tempname);
		if ((obuf = fopen(tempname, "w")) == NULL) {
			perror(tempname);
			relsesigs();
			reset(0);
		}
		if ((ibuf = lk_fopen(editfile, "r", (char *) 0, (char *) 0, 5)) == NULL) {
			perror(editfile);
			fclose(obuf);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		fseek(ibuf, mailsize, 0);
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		lk_fclose(ibuf, editfile, (char *) 0, (char *) 0);
		fclose(obuf);
		if ((ibuf = fopen(tempname, "r")) == NULL) {
			perror(tempname);
			remove(tempname);
			relsesigs();
			reset(0);
		}
		remove(tempname);
	}
	printf("\"%s\" ", editfile);
	fflush(stdout);
	if ((obuf = lk_fopen(editfile, "w+", (char *) 0, (char *) 0, 5)) == NULL) {
		perror(editfile);
		relsesigs();
		reset(0);
	}
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (send(mp, obuf, 0, 1) < 0) {
			perror(editfile);
			lk_fclose(obuf, editfile, (char *) 0, (char *) 0);
			relsesigs();
			reset(0);
		}
	}
	gotcha = (c == 0 && ibuf == NULL);
	if (ibuf != NULL) {
		while ((c = getc(ibuf)) != EOF)
			putc(c, obuf);
		fclose(ibuf);
	}
	fflush(obuf);
	if (ferror(obuf)) {
		perror(editfile);
		lk_fclose(obuf, editfile, (char *) 0, (char *) 0);
		relsesigs();
		reset(0);
	}
	lk_fclose(obuf, editfile, (char *) 0, (char *) 0);
	if (gotcha) {
		remove(editfile);
		printf("removed\n");
	}
	else
		printf("complete\n");
	fflush(stdout);

done:
	relsesigs();
}

static int sigdepth = 0;		/* depth of holdsigs() */
static int omask = 0;
/*
 * Hold signals SIGHUP - SIGQUIT.
 */
holdsigs()
{

# ifdef V4_2BSD
	if (sigdepth++ == 0)
		omask = sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT));
# endif V4_2BSD
# ifdef V4_1BSD
	if (sigdepth++ == 0) {
		sighold(SIGHUP);
		sighold(SIGINT);
		sighold(SIGQUIT);
	}
# endif V4_1BSD
}

/*
 * Release signals SIGHUP - SIGQUIT
 */
relsesigs()
{

# ifdef V4_2BSD
	if (--sigdepth == 0)
		sigsetmask(omask);
# endif V4_2BSD
# ifdef V4_1BSD
	if (--sigdepth == 0) {
		sigrelse(SIGINT);
		sigrelse(SIGHUP);
		sigrelse(SIGQUIT);
	}
# endif V4_1BSD
}

/*
 * Open a temp file by creating, closing, unlinking, and
 * reopening.  Return the open file descriptor.
 */

opentemp(file)
	char file[];
{
	register int f;

	if ((f = creat(file, 0600)) < 0) {
		perror(file);
		return(-1);
	}
	close(f);
	if ((f = open(file, 2)) < 0) {
		perror(file);
		remove(file);
		return(-1);
	}
	remove(file);
	return(f);
}

/*
 * Determine the size of the file possessed by
 * the passed buffer.
 */

off_t
fsize(iob)
	FILE *iob;
{
	register int f;
	struct stat sbuf;

	f = fileno(iob);
	if (fstat(f, &sbuf) < 0)
		return(0);
	return(sbuf.st_size);
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 */

char *
expand(name)
	char name[];
{
	char xname[BUFSIZ];
	char cmdbuf[BUFSIZ];
	register int pid, l;
	register char *cp, *Shell;
	int s, pivec[2];
	struct stat sbuf;

	if (name[0] == '+' && getfold(cmdbuf) >= 0) {
		sprintf(xname, "%s/%s", cmdbuf, name + 1);
		return(expand(savestr(xname)));
	}
	if (!anyof(name, "~{[*?$`'\"\\"))
		return(name);
	if (pipe(pivec) < 0) {
		perror("pipe");
		return(name);
	}
	sprintf(cmdbuf, "echo %s", name);
	if ((pid = vfork()) == 0) {
		sigchild();
		Shell = value("SHELL");
		if (Shell == NOSTR)
			Shell = SHELL;
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		execl(Shell, Shell, "-c", cmdbuf, 0);
		_exit(1);
	}
	if (pid == -1) {
		perror("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, BUFSIZ);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		fprintf(stderr, "\"Echo\" failed\n");
		goto err;
	}
	if (l < 0) {
		perror("read");
		goto err;
	}
	if (l == 0) {
		fprintf(stderr, "\"%s\": No match\n", name);
		goto err;
	}
	if (l == BUFSIZ) {
		fprintf(stderr, "Buffer overflow expanding \"%s\"\n", name);
		goto err;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	if (any(' ', xname) && stat(xname, &sbuf) < 0) {
		fprintf(stderr, "\"%s\": Ambiguous\n", name);
		goto err;
	}
	return(savestr(xname));

err:
	return(NOSTR);
}

/*
 * Determine the current folder directory name.
 */
getfold(name)
	char *name;
{
	char *folder;

	if ((folder = value("folder")) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(name, folder);
	else
		sprintf(name, "%s/%s", homedir, folder);
	return(0);
}

/*
 * A nicer version of Fdopen, which allows us to fclose
 * without losing the open file.
 */

FILE *
Fdopen(fildes, mode)
	char *mode;
{
	register int f;
	FILE *fdopen();

	f = dup(fildes);
	if (f < 0) {
		perror("dup");
		return(NULL);
	}
	return(fdopen(f, mode));
}
