/*
 *     MULTI-CHANNEL MEMO DISTRIBUTION FACILITY  (MMDF)
 *
 *
 *     Copyright (C) 1979,1980,1981  University of Delaware
 *
 *     Department of Electrical Engineering
 *     University of Delaware
 *     Newark, Delaware  19711
 *
 *     Phone:  (302) 738-1163
 *
 *
 *     This program module was developed as part of the University
 *     of Delaware's Multi-Channel Memo Distribution Facility (MMDF).
 *
 *     Acquisition, use, and distribution of this module and its listings
 *     are subject restricted to the terms of a license agreement.
 *     Documents describing systems using this module must cite its source.
 *
 *     The above statements must be retained with all copies of this
 *     program and may not be removed without the consent of the
 *     University of Delaware.
 *     
 *
 *     version  -1    David H. Crocker    March   1979
 *     version   0    David H. Crocker    April   1980
 *     version  v7    David H. Crocker    May     1981
 *     version   1    David H. Crocker    October 1981
 *
 */
/*  CHECKQUE:  Peruse status of mail queue
 *
 *  Oct 81  D. Crocker          Initial coding
 *  Nov 81  D. Crocker          message name must begin "msg."
 *                              show hours since last contact
 *                              show size of aquedir directory
 *  Apr 82  D. Crocker          show last pickup in elapsed hours, also
 *  Jun 82  S. Manion           added flags s,p,z,f, and t.
 *                              rearanged output format.
 *  Jul 82  D. Crocker          convert to mq_ package
 *  Mar 83  Doug Kingston       changed to use format independent directory
 *                              access routines a la 4.2BSD.  (libndir.a)
 *  Sep 84  D. Rockwell		-c only looks at relevant queues
 *  Oct 86  S. Kille            -h gives list of hosts
 *  Oct 88  E. Bennett		converted to use getopt(3)
 */

#include "util.h"
#include "mmdf.h"
#include <sys/stat.h>
#include <utmp.h>
#include "ch.h"
#include "msg.h"
#include "adr_queue.h"
#include "phs.h"
#ifdef SYS5
#include <string.h>
#else
#include <strings.h>
#endif

#define WARNTIME	(60L * 60L * 24) /* 1 day */

extern time_t time();

time_t	curtime;		/* Current time secconds		*/
time_t	toolate;		/* Time to wait before panicing		*/

extern	char	*quedfldir,	/* home directory for mmdf		*/
		*mquedir,	/* subordinate message directory	*/
		*squepref,	/* subordinate queue prefix		*/
		*aquedir;	/* subordinate address directory	*/
extern	char	*strdup();

extern	Chan	**ch_tbsrch;	/* full list of channels		*/

extern	int	ch_numchans;	/* number of channels which are known	*/
extern	int	errno;		/* system error number			*/

LOCFUN mn_dirinit();

struct ch_hostat
{
	char			*hname;
	time_t			h_oldest;
	int			hcount;
	struct	ch_hostat	*next;
};


struct ch_chstat
{
	int	ch_nummsgs;
	long	ch_bytes;
	time_t	dstrt,
		dmsg,
		ddone,
		pstrt,
		pmsg,
		pdone,
		oldest,			/* oldest message in queue */
		llog;
	char	oldmsg[15];		/* name of oldest msg */
	struct	ch_hostat	*hlist; /* detailed host info */
} ch_stat[NUMCHANS];

long	msglen,			/* length of the current message	*/
	totblks;		/* number of blocks (1K) of msg data	*/

DIR	*ovr_dirp;		/* handle on the queue directory	*/

int	msg_total;		/* total in the queue			*/
long	msg_dirsiz;		/* total spaces in aquedir		*/

char	msg_sender[ADDRSIZE + 1]; /* return address of current message	*/
char	bufout[BUFSIZ];
char	*program;

int	ssflag,			/* print site summary lines only	*/
	nzflag,			/* print only non-zero sites		*/
	pcflag,			/* print only problem channels		*/
	fflag,			/* print name of first queued msg	*/
	chflag,			/* print only specified channels	*/
	hoflag;			/* give detailed host info		*/

int	nchan;
Chan	*chcodes[NUMCHANS];

