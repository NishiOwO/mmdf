/*
 *                              R M A I L . C
 *
 *      Developed from the Berkeley mail program of the same name
 *      by Mike Obrien at RAND to run with the MMDF mail system.
 *      Rewritten by Doug Kingston, US Army Ballistics Research Laboratory
 *      Hacked a lot by Steve Bellovin (smb@unc)
 *
 *      This program runs SETUID to Opr so that it can set effective and
 *      real [ug]ids to mmdflogin.
 *
 *      Steve Kille -  Aug 84
 *              Take machete to it.  Use protocol mode,
 *              and always go thru submit - phew
 *      Lee McLoughlin Oct 84.
 *              Address munges some header lines into 822 format.
 *	Peter Collinson Dec 85.
 *		Lee's version was dependent on 
 *		(a) the uucp channel
 *		(b) the domain tables associated with the uucp channel
 *		This version costs more cycles (but there is generally
 *		only one of these running on a machine at a time) and
 *		uses mmdf's tables to dis-assemble bang routes converting
 *		them into a domain address.
 *	Peter Collinson March 87.
 *		Add the CITATION ifdef to allow shortened messages to be
 *		returned on failure. The value of CITATION is the number of
 *		message lines returned. Change the format of bounced messages
 *		to be more like the rest of MMDF. Add a line formatter so that
 *		error messages are folded over several lines.
 *		Add sundry (void)'s to stop the C compiler on the HLH Orion
 *		complaining.
 *	Peter Collinson June 87
 *		The existing version send bang addresses into submit
 *		from the rmail command line. This breaks other channels
 *		which do not know about !'s. So, alter any PURE bang
 *		address into a 733 % % % @ sequence allowing the uucp
 *		channel to change these back to ! ! ! and other channels
 *		to deal with them appropriately.
 *	
 *	Jacob Gore May 89
 *		Instead of converting "a!b!c!user" to "user%c%b@a", which
 *		does NOT get converted back by the uucp channel, just convert
 *		it to pseudo-RFC-976 form, "b!c!user@a" ("pseudo-" because
 *		"a" by itself may not be a valid internet hostname).
 */
#undef  RUNALON

#include "util.h"
#include "mmdf.h"
#include "ap.h"
#include "ch.h"
#include "dm.h"			/* needed for host_equal */
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include "ml_send.h"
#include "mm_io.h"

#define	VERSION		"4.0"

#define	CITATION	6	/* Added Peter C March 1987 */
				/* when bouncing only bounce part of */
				/* a message */

#define NAMESZ		256	/* Limit on component name size. LMCL was 64 */
#define	MAXADRS 	6	/* Max no. of adrs to output on a single line */
#define	MAXERRSTASH	5
#define	ungetline(s)	((void) strcpy(tmpline,s),usetmp=1)

typedef	struct {
	char	*hname;
	int	hfound;
} Hdrselect;

typedef	struct	{		/* Where to save returnerror messages */
	char	e_s_dest[LINESIZE];
	struct	rp_bufstruct e_s_reason;
} Err_stash;

typedef	struct {
	char	*o_host;		/* Host name */
	char	*o_official;		/* Official name */
} Official;

Hdrselect hdrselect[] = {
#define HDATE	0
	"date",		0,	/* this is a little naughty - we need */
				/* to register that we have had this */
				/* but not address munge it */
				/* so it has token 0 */
#define HFROM	1
	"from",		0,
#define HTO	2
	"to",		0,
#define HCC	3
	"cc",		0,
#define HBCC	4
	"bcc",		0,
#define HSENDER	5			
	"sender",	0,
#define HREPLY	6
	"reply-to",	0,
#define HRFROM	7
	"resent-from",	0,
#define HRSENDER	8			
	"resent-sender",0,
#define HRTO	9
	"resent-to",	0,
	0,	0
};

Err_stash	e_stash[MAXERRSTASH];

extern	LLog	*logptr;
extern	char	*locfullname;
extern	char	*Uchan;
extern	char	*mmdflogin;
extern	char	*sitesignature;
extern	char	*supportaddr;
extern	char	*optarg;

extern char *getenv();	/* get the Accounting system from the environment */
extern char *compress();

extern	int	optind;

FILE	*rm_msgf;		/* temporary out for message text */
Chan	*chanptr;
char	Msgtmp[] = "/tmp/rmail.XXXXXX";
char	*next = NULL;		/* Where nextchar() gets the next char from */
char	rm_date[LINESIZE];	/* date of origination from uucp header */
char	rm_from[LINESIZE];	/* accumulated path of sender */
char	origsys[NAMESZ];	/* originating system */
char	origpath[LINESIZE];	/* path from us to originating system */
char	Mailsys[LINESIZE];	/* Mail system signature */
char	tmpline[LINESIZE];	/* Temp buffer for continuations and such */
char	adrs[LINESIZE];		/* Partly munged addresses go here */
char	linebuf[LINESIZE];	/* scratchpad */
char	nextdoor[LINESIZE];	/* The site nextdoor who sent the mail */
char	uchan[LINESIZE];	/* Channel to use for submission */