/*****  (mn_)  MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN MAIN  ***** */

main(argc, argv)
int argc;
char *argv[];
{
	extern	char	*dupfpath();

	program = argv[0];

	mmdf_init(argv[0]);
	setbuf(stdout, bufout);
	if (ch_numchans > NUMCHANS)
	{
		printf("%s: Struct ch_stat[NUMCHANS] not large enough - recompile\n",
			argv[0]);
		exit(-1);
	}

	siginit();		/* standard interrupt initialization	*/
				/* distinguish different delivers	*/
	flaginit(argc, argv);	/* initialize the flags			*/

	mn_dirinit();		/* get right working directory		*/

	ovr_queue();		/* do the entire mail queue		*/

	ch_callstat();
	ch_report();

	exit(RP_OK);
}

flaginit(argc, argv)		/* initialize flags */
int argc;
char **argv;
{
	register	char	*p;
	register	int	ch;
	extern		char	*optarg;
	extern		int	optind;

	chflag = fflag = ssflag = pcflag = nzflag = hoflag = 0;
	nchan = 0;
	/*NOSTRICT*/
	toolate = WARNTIME;
	while ((ch = getopt(argc, argv, "c:fhpst:z")) != EOF) {
		switch (ch) {
		case 'c':	/*  channel flag  */
			chflag++;
			while ((p = index(optarg, ',')) != (char *)NULL) {
				*p = '\0';
				add_chan(optarg);
				optarg = ++p;
			}
			add_chan(optarg);
			break;

		case 'f':	/*  print name of first queued msg  */
			fflag++;
			break;

		case 'h':	/* print detailed host info */
			hoflag++;
			break;

		case 'p':	/*  problem channel flag  */
			pcflag++;
			break;

		case 's':	/*  site summary flag  */
			ssflag++;
			break;

		case 't':	/*  set time on problem channels  */
			pcflag++;	/* implied p option */

			/*
			 * We want toolate to be in seconds. We start off
			 * assuming that the given argument was in minutes.
			 */
			toolate = atoi(optarg) * 60L;
			/* check for a trailing M to indicate minutes */
			while (isdigit(*optarg))
				optarg++;
			if (*optarg != 'm' && *optarg != 'M')
				/* must be in hours */
				toolate *= 60L;
			break;

		case 'z':	/*  non-zero flag  */
			nzflag++;
			break;

		default:	/*  abzorb unrecognized flags  */
			Usage();
			exit(-1);
			break;
		}
	}

	(void) fflush(stdout);
	return(0);
}

add_chan(channel)
char *channel;
{
	/* Is there room for the channel name? */
	if (nchan == NUMCHANS) {
		printf("%s: can only specify %d channels.\n",
			program, NUMCHANS);
		return;
	}

	/* Convert from the ch_spec form to structure pointer */
	if ((chcodes[nchan++] = ch_nm2struct(channel)) == (Chan *)NOTOK) {
		printf("%s: unknown channel: %s\n",
			program, channel);
		(void) fflush(stdout);
	}
}

LOCFUN
mn_dirinit()		/* get to the working directory		*/
{
	if (chdir(quedfldir) < OK)
	{
		printf("%s: can't chdir to %s\n", program, quedfldir);
		exit(-1);
	}
}

/************  (ovr_)  OVERALL SEQUENCING MANAGEMENT  *************** */

LOCFUN
ovr_ismsg(entry)	/* a processable message?		*/
register struct dirtype *entry;
{
	return((
#ifdef BSDDIRECT
	entry->d_namlen < MSGNSIZE
#else
	(strlen(entry->d_name) < MSGNSIZE)
#endif
	    && equal(entry->d_name, "msg.", 4)) ? TRUE : FALSE );
}

ovr_queue()			/* Process entire message queue		*/
{
	register	int	index;
			char	qname[128];

	time(&curtime);		/* Get current time (in hours)		*/

	if (chflag != 0)
	{
		for (index = 0; index < nchan; index++)
		{
			(void) strcpy(qname, squepref);
			strcat(qname, chcodes[index]->ch_queue);
			ovr_aqueue(qname);
		}
	} else
		ovr_aqueue(aquedir);
}