int	Tmpmode = 0600;
int	usetmp = 0;		/* whether tmpline should be used */
int	e_s_count;
int	e_s_other;
int	debug;


main(argc, argv)
char **argv;
{
	struct	passwd	*pw, *getpwnam(), *getpwuid();
	char		*fromptr, *cp, *d;
	char		fromwhom[NAMESZ];	/* user on remote system */
	char		sys[NAMESZ];	/* an element in the uucp path */
	int		fd;

	mmdf_init(argv[0]);

	while ((fd = getopt(argc, argv, "df:")) != EOF) {
		switch (fd) {
		case 'd':
			debug = 1;
			break;

		case 'f':
			close(0);
			open(optarg, 0);
			break;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "Usage: rmail user [user ...]\n");
		exit(1);
	}

#if 0
	/*
	 * point standard error and standard out at /dev/null
	 */
	if (debug == 0) {
		close(1);
		open("/dev/null", 1);
		close(2);
		dup(1);
	}
#endif
	
	umask(0);

	/*
	 * First, change effective and real UID/GID into Mailsys owner
	 */
	if ((pw = getpwnam(mmdflogin)) == NULL) {
		fprintf(stderr, "Cannot find mmdflogin\n");
		exit(99);
	}
	setgid(pw->pw_gid);
	setuid(pw->pw_uid);

	(void) sprintf(Mailsys, "%s <%s@%s>",
		sitesignature, mmdflogin, locfullname);

	/*
	 * Create temp file for body of message
	 */
	mktemp(Msgtmp);
	if ((fd = creat(Msgtmp, Tmpmode)) < 0)
		bomb("Can't create %s\n", Msgtmp);
	close(fd);

	if ((rm_msgf = fopen(Msgtmp, "r+")) == NULL)
		bomb("Can't reopen %s\n", Msgtmp);

	unlink(Msgtmp);

	/*
	 * Unravel the From and >From lines at the head of the message
	 * to work who sent it, the path it took to get here and when
	 * it was sent.
	 *
	 * Some or all of this info may be ignore depending on the 822 header
	 * info given.
	 */
	for (;;) {
		if (fgets(linebuf, sizeof(linebuf), stdin) == NULL)
			break;
		if (strncmp(linebuf, "From ", 5) &&
		    strncmp(linebuf, ">From ", 6))
			break;

		cp = strchr(linebuf, ' ');	/* start of name */
		fromptr = ++cp;
		cp = strchr(cp, ' ');		/* cp at end of name */
		*cp++ = '\0';			/* term. name, cp at date */
		(void) strcpy(fromwhom, fromptr);
		while (isspace(*cp)) cp++;	/* Skip any ws */
		/*
		 * The date is the rest of the line ending at \0 or remote
		 */
		d = rm_date;
		while (*cp && strncmp(cp, " remote from ", 13))
			*d++ = *cp++;
		*d = '\0';

		for (;;) {
			cp = strchr(cp+1, 'r');
			if (cp == NULL) {
				cp = strrchr(fromwhom, '!');
				if (cp != NULL) {
					char *p;
					*cp = '\0';
					p = strrchr(fromwhom, '!');
					if (p != NULL)
						(void) strcpy(origsys, p+1);
					else
						(void) strcpy(origsys, fromwhom);
					(void) strcat(rm_from, fromwhom);
					(void) strcat(rm_from, "!");
					(void) strcpy(fromwhom, cp+1);
					goto out;
				}
				/*
				 * Nothing coherent found - so look in
				 * environment for ACCTSYS
				 */
				if ((cp = getenv("ACCTSYS")) && *cp) {
					(void) strcpy(origsys, cp);
					(void) strcat(rm_from, cp);
					(void) strcat(rm_from, "!");
					goto out;
				}
				cp = "remote from somewhere";
			}
			if (strncmp(cp, "remote from ", 12) == 0)
				break;
		}

		sscanf(cp, "remote from %s", sys);
		(void) strcat(rm_from, sys);
		(void) strcpy(origsys, sys);	/* Save for quick ref. */
		(void) strcat(rm_from, "!");
	}
out:
	if (fromwhom[0] == '\0')		/* No from line, illegal */
		bomb("No from lines in message\n");

	ungetline(linebuf);

	/*
	 * Complete the from field
	 */
	(void) strcat(rm_from, fromwhom);
	
	/*
	 * A uk special - see if the first two components of the constructed
	 * bang address are in fact the same site. If so replace by their
	 * official name
	 */
	domain_cross(rm_from);

	/*
	 * Save a copy of the path to the original site.
	 * This is all the the path we were given - the user name after
	 * the last !. NOTE: We keep the trailing !, hence the +1.
	 */
	(void) strcpy(origpath, rm_from);
	*(strrchr(origpath, '!') + 1) = '\0';

	/*
	 * Savepath is given a copy of the immediate neighbour
	 */
	if ((d = strchr(rm_from, '!')) != NULL) {
		*d = '\0';
		(void) strcpy(nextdoor, rm_from);
		*d = '!';
	}
	else
		(void) strcpy(nextdoor, rm_from);
	
	/*
	 * Find the channel depending in the nextdoor site
	 */
	set_channel(nextdoor);
	ch_llinit(chanptr);

	/*
	 * Convert the from to Arpa format and leave it in from
	 */
	fromcvt(rm_from, fromwhom);
	(void) strcpy(rm_from, fromwhom);

	if (debug)
		printf("from=%s, origpath=%s, date=%s\n",
			rm_from, origpath, rm_date);

	msgfix(rm_from, rm_date);

	xsubmit(rm_from, argv, argc);

	exit(0);
}

/*
 * Munge the message header.
 *
 * All header lines with addresses in are munged into 822 format.
 * If no "From:" line is given supply one based on the UUCP Froms, do the
 * same if no "Date:".
 */
/*
 * Is this header in the list of those to have their addresses munged?
 * returnheader value offset value
 * notice that it returns 0 for the date header
 */
shouldmunge(name)
char *name;
{
	register	Hdrselect	*h;

	if (debug)
		printf("in shouldmunge with '%s'\n", name);

	for (h = hdrselect; h->hname != NULL; h++)
		if (lexequ(h->hname, name)) {
			h->hfound++;
			return(h - hdrselect);
		}

	return(0);
}

/*
 * If this is a header line then grab the name of the header and stuff it
 * into 'name' then return a pointer to the actually body of the line.
 * Otherwise return NULL.
 *
 * NOTE: A header is a line that begins with a word formed of alphanums and
 * dashes (-) then possibly some whitespace then a colon (:).
 */
char *
grabheader(s, name)
register char *s, *name;
{
	char	tmpbuf[LINESIZE];

	/*
	 * Copy the name into name
	 */
	while (isalpha(*s) || isdigit(*s) || *s == '-')
		*name++ = *s++;
	*name = '\0';

	/*
	 * Skip any whitespace
	 */
	while (isspace(*s))
		s++;

	/*
	 * This is a header if the next char is colon
	 */
	if (*s == ':') {
		s++;
		/* 
		 * This is probably illegal but we fail horribly if it
		 * happens - so we will guard against it
		 */
		if (*s != ' ' && *s != '\t' && *s != '\0') {
			/* we need to add a space */
			(void) strcpy(tmpbuf, s);
			(void) sprintf(s, " %s", tmpbuf);
		}
		return(s);	/* Return a pointer to the rest of the line */
	}

	return(NULL);
}

msgfix(from, date)
char *from, *date;
{
	register	char	*s;
	register	int	c;
			char	*rest, *lkp;
			char	name[LINESIZE], tmpbuf[LINESIZE];
			char	*grabline();
			int	haveheader = 0;	/* Do I have a header? */
			int	headertoken;

	/*
	 * Loop through all the headers
	 */
	while (1) {
	
		if (debug)
			printf("in msgfix about to grabline\n");

		s = grabline();

		if (debug)
			printf("got '%s'\n", s);

		/*
		 * Is this the end of the header?
		 */
		if (*s == '\0' || feof(stdin))
			break;


		/*
		 * Is this a continuation line?
		 */
		if (haveheader && isspace(*s)) {
			/* Note: Address munged headers handled specially */
			fprintf(rm_msgf, "%s\n", s);
		}
		else {
			/*
			 * Grab the header name from the line
			 */
			if ((rest = grabheader(s, name)) == NULL)
				/* Not a header therefore all headers done */
				break;

			haveheader = 1;

			/*
			 * Should I address munge this?
			 */
			if (headertoken = shouldmunge(name)) {
				char *finalstr;
				finalstr = "\n";
				fprintf(rm_msgf, "%s: ", name);
				/*
				 * deal specially with From lines
				 * the parser loses comments so we retain
				 * them specially here
				 */
				if (headertoken == HFROM
				 || headertoken == HRFROM) {
					if (lkp = strchr(rest, '<')) {
						*lkp = '\0';
						if (*rest == ' ')
							fprintf(rm_msgf, "%s<", rest+1);
						else
							fprintf(rm_msgf, "%s<", rest);
						*lkp = '<';
						finalstr = ">\n";
						(void) strcpy(tmpbuf, lkp);
						/*
						 * notice that we copy from the
						 * '<' to start with a space
						 */
						tmpbuf[0] = ' ';
						rest = tmpbuf;
						lkp = strrchr(rest, '>');
						if (lkp)
							*lkp = '\0';
					}
					if (lkp = strchr(rest, '(')) {
						*lkp = '\0';
						(void) sprintf(tmpbuf, " (%s\n", lkp+1);
						finalstr = tmpbuf;
					}
				}
				hadr_munge(rest);
				fputs(finalstr, rm_msgf);
			}
			else
				fprintf(rm_msgf, "%s\n", s);
		}
	}

	/*
	 * No From: line was given, create one based on the From's
	 */
	if (hdrselect[HFROM].hfound == 0) {
		fprintf(rm_msgf, "From: %s\n", from);
	}

	/*
	 * No Date: line was given, create one based on the From's
	 */
	if (hdrselect[HDATE].hfound == 0) {
		datecvt(date, tmpbuf);		/* Convert from uucp -> Arpa */
		fprintf(rm_msgf, "Date: %s\n", tmpbuf);
	}

	/*
	 * Copy the rest of the file, if there is any more
	 */
	if (!feof(stdin)) {
		/*
		 * If the first line of the message isn't blank
		 * output the blank separator line.
		 */
		if (*s)
			putc('\n', rm_msgf);
		fprintf(rm_msgf, "%s\n", s);
		while ((c = getchar()) != EOF) {
			if (ferror(stdin))
				fputs("\n  *** Problem during receipt from UUCP ***\n", rm_msgf);
			putc(c, rm_msgf);
		}
	}
}