ovr_aqueue(ch)			/* Process a single message queue	*/
register char *ch;		/* queue directory name			*/
{
	register	struct	dirtype	*dp;
			struct	stat	statbuf;

	stat(ch, &statbuf);
	msg_dirsiz += st_gsize(&statbuf);

	if ((ovr_dirp = opendir (ch)) == NULL)
	{
		printf("%s: can't open queue %s\n", program, ch);
		return;
	}

	while ((dp = readdir (ovr_dirp)) != NULL)
	{			/* straight linear processing		*/
		if (ovr_ismsg (dp))
			msg_proc (dp -> d_name);
	}
	closedir(ovr_dirp);
}
/****************  (msg_)  HANDLE A SINGLE MESSAGE  ***************** */

msg_proc(thename)		/* get, process & release a msg		*/
char thename[];
{
	Msg		curmsg;
	struct	stat	statbuf;
	char		msgpath[28];

	(void) sprintf(msgpath, "%s%s", mquedir, thename);
	if (stat(msgpath, &statbuf) < 0) {
		printf("\n*** %s: Couldn't stat;  error %d.\n", thename, errno);
		(void) fflush(stdout);
		msglen = 0;
	} else {
		msglen = st_gsize(&statbuf);
		if (msglen == 0)
		{
			printf("\n*** %s: msg length of zero.\n", thename);
			(void) fflush(stdout);
		} else {
			totblks += ((msglen+1023L)/1024L);
			msg_total++;
		}
	}

	/*
	 *  Check for real anomalies!
	 */
	(void) strcpy(curmsg.mg_mname, thename);

	if (mq_rinit((Chan *)0, &curmsg, msg_sender) == OK)
	{
		if (adr_each(&curmsg) == 0)
		{
			printf("\n*** %s: msg exists (%d chars), but goes nowhere.\n",
				thename, msglen);
			(void) fflush(stdout);
		}
		mq_rkill(OK);
	}
}

/************  (adr_)  INDIVIDUAL ADDR DELIVERY ATTEMPT  ************ */

adr_each(themsg)		/* do each address			*/
Msg *themsg;
{
	register	int			naddrs, curind;
			Chan			*curch, *newch;
			struct	adr_struct	newadr;
			char			curhost[LINESIZE];
			char			oldque[LINESIZE];

	curhost[0] = '\0';
	oldque[0] = '\0';
	for (naddrs = 0, curch = (Chan *)0, mq_setpos(0L);;)
		switch(mq_radr(&newadr))
		{
		default:		/* should not get this		*/
			printf ("\n*** %s: bad return from mq_radr\n", themsg -> mg_mname);
			exit(NOTOK);

		case NOTOK:		/* rest of list must be skipped	*/
		case DONE:		/* end of list			*/
			return(naddrs);

		case OK:		/* normal addr acquisition	*/
			naddrs++;
			if (strcmp(newadr.adr_que, oldque) != 0 &&
			    (newch = ch_qu2struct(newadr.adr_que)) == (Chan *)NOTOK)
			{
				printf("\n*** Unknown channel queue ('%s') in message '%s'\n",
				    newadr.adr_que, themsg->mg_mname);
				(void) fflush(stdout);
			}
			else
			{
				(void) strcpy(oldque, newadr.adr_que);
				if (newadr.adr_delv != ADR_DONE)
				{
					if (curch != newch)
					{
						curch = newch;
						curind = ch_find(curch);
						ch_newmsg(curind, themsg);
						if (hoflag)
						{
							if (curch->ch_host == (char *)0)
							{
								/* code folded from here */
								(void) strcpy(curhost, newadr.adr_host);
								ch_newhost(curind, curhost, themsg->mg_time);
								/* unfolding */
							}
#ifdef notdef
							else
								/* code folded from here */
								ch_newhost(curind, curch->ch_host, themsg->mg_time);
#endif notdef
								/* unfolding */
						}
					}
					else
					{
						if (hoflag
						    && curch->ch_host == (char *)0
						    && !lexequ(curhost, newadr.adr_host))
						{
							(void) strcpy(curhost, newadr.adr_host);
							ch_newhost(curind, curhost, themsg->mg_time);
						}
					}
				}
			}
		}
}
/**/
ch_find(thech)
Chan *thech;
{
	register	int	chind;

	for (chind = 0; chind < ch_numchans; chind++)
		if (ch_tbsrch[chind] == thech)
			return(chind);

	printf ("\n*** Could not locate channel ('%s') in channel list\n",
		thech->ch_name);
	(void) fflush(stdout);

	return(-1);
}