char *
grabline()
{
	if (debug)
		printf("in grabline ");

	/*
	 * Grab the next line.
	 * Remembering this might be the tmpline
	 */
	if (usetmp) {
		if (debug)
			printf("using tmpline ");
		(void) strcpy(linebuf, tmpline);
		usetmp = 0;
	}
	else
		fgets(linebuf, sizeof(linebuf), stdin);

	/* Anything wrong? */
	if (ferror(stdin))
		fputs("\n  *** Problem during receipt from UUCP ***\n", rm_msgf);

	if (debug)
		printf("returning '%s'\n ", linebuf);

	return(linebuf);

}

nextchar()
{
	register	char	*s;	/* scratch pointer. */

	if (next != NULL && *next != '\0') {
		if (debug)
			printf("<%c>", *next);
		return(*next++);
	}

	/*
	 * The last buffer is now empty, fill 'er up
	 */
	next = grabline();

	if (feof(stdin) || !isspace(*next) || *next == '\0') {
		/* Yipee! We've reached the end of the address list. */
		ungetline(next);
		next = NULL;			/* Force it to read buffer */
		return(-1);
	}

	/* Zap excess whitespace */
	(void) compress(next, adrs);

	/*
	 * This may be a slightly duff list generated by some of the
	 * UNIX mailers eg: "x y z" rather than "x,y,z"
	 * If it is in this ^^^^^^^ convert to  ^^^^^^^ 
	 * NOTE: This won't handle list: "x<x@y> y<y@z>" conversions!
	 */
	if (strchr(adrs, ' ') != NULL &&
	    strchr(adrs, ',') == NULL &&
	    strchr(adrs, '<') == NULL &&
	    strchr(adrs, '(') == NULL &&
	    strchr(adrs, '"') == NULL) {
		for (s = adrs; *s; s++)
			if (*s == ' ')
				*s = ',';
	}
	next = adrs;
	/* This is a continuation line so return a seperator just in case */
	return(',');
}

/*
 * Munge an address list thats part of a header line. Try and get the
 * ap_* (MMDF) routines to do as much as possible for us.
 */
hadr_munge(list)
char *list;
{
	AP_ptr	the_addr;		/* ---> THE ADDRESS <--- */
	int	adrsout = 0;		/* How many address have been output */

	ungetline(list);

	while (1) {
		AP_ptr ap_pinit();

		the_addr = ap_pinit(nextchar);

		if (the_addr == (AP_ptr)NOTOK)
			bomb("cannot initialise address parser!");

		ap_clear();

		switch(ap_1adr()) {
		case NOTOK:
			/* Something went wrong!! */
			(void) ap_sqdelete(the_addr, (AP_ptr)NULL);
			ap_free(the_addr);
			/*
			 * Emergency action in case of parser break down:
			 * print out the rest of the line (I hope).
			 */
			fprintf(rm_msgf, "%s", tmpline);
			continue;

		case OK:
			/*
			 * I've got an address to output!
			 * Output a comma seperator between the addresses
			 * and a newline every once in a while to make
			 * sure the lines aren't too long
			 */
			if (adrsout++)
				fprintf(rm_msgf, ",%s ", adrsout%MAXADRS?"":"\n");

			/* Munge will do all the work to output it */
			munge(the_addr);

			/* Reclaim the space */
			(void) ap_sqdelete(the_addr, (AP_ptr)NULL);
			ap_free(the_addr);

			if (debug)
				printf("munged and space freed\n");
			break;

		case DONE:
			if (debug)
				printf("hadr_munge all done\n");

			/* Reclaim the space */
			(void) ap_sqdelete(the_addr, (AP_ptr)NULL);
			ap_free(the_addr);

			return;
		}
	}
}

/*
 * We now have a single address in the_addr to output.
 */