ch_newmsg(theind, themsg)
int theind;
Msg  *themsg;
{
	if (theind < 0)
		return;
	ch_stat[theind].ch_nummsgs++;
	ch_stat[theind].ch_bytes += msglen;
	if (themsg -> mg_time < ch_stat[theind].oldest ||
		ch_stat[theind].oldest == 0)
	{
		ch_stat[theind].oldest = themsg->mg_time;
		(void) strcpy(ch_stat[theind].oldmsg, themsg->mg_mname);
	}
	return;

}
/**/

ch_newhost(theind, thehost, age)
int theind;
char *thehost;
time_t age;
{
	struct	ch_hostat	*hptr;
	char			*ctime();

	if (theind < 0)
		return;

	if (ch_stat[theind].hlist == (struct ch_hostat *)0)
	{
		hptr = (struct ch_hostat *)malloc(sizeof(struct ch_hostat));
		ch_stat[theind].hlist = hptr;
		hptr->hname = strdup(thehost);
		hptr->h_oldest = age;
		hptr->hcount = 1;
		hptr->next = (struct ch_hostat *)0;
		return;
	}
	for (hptr = ch_stat[theind].hlist;; hptr = hptr -> next)
	{
		if (lexequ(thehost, hptr->hname))
		{
			hptr->hcount++;
			if (age < hptr->h_oldest)
				hptr->h_oldest = age;
			return;
		}
		if (hptr->next == (struct ch_hostat *)0)
			break;
	}
	hptr->next = (struct ch_hostat *)malloc(sizeof(struct ch_hostat));
	hptr = hptr->next;
	hptr->hname = strdup(thehost);
	hptr->h_oldest = age;
	hptr->hcount = 1;
	hptr->next = (struct ch_hostat *)0;
	return;
}
/**/

ch_callstat()
{
	register	int		chind;
	extern		struct	utmp	*getllnam();
			struct	utmp	*llogptr;

	for (chind = 0; chind < ch_numchans; chind++)
	{
		if (ch_tbsrch[chind]->ch_login != NOLOGIN)
		{
			setllog();	/* setup for checking last logins */
			if ((llogptr = getllnam(ch_tbsrch[chind]->ch_login)) != NULL)
				/*NOSTRICT*/
				ch_stat[chind].llog = llogptr->ut_time;
		}

		ch_stat[chind].dstrt = phs_get(ch_tbsrch[chind], PHS_WRSTRT);
		ch_stat[chind].dmsg  = phs_get(ch_tbsrch[chind], PHS_WRMSG);
		ch_stat[chind].ddone = phs_get(ch_tbsrch[chind], PHS_WREND);
		ch_stat[chind].pstrt = phs_get(ch_tbsrch[chind], PHS_RESTRT);
		ch_stat[chind].pmsg  = phs_get(ch_tbsrch[chind], PHS_REMSG);
		ch_stat[chind].pdone = phs_get(ch_tbsrch[chind], PHS_REEND);
	}

	endllog();
}
/**/

ch_report()
{
	register	int	chind;
	register	int	index;
	register	Chan	*chptr;
			time_t	age, timestamp[7], overstamp[7];
			char	*tptr;
			char	*ctime();
			char	tmp[LINESIZE], *textout[7];
			int	prtpos,	prtflag;

	tptr = ctime(&curtime);
	tptr[16] = '\0';
	printf("\n%s:  %d queued msgs / %ld byte queue directory\n",
		tptr, msg_total, msg_dirsiz);
	printf("\t\t   %ld Kbytes in msg dir\n\n", totblks);

	prtflag = 0;
	for (chind = 0; chind < ch_numchans; chind++)
	{
		/*  If the non-zero flag is set, then make sure the number
		 *  of messages on this channel is non-zero.
		 */
		if (nzflag != 0)
			if (ch_stat[chind].ch_nummsgs == 0)
				continue;

		/*  An often used variable  */
		chptr = ch_tbsrch[chind];

		/*  If specific channels were requested, make sure this is
		 *  one of them.
		 */
		if (chflag != 0)
		{
			for (index = 0;  index < nchan;  index++)
				if (chptr == chcodes[index])
					break;
			if (index >= nchan)
				continue;
		}

		/*  find time information  */

		for (index = 0; index <= 6; index++)
			overstamp[index] = (time_t)0;

		if (chptr->ch_login == NOLOGIN) {
			textout[1] = "deliver start";
			textout[2] = "deliver message";
			textout[3] = "deliver end";
			textout[4] = "pickup start";
			textout[5] = "pickup message";
			textout[6] = "pickup end";
			timestamp[0] = (time_t) 0;
			timestamp[1] = ch_stat[chind].dstrt;
			timestamp[2] = ch_stat[chind].dmsg;
			timestamp[3] = ch_stat[chind].ddone;
			timestamp[4] = ch_stat[chind].pstrt;
			timestamp[5] = ch_stat[chind].pmsg;
			timestamp[6] = ch_stat[chind].pdone;
			if (ch_stat[chind].ch_nummsgs > 0 && timestamp[3] != (time_t) 0)
				overstamp[3] = curtime - timestamp[3];
			if (timestamp[6] != (time_t) 0)
				overstamp[6] = (curtime - timestamp[6])/2;
		} else {
			sprintf(tmp, "%-8s login", chptr->ch_login);
			textout[0] = tmp;
			timestamp[0] = ch_stat[chind].llog;
			if (timestamp[0] != (time_t) 0)
				overstamp[0] = curtime - timestamp[0];
			textout[1] = "pickup start";
			textout[2] = "pickup message";
			textout[3] = "pickup end";
			textout[4] = "deliver start";
			textout[5] = "deliver message";
			textout[6] = "deliver end";
			timestamp[1] = ch_stat[chind].pstrt;
			timestamp[2] = ch_stat[chind].pmsg;
			timestamp[3] = ch_stat[chind].pdone;
			timestamp[4] = ch_stat[chind].dstrt;
			timestamp[5] = ch_stat[chind].dmsg;
			timestamp[6] = ch_stat[chind].ddone;
			if (ch_stat[chind].ch_nummsgs > 0 && timestamp[6] != (time_t) 0)
				overstamp[6] = curtime - timestamp[6];
			if (timestamp[3] != (time_t) 0)
				overstamp[3] = (curtime - timestamp[3])/2;
		}

		/*  how long ago was the completion?  */
		prtpos = -1;
		for (index=0; index <= 6; index++)
			if (timestamp[index] != (time_t)0)
				prtpos = index;
		if (prtpos >= 0)
			age = curtime - timestamp[prtpos];
		else
			age = (time_t)0;

		/*  if problem channel flag is set, check age  */
		if (pcflag != 0)
		{
			if (age < toolate)
				continue;
			else
				prtflag++;
		}

		prtinfo(chind, timestamp, overstamp, textout, age, prtpos);
	}
	if ((pcflag != 0 ) && (prtflag == 0))
		printf("No problems with any channels.\n");
}