munge(the_addr)
AP_ptr the_addr;
{
	AP_ptr	local, domain, route;
	char	*s, *frees;
	char	adr[LINESIZE];
	char	*ap_p2s();

	if (debug)
		printf("in munge with: ");

	/*
	 * Find where the important bits begin in the tree
	 */
	(void) ap_t2parts(the_addr, (AP_ptr *)NULL, (AP_ptr *)NULL,
			&local, &domain, &route);

	/*
	 * Convert from the tree back into a string
	 */
	frees = s = ap_p2s((AP_ptr)NULL, (AP_ptr)NULL, local, domain, route);

	if (debug)
		printf("%s\n", s);

	/* Is it a uucp style address? */
	if (domain == (AP_ptr) 0 && strchr (s, '!') != (char *)NULL) {
		char	adr2[LINESIZE];

		(void) strcpy(adr, s);

		/*
		 * Stick the path it took to get here at the start.
		 * But try to avoid any duplicates by overlapping
		 * the matching parts of the address. Eg:
		 *	adr = 'x!y!z'  path = 'a!b!x!y!'
		 *	adr2 = 'a!b!x!y!z'
		 */
		for (s = origpath; *s; s++) {
			/* Does it match? */
			if (strncmp(adr, s, strlen(s)) == 0) {
				char c = *s;
				*s = '\0';
				(void) strcpy(adr2, origpath);
				(void) strcat(adr2, adr);
				*s = c;
				break;
			}
		}

		/*
		 * Did I just scan the whole path without finding a match!
		 */
		if (*s == '\0') {
			/* Append the adr to the path */
			(void) strcpy(adr2, origpath);
			(void) strcat(adr2, adr);
		}

		/* Munge the address into its shortest form and print it */
		fromcvt(adr2, adr);
		fprintf(rm_msgf, "%s", adr);

		if (debug)
			printf("uucp munge gives %s\n", adr);
	}
	else {
		extern	int	ap_outtype;
		AP_ptr	norm;
		char	*at, *brace, *fmt, *official, *p;
		char	*get_official();

		/*
		 * Normalise the address
		 */
		ap_outtype = AP_733;	/* Hmm. Maybe should be from channel */
		norm = ap_normalize(nextdoor, (char *)NULL, the_addr, chanptr);

		/* Convert it back in to a string and output it */
		(void) ap_t2s(norm, &p);
		if (debug)
			printf("Normalised address: %s\n", p);
		/*
		 * Look for the last address component and re-write to the
		 * official form. There are probably better ways of doing this
		 */
		at = strrchr(p, '@');
		fmt = "%s";
		if (at) {
			brace = strchr(at+1, '>');
			if (brace) *brace = '\0';
			official = get_official(at+1, (int *)NULL);
			if (official) {
				*at = '\0';
				fmt = brace ? "%s@%s>" : "%s@%s";
			}
			else if (brace)
				*brace = '>';
		}
		fprintf(rm_msgf, fmt, p , official);

		if (debug) {
			printf("arpa munge gives ");
			printf(fmt, p, official);
			printf("\n");
		}

		free(p);
	}
	free(frees);
}



/*
 * datecvt()  --  convert UNIX ctime() style date to ARPA style
 *			(change done in place)
 */
datecvt (date, newdate)
char *date;
char *newdate;
{
	/*
	 * LMCL: Changed the default timezone, when none given, to be GMT
	 *	012345678901234567890123456789
	 *	Wed Nov 18 22:23:17 1981	Unix
	 *	Wed, 18 Nov 81 22:23:45 GMT	ARPA
	 * or
	 *	Wed Nov 18 22:23:17 EDT 1981	Unix
	 *	Wed, 18 Nov 81 22:23:45 EDT	ARPA
	 * or even (LMCL yet again)
	 *	Mon Apr 23 16:50 BST 1984	Unix
	 */

	if (isdigit(date[0]) || date[3] == ',' || isdigit(date[5]))
		(void) strcpy(newdate, date);	/* Probably already ARPA */
	else if (isdigit(date[20]))
		(void) sprintf(newdate, "%.3s, %.2s %.3s %.2s %.2s:%.2s:%.2s GMT",
		    date, date+8, date+4, date+22, date+11, date+14, date+17);
	else if (isalpha(date[17]))		/* LMCL. Bum Unix format */
		(void) sprintf(newdate, "%.3s, %.2s %.3s %.2s %.2s:%.2s:00 GMT",
		    date, date+8, date+4, date+23, date+11, date+14);
	else
		(void) sprintf(newdate, "%.3s, %.2s %.3s %.2s %.2s:%.2s:%.2s %.3s",
		    date, date+8, date+4, date+26,
		    date+11, date+14, date+17, date+20);
}

/**/
msg_stash (dest, rep)
char *dest;
struct rp_bufstruct *rep;
{
	Err_stash	*es;

	/*
	 * I wanted to do this using a linked list
	 * but there was something peculiar in the
	 * Orion which meant that the code failed
	 */
	if (e_s_count >= MAXERRSTASH) {
		e_s_other++;
		return;
	}
	es = &e_stash[e_s_count++];
	(void) strcpy(es->e_s_dest, dest);
	es->e_s_reason = *rep;
}

msg_return()
{
	Err_stash	*ep;
	int		emax;
#ifdef	CITATION
	int		lines;
#endif
	
	ml_init(YES, NO, "UUCP (rmail)", "Failed Message");
	ml_adr(rm_from);
	ml_aend();
	ml_tinit();
	ml_txt("Your message was not delivered to:\n    ");
	for (ep = e_stash; ;) {
		ml_txt(ep->e_s_dest);
		ml_txt("\nFor the following reason:\n    ");
		to80(ep->e_s_reason.rp_line);
		ml_txt(ep->e_s_reason.rp_line);
		if (++ep < &e_stash[e_s_count])
			ml_txt("\n\nIt was also not delivered to:\n     ");
		else
			break;
	}
	if (e_s_other)
		ml_txt("\nThere were other addressing errors\n\n");
	e_s_other = 0;
	emax = e_s_count;	/* for non delivery */
	e_s_count = 0;
#ifdef CITATION
	ml_txt("\n\nYour message begins:\n\n");
	rewind(rm_msgf);
	while (fgets(linebuf, sizeof linebuf, rm_msgf) != NULL) {
		ml_txt(linebuf);
		if (linebuf[0] == '\n')
			break;
	}
	if (!feof(rm_msgf)) {
		lines = 0;
		for (lines = CITATION; --lines > 0 &&
			fgets(linebuf, sizeof linebuf, rm_msgf) != NULL;) {
		
			if (linebuf[0] == '\n')
				lines++;	/* truly blank lines don't count */
			ml_txt(linebuf);
		}
		if (!feof(rm_msgf))		/* if more, give an elipses */
			ml_txt("...\n");
	}
#else
	ml_txt("\n\n--------------- Returned Mail ---------------\n\n");
	rewind(rm_msgf);
	ml_file(rm_msgf);
	ml_txt("--------------- End of Returned Mail -------------\n");
#endif
	if (ml_end(OK) != OK) {
	
		ml_init(YES, NO, "UUCP (rmail)", "Failed Err Message");
		ml_adr(supportaddr);
		ml_aend();
		ml_tinit();
		ml_txt("Error message failed to:\n    ");
		ml_txt(rm_from);
		ml_txt("\n\nWhen failing to send to:\n    ");
		for (ep = e_stash; ;) {
			ml_txt(ep->e_s_dest);
			ml_txt("\nFor the following reason:\n    ");
			ml_txt(ep->e_s_reason.rp_line);
			if (++ep < &e_stash[emax])
				ml_txt("\nand failing to send to:\n     ");
			else
				break;
		}
		ml_txt("\n\n----------- Returned Mail --------------\n\n");
		rewind(rm_msgf);
		ml_file(rm_msgf);
		ml_txt("-------- End of Returned Mail ---------------\n");
		if (ml_end(OK) != OK)
			bomb("Failed to returnerror message to overseer");
	}
}

/*
 *	line formatter stolen from niftp/rtn_proc.c
 */
to80(from)
char *from;
{
	register	char	*p;
			char	*lastspace = NULL, *lastnl = from;

	for (p = from ; *p ; p++) {
		if (*p != ' ' && *p != '\t')
			continue;
		if (p - lastnl > 80) {
			if (lastspace != NULL) {
				*lastspace = '\n';
				lastnl = lastspace;
				if (p - lastnl <= 80) {
					 lastspace = p;
					continue;
				}
			}
			lastspace = NULL;
			*p = '\n';
			lastnl = p;
			continue;
		}
		lastspace = p;
	}
	if (p - lastnl > 80 && lastspace != NULL)
		*lastspace = '\n';
}

/*  */

xsubmit(from, argv, argc)
char *from;
char **argv;
int argc;
{
	register	char			*p, *q;
			struct	rp_bufstruct	thereply;
			char			buf[LINESIZE];
			char			subargs[LINESIZE];
			char			*un_bang();
			int			len;

	rewind (rm_msgf);

	if (debug) {
		for (argv++; --argc > 0; argv++)
			printf("arg = %s\n", un_bang(*argv));
		while (fgets(buf, LINESIZE-1, rm_msgf) != NULL)
			printf("T=%s", buf);
		if (ferror(rm_msgf))
			bomb("Error reading message file");

		exit(0);
	}

	if (rp_isbad (mm_init()) || rp_isbad (mm_sbinit ()))
		bomb("Failed to initialise submit");

	/* Set nameserver timeout up high -- DSH */
	(void) sprintf(subargs, "lvmti%s*k30*h", uchan);

	if (nextdoor[0]) {
		q = subargs + strlen(subargs);
		for (p = nextdoor; *p;) {
			if (!isprint(*p)) {
				p++;
				continue;
			}
			if (*p == '\'') *q++ = '\\';
			*q++ = *p++;
		}
		*q = '\0';
	}
	(void) strcat(subargs, "*");

	if (rp_isbad(mm_winit((char *)NULL, subargs, from)))
		bomb("mm_winit(%s, %s) failed", subargs, from);
	if (rp_isbad(mm_rrply(&thereply, &len)))
		bomb("Failed to read address reply");
	if (rp_isbad(thereply.rp_val))
		bomb("Initialization failure '%s'", thereply.rp_line);

	for (argv++; --argc > 0; argv++) {
		char	*name;

		name = un_bang(*argv);
		if (rp_isbad(mm_wadr("", name)))
			bomb("failed to write address '%s'", name);

		if (rp_isbad(mm_rrply(&thereply, &len)))
			bomb("Failed to read address reply");

		switch (rp_gval(thereply.rp_val)) {
		case RP_DOK:
		case RP_AOK:
			break;

		case RP_USER:
			msg_stash(name, &thereply);
			break;		/* LMCL: Was return*/

		default:
			bomb("Address check failure '%s' (%s)",
				thereply.rp_line,
			rp_valstr(thereply.rp_val));
		}
	}

	if (rp_isbad(mm_waend()))
		bomb("Failed to write address end");
	rewind(rm_msgf);
	while (fgets(buf, LINESIZE-1, rm_msgf) != NULL)
		if (rp_isbad(mm_wtxt(buf, strlen(buf))))
			bomb("mm_wtxtfailure");
	if (ferror(rm_msgf))
		bomb("Error reading messge file");

	if (rp_isbad(mm_wtend()))
		bomb("mm_wtend error");

	if (rp_isbad(mm_rrply(&thereply, &len)))
		bomb("mm_rrply at end of message");

	if (rp_isbad(thereply.rp_val))
		bomb("Bad text reply val '%s' (%s)", thereply.rp_line,
				rp_valstr(thereply.rp_val));

	mm_sbend();
	if (e_s_count)
		msg_return();
	return;
}