prtinfo(chind, timestamp, overstamp, textout, age, prtpos)
int chind, prtpos;
time_t timestamp[], overstamp[];
char *textout[];
time_t age;
{
	register	Chan			*chptr;
			struct	ch_hostat	*hptr;
			time_t			ageh;
			char			*tptr, *ctime();
			int			temp, line, toploop;

	chptr = ch_tbsrch[chind];

	/*  print the site summary line  */
	if(ssflag == 0)
		putchar('\n');
	else
	{
		/*  print a star to indicate overdue channels  */
		if ((age != (time_t)0) && (age > toolate))
			printf("* ");
		else
			printf("  ");
	}
	/*NOSTRICT*/
	temp = (ch_stat[chind].ch_bytes + 500) / 1000;
	if ((ch_stat[chind].ch_bytes != 0) && (temp == 0))
		temp++;
	/*NOSTRICT*/
	ageh = age / (60L * 60L);
	printf ("%3d msg%s %4d Kb ",
		ch_stat[chind].ch_nummsgs,
		((ch_stat[chind].ch_nummsgs == 1) ? " " : "s"),
		temp);
	if (ssflag != 0)
	{
		if ((age != (time_t)0))
			printf("%5ld hour%s ", ageh, ((ageh == 1)  ?  " " : "s"));
		else
			if (chptr->ch_login == NOLOGIN)
				printf("   inactive ");
			else
				printf("   no login ");
	}

	printf("(%-8s) %-15s :  %s\n",
		chptr->ch_queue, chptr->ch_name, chptr->ch_show);

	/*  If only a summary was requested, then go on  */
	if (ssflag != 0)
	{
		/*  print oldest file if specifically requested  */
		if(fflag != 0)
			printf("    oldest msg:  %s\n", ch_stat[chind].oldmsg);
		return;
	}

	toploop = ((chptr->ch_login==NOLOGIN) &&
		((chptr->ch_access&CH_PICK)!=CH_PICK) &&
		(timestamp[4] == (time_t)0) &&
		(timestamp[5] == (time_t)0) &&
		(timestamp[6] == (time_t)0)) ? 3 : 6;
	for (line = (chptr->ch_login != NOLOGIN ? 0 : 1); line <= toploop; line++)
	{
		overdue(overstamp[line]);

		if (timestamp[line] == (time_t)0)
			printf("No %-23s", textout[line]);
		else
		{
			tptr = ctime(&timestamp[line]);
			tptr[16] = '\0';
			printf("%-26s:  %s", textout[line], tptr);
		}

		if (line == prtpos)
			printf(" / %ld hour%s\n",  ageh, ((ageh == 1)  ?  "" : "s"));
		else
			putchar('\n');
	}

	if (((timestamp[1] > (time_t)0) && (timestamp[3] > (time_t)0) &&
	    (timestamp[1] > timestamp[3])) || 
	    ((timestamp[4] > (time_t)0) && (timestamp[6] > (time_t)0) &&
	    (timestamp[4] > timestamp[6])))
		printf("  *** INCOMPLETE **\n");

	/*  print oldest file in queue if requested  */
	if ((fflag != 0) && (ch_stat[chind].oldest != (time_t)0))
		printf("\t\t  oldest msg                :  %s\n", ch_stat[chind].oldmsg);

	if (ch_stat[chind].ch_nummsgs != 0 &&
	    ch_stat[chind].oldest < (curtime - toolate))
	{
		tptr = ctime (&(ch_stat[chind].oldest));
		tptr[16] = '\0';

		printf("  *** WAITING **  First message             :  %s\n", tptr);
	}

	if ((hoflag != 0) && (chptr->ch_host == (char *)0))
	{
		for (hptr = ch_stat[chind].hlist; hptr != (struct ch_hostat *)0;
		    hptr = hptr->next)
		{
			tptr = ctime(&(hptr->h_oldest));
			tptr[16] = '\0';
			printf("    %-32s (%3d)  :  %s", hptr->hname,
				hptr->hcount, tptr);
			if ((curtime - hptr->h_oldest) > toolate)
				printf("  ** BLOCKING **\n");
			else
				printf("\n");
		}
	}
}

overdue(age)
time_t age;
{
	if (age == (time_t)0 || age <= toolate) {
		printf("\t\t  ");
		return(0);
	}

	printf("  *** OVERDUE **  ");
	return;
}

Usage()
{
	printf("Usage: %s [-z] [-s] [-p] [-f] [-h] [-t<age>[m]] [-c <channel name(s)>]\n", program);
	printf("\t-c <channel names>\n");
	printf("\t\tspecifies the channel names to look at\n");
	printf("\t-f  ->  print name of oldest queued msg\n");
	printf("\t-h  ->  print list of overdue hosts\n");
	printf("\t-p  ->  print only the problem channels\n");
	printf("\t-s  ->  summary lines only\n");
	printf("\t-t<age>[m]\n");
	printf("\t\tspecifies the age for problem channels in hours or minutes\n");
	printf("\t-z  ->  non zero channels only\n");
}