/*VARARGS1*/
bomb(fmt, a, b, c)
char *fmt, *a, *b, *c;
{
	if (e_s_count)
		msg_return();
	ll_log(logptr, LLOGFAT, fmt, a, b, c);
	ll_close(logptr);
	fputs("rmail: ", stderr);
	fprintf(stderr, fmt, a, b, c);
	putc('\n', stderr);
	exit(99);
}

/*
 * fromcvt()  --  trys to convert the UUCP style path into an ARPA style name.
 */
fromcvt(from, newfrom)
char *from, *newfrom;
{
	register	char	*cp;
	register	char	*sp;
	register	char	*off;
			char	*at, *atoff;
			char	atstore[LINESIZE], buf[LINESIZE];
			char	*get_official();
			int	saveit;
	
	if (debug)
		printf("fromcvt on %s\n", from);

	(void) strcpy(buf, from);
	cp = strrchr(buf, '!');
	if (cp == 0) {
		(void) strcpy(newfrom, from);
		return;
	}
	/*
	 *	look for @site at the end of the name
	 */
	atoff = NULL;
	if ((at = strchr(cp, '@')) != NULL) {
		/* got one - is it followed by a ! ? */
		if (strchr(at+1, '!') != NULL)
			 at = NULL;
		else {
			/* look up the official name of the at site */
			atoff = get_official(at+1, &saveit);
			if (atoff) {
				if (saveit) {
					(void) strcpy(atstore, atoff);
					atoff = atstore;
				}
			}
		}
	}
	*cp = 0;
	while (sp = strrchr(buf, '!')) {
		/*
		 * scan the path backwards looking for hosts that we
		 * know about
		 */
		if (off = get_official(sp+1, &saveit)) {
			if (atoff && lexequ(atoff, off))
				(void) strcpy(newfrom, cp+1);
			else
				(void) sprintf(newfrom, "%s@%s", cp+1, off);
			return;
		}
		else if (at && lexequ(at+1, sp+1)) {
			(void) strcpy(newfrom, cp+1);
			return;
		}
		*cp = '!';
		cp = sp;
		*cp = 0;
	}
	if (off = get_official(buf, (int *)NULL))
		(void) sprintf(newfrom, "%s@%s", cp+1, off);
	else
		(void) sprintf(newfrom, "%s@%s", cp+1, buf);
}

/*
 *	This piece added by Peter Collinson (UKC)
 *	rather than just making rmail submit into the uucp channel which
 *	is hard coded. First look up in a table 'rmail.chans' for entries
 *	of the form
 *	nextdoor:channel
 *	If the name exists then use that channel otherwise just default to
 *	the uucp channel. This is for authorisation purposes really
 */
set_channel(nxt)
char *nxt;
{
	Table	*tb_rmchans;
	char	tuchan[LINESIZE];

#ifdef DEBUG
	ll_log(logptr, LLOGBTR, "set_channel(%s)", nxt);
#endif
	/*
	 * set default
	 */
	(void) strcpy(uchan, Uchan);
	
	if ((tb_rmchans = tb_nm2struct("rmail.chans")) != (Table *)NOTOK) {
		/*
		 * Well, we have the table. Extract name of current channel
		 * from list of channels
		 */
		if (tb_k2val(tb_rmchans, TRUE, nxt, tuchan) == OK)
			(void) strcpy(uchan, tuchan);
	}

	/*
	 * Get the channel attributes
	 */
	if ((chanptr = ch_nm2struct(uchan)) == (Chan *)NOTOK) {
		/* fail safe - not sure that we should do this */
		/* find the default channel */
		if ((chanptr = ch_nm2struct(Uchan)) == (Chan *)NOTOK)
			bomb("Cannot look up channel '%s'\n", uchan);
	}
}

/*
 *	Check if the from machine which we have constructed actually
 *	contains two references to the same machine of the form
 *	site.uucp!site.??.uk!something
 *	if so replace by the single official name
 *
 * This can fail can't it? -- DSH
 */
domain_cross(route)
char *route;
{
	char	*enda, *endsecond, *official, *second;
	char	*host_equal();

	second = strchr(route, '!');
	if (second == (char *)NULL)
		return;
	second++;
	endsecond = strchr(second, '!');
	if (endsecond == (char *)NULL)
		return;
	second[-1] = '\0';
	*endsecond = '\0';
	if (official = host_equal(route, second)) {
		enda = strdup(endsecond+1);
		(void) strcpy(route, official);
		(void) strcat(route, "!");
		(void) strcat(route, enda);
		free(enda);
	}
	else {
		second[-1] = '!';
		*endsecond = '!';
	}
}

/*
 *	host_equal
 *	a routine to see whether the strings which are input
 *	are in fact the same host
 *	Returns the official name if they are and a null pointer if not
 */
char *
host_equal(n1, n2)
char *n1, *n2;
{
	static	char	res1[LINESIZE];
		char	*o1, *o2;
		char	*get_official();
		int	saveit;
	
	o1 = get_official(n1, &saveit);
	if (o1 == NULL)
		return(NULL);
	if (saveit) {
		(void) strcpy(res1, o1);
		o1 = res1;
	}
	o2 = get_official(n2, &saveit);
	if (o2 == NULL)
		return(NULL);
	if (lexequ(o1, o2) == TRUE)
		return(o1);
	return(NULL);
}

/*  */

/*
 *	These routines perform some caching on the host name/official
 *	name pair in and attempt to reduce overheads
 */
Official *off_cache;			/* pointer to the base of the cache */
#define OFFICIALSIZE	50		/* Chunk size for the cache */
int	off_size;			/* current size of the cache */
int	off_used;			/* current set which are used */
/*
 *	get a host from the domain tables
 *	if the cache has fallen over due to lack of memory
 *	then the saveit switch will be set showing that the
 *	address is in static memory
 */
char *
get_official(host, saveit)
char *host;
int *saveit;				/* set if the cache is not working */
{
	register	Official	*off;
	static		char		res[LINESIZE];
			Domain		*dmn;
			Domain		*dm_v2route();
			Dmn_route	rbuf, *route = &rbuf;
			int		sadummy;

	if (debug)
		printf("get_official called for %s\n", host);

	if (saveit == (int *)NULL)
		saveit = &sadummy;
	*saveit = 0;			/* assume cache is working */
	
	if (off_size == 0) {
		off_cache = (Official *)malloc(OFFICIALSIZE*sizeof(Official));
		off_size = OFFICIALSIZE;
		if (debug)
			printf("Cache%sset - %d items\n",
				off_cache ? " " : " not " ,
				off_cache ? off_size : 0);
	}

	/*
	 * If we have managed to get space for the cache 
	 * search it for the name
	 */
	if (off_cache == (Official *)NULL)
		*saveit = 1;
	else {
		for (off = off_cache; off < &off_cache[off_used]; off++)
			if (*host == *off->o_host && lexequ(host, off->o_host)) {
				if (debug)
					printf("Cache hit - %s\n", off->o_official);
				return(off->o_official);
			}
	}

	/*
	 * Bad news - we must do something to get the information
	 */
	dmn = dm_v2route(host, res, route);
	if (dmn == (Domain *)NOTOK) {
		if (debug)
			printf("%s not in tables\n", host);
		return(NULL);
	}

	/*
	 * we have a hit in the tables
	 * save it in the cache
	 */
	if (off_used >= off_size) {
		off_size += OFFICIALSIZE;
		/*NOSTRICT*/
		off_cache = (Official *)realloc(off_cache,off_size * sizeof(Official));
		if (debug) {
			if (off_cache)
				printf("Cache grown to %d items\n", off_size);
			else
				printf("Cache grow failed\n");
		}
	}

	if (off_cache) {
		off = &off_cache[off_used];
		if ((off->o_host = strdup(host)) &&
		    (off->o_official = strdup(res))) {
			off_used++;
			if (debug) {
				printf("%s - %s stored in cache - cachesize %d\n",
					off->o_host,
					off->o_official,
					off_used);
			}
			return(off->o_official);
		}	
	}

	*saveit = 1;
	if (debug)
		printf("%s - %s not cached\n", host, res);
	return(res);
}

/*
 *	Added Peter Collinson June 1987
 *	take a possibly pure bang address and convert it into
 *	733 % @ format
 *
 *
 *	The above is unnecessary and triggers problems, such as
 *	propagation of
 *		rmail oddjob!ncar!mailrus!utai!watmath!looking!brad
 *	as
 *		rmail ncar!brad%looking%watmath%utai%mailrus
 *
 *	Instead of converting	x!y!z!user to user%z%y@x, convert
 *	it into y!z!user@x.  This is perfectly fine for systems
 *	that obey RFC-976 (which MMDF is one of).
 *		-- JG
 */
char *
un_bang(adr)
register char *adr;
{
	register	char	*bangptr, *dest;
	
	/*
	 * get out of anything other than bangs
	 */
	if (strchr(adr, '@') || strchr(adr, '%') || strchr(adr, ':'))
		return(adr);
	/*
	 * get out if no '!'
	 */
	if ((bangptr = strchr(adr, '!')) == (char *)NULL)
		return(adr);

	/*
	 * convert "a!b!c!user" into "b!c!user@a"
	 */
	dest = malloc(strlen(adr+16));
	(void) strcpy(dest, bangptr+1);
	(void) strcat(dest, "@");
	*bangptr = '\0';
	(void) strcat(dest, adr);
	(void) strcpy(adr, dest);
	free(dest);
	return(adr);
}
