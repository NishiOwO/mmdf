#include "util.h"
#include "mmdf.h"
#include "mm_io.h"

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
/*                  SEND FROM PHONE TO LOCAL (SUBMIT)                   */

#include "ch.h"
#include "phs.h"
#include "ph.h"

extern char *getline();

char *sender = 0;			/* address of mail sender */

#define BUFL 600		/* length of buf */
char	buf[BUFL];		/* general usage */
char	netbuf[BUFL];		/* the place that has the valid characters */
int	netcount = 0;		/* number of valid characters in netbuf */
char	*netptr  = netbuf;	/* next character to come out of netbuf */
char	*arg;			/* zero if no argument - pts to comm param */
int	numrecipients = 0;	/* number of valid recipients accepted */
char	*addrfix();
int     doneflag = 0;

/* character defines */
#define CNULL	'\0'	/* null */


/****************************************************************
 *                                                              *
 *      C O M M A N D   D I S P A T C H   T A B L E             *
 *                                                              *
 ****************************************************************/

int helo(), mail(), quit(), help(), rcpt(), accept();
int data(), rset(), reject(), turn();

struct comarr		/* format of the command table */
{
	char *cmdname;		/* ascii name */
	int (*cmdfunc)();       /* command procedure to call */
} commands[] = {
	"helo", helo,		"noop", accept,
	"mail", mail,		"data", data,
	"rcpt", rcpt,		"help", help,
	"send", mail,		"quit", quit,
	"saml", mail,		"soml", reject,
	"vrfy", reject,		"expn", reject,
	"rset", rset,           "turn", turn,
	NULL, NULL
};

extern LLog *logptr;
extern struct ps_rstruct ps_rp;
extern char *supportaddr;
extern Chan *curchan;

LOCVAR long msglen;
LOCVAR int numadrs;

ph2mm_send (curchan)             /* overall mngmt for batch of msgs    */
    Chan *curchan;               /* who to say is doing relaying       */
{
    short     result;
    char *i;
    char    info[LINESIZE],
            sender[LINESIZE];
    register struct comarr *comp;

#ifdef DEBUG
    ll_log (logptr, LLOGPTR, "ph2mm_send");
#endif

    if (rp_isbad (result = ph_pkinit ()))
    {
	printx ("remote site refuses to send mail...\n");
	fflush (stdout);
	return (result);
    }
    if (rp_isbad (result = mm_sbinit ()))
	return (result);

    printx ("starting to pickup mail...\n");
    fflush (stdout);

nextcomm:
	while (i = getline())
	{
		if (i == (char *)NOTOK)		/* handle error ??? */
			return(byebye( 1 ));

		/* find and call command procedure */
		comp = commands;
		while( comp->cmdname != NULL)	/* while there are names */
		{
		    if (strcmp(buf, comp->cmdname) == 0) /* a winner */
		    {
			/* call comm proc */
			result = (*comp->cmdfunc)();
			if (doneflag)           /* set by byebye */
			    return(result);
			goto nextcomm;		/* back for more */
		    }
		    comp++;		/* bump to next candidate */
		}
		netreply("500 I never heard of that command before" );
	}
	return(byebye(0));
}

/*name:
	getline

function:
	get commands from the standard input terminated by <cr><lf>.
	afix a pointer( arg ) to any arguments passed.
	ignore carriage returns
	map UPPER case to lower case.
	manage the netptr and netcount variables

algorithm:
	while we havent received a line feed and buffer not full

		if netcount is zero or less
			get more data from net
				error: return 0
		check for delimiter character
			null terminate first string
			set arg pointer to next character
		check for carriage return
			ignore it
		if looking at command name
			convert upper case to lower case

	if command line (not mail)
		null terminate last token
	manage netptr

returns:
	0 for EOF
	-1 when an error occurs on network connection
	ptr to last character (null) in command line

globals:
	netcount=
	netptr =
	buf=
*/

char *
getline()
{
	register char *inp;	/* input pointer in netbuf */
	register char *outp;	/* output pointer in buf */
	register int c;		/* temporary char */
	extern char *progname;

	inp = netptr;
	outp = buf;
	arg = 0;

	if (rp_isbad (ph_rrec(netbuf, &netcount)))
	{
		ll_log( logptr, LLOGTMP, "net input read error");
		return((char *)NOTOK);
	}
	inp = netbuf;
	do
	{
		c = *inp++ & 0377;
		if (c == '\r' ||	/* ignore CR */
		    c >= 0200)		/* or any telnet codes that */
			continue;	/*  try to sneak through */
		if (arg == NULL)
		{
			/* if char is a delim afix token */
			if (c == ' ' || c == ',')
			{
				c = CNULL;       /* make null term'ed */
				arg = outp + 1; /* set arg ptr */
			}
			else if (c >= 'A' && c <= 'Z')
			/* do case mapping (UPPER -> lower) */
				c += 'a' - 'A';
		}
		*outp++ = c;
	} while (netcount-- > 0 && outp < &buf[BUFL] );

	*--outp = 0;                    /* null term the last token */

	/* scan off blanks in argument */
	if (arg) {
		while (*arg == ' ')
			arg++;
		if (*arg == '\0')
			arg = 0;	/* if all blanks, no argument */
	}
	ll_log( logptr, LLOGFTR, "'%s', '%s'", buf, 
	                arg == 0 ? "<noarg>" : arg );

	/* reset netptr for next trip in */
	netptr = inp;
	/* return success */
	return (outp);
}

/*
 *  Process the HELO command
 */
helo()
{
	char replybuf[128];

	if (curchan -> ch_confstr)
	    sprintf (replybuf, "250 %s", curchan -> ch_confstr);
	else
	    sprintf (replybuf, "250 %s.%s", curchan -> ch_lname,
					        curchan -> ch_ldomain);
	netreply (replybuf);
}

/*
 *	mail
 *
 *	handle the MAIL command  ("MAIL from:<user@host>")
 */
mail()
{
	int result;
	char	*cp;
	char	info[40];
	char    replybuf[256];
	struct	rp_bufstruct thereply;
	int	len;

	if (arg == 0) {
		netreply("501 No argument supplied");
		return;
	} else if( sender ) {
		netreply("503 MAIL command already accepted");
		return;
	} else if (!equal(arg, "from:", 5)) {
		netreply("501 No sender named");
		return;
	}

	if (equal(buf, "mail", 4))
	    strcpy (info, "mv");
	else if (equal(buf, "send", 4))
	    strcpy (info, "yv");
	else if (equal(buf, "saml", 4))
	    strcpy (info, "bv");
	else
	    strcpy (info, "mv");  /* shouldn't ever get here */

	/* Scan FROM: parts of arg */
	sender = strchr (arg, ':') + 1;
	sender = addrfix( sender );

	/* Supply necessary flags, "tiCHANNEL" will be supplied by winit */
	if (*sender == NULL) {
		strcat( info, "q"); 		/* No return mail */
		sender = "Orphanage";		/* Placeholder */
	} 

	if( rp_isbad( mm_winit(curchan -> ch_name, info, sender))) {
		netreply("451 Temporary problem initializing");
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else if( rp_isbad( mm_rrply( &thereply, &len ))) {
		netreply( "451 Temporary problem initializing" );
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else if( rp_gbval( thereply.rp_val ) == RP_BNO) {
		sprintf (replybuf, "501 %s", thereply.rp_line);
		netreply (replybuf);
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else if( rp_gbval( thereply.rp_val ) == RP_BTNO) {
		sprintf (replybuf, "451 %s", thereply.rp_line);
		netreply (replybuf);
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else
		netreply("250 OK");
	numrecipients = 0;
}

/*
 *  Process the RCPT command  ("RCPT TO:<forward-path>")
 */
rcpt()
{
	register char *p;
	struct rp_bufstruct thereply;
	char	replybuf[256];
	int	len;

	/* parse destination arg */
	if( sender == (char *)0 ) {
		netreply("503 You must give a MAIL command first");
		return;
	} else if (arg == (char *)0 || !equal(arg, "to:", 3)) {
		netreply("501 No recipient named.");
		return;
	}
	p = strchr( arg, ':' ) + 1;
	p = addrfix( p );

	if( rp_isbad( mm_wadr( (char *)0, p ))) {
		if( rp_isbad( mm_rrply( &thereply, &len )))
			netreply( "451 Mail system problem" );
		else {
			sprintf (replybuf, "451 %s", thereply.rp_line);
			netreply (replybuf);
		}
	} else {
		if( rp_isbad( mm_rrply( &thereply, &len )))
			netreply("451 Mail system problem");
		else if( rp_gbval( thereply.rp_val ) == RP_BNO) {
			sprintf (replybuf, "550 %s", thereply.rp_line);
			netreply (replybuf);
		}
		else if( rp_gbval( thereply.rp_val ) == RP_BTNO) {
			sprintf (replybuf, "451 %s", thereply.rp_line);
			netreply (replybuf);
		}
		else {
			netreply("250 Recipient OK.");
			numrecipients++;
		}
	}
}

/*
 *	ADDRFIX()  --  This function takes the SMTP "path" and removes
 *	the leading and trailing "<>"'s which would make the address
 *	illegal to RFC822 mailers.  Note that although the spec states
 *	that the angle brackets are required, we will accept addresses
 *	without them.   (DPK@BRL, 4 Jan 83)
 */
char *
addrfix( addrp )
char *addrp;
{
	register char	*cp;

	if( cp = strchr( addrp, '<' )) {
		addrp = ++cp;
		if( cp = strrchr( addrp, '>' ))
			*cp = 0;
	}
	compress (addrp, addrp);
#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "addrfix(): '%s'", addrp );
#endif
	return( addrp );
}

/*
 *  Process the DATA command.  Send text to MMDF.
 */
data()
{
	int	werrflg;
	struct rp_bufstruct thereply;
	int	len;
	int     result;

	werrflg = 0;
	if (numrecipients == 0) {
		netreply("503 No recipients have been specified.");
		return;
	}

	if( rp_isbad(mm_waend())) {
		netreply("451 Unknown mail system trouble.");
		return;
	}

	netreply ("354 Enter Mail, end by a line with only '.'");

#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "... body of message ..." );
#endif
	printx ("receiving...:");
	fflush (stdout);

        len = BUFL;
        while ((rp_gval (result = ph_rstm (buf, &len))) == RP_OK)
        {
		printx (".");
		fflush (stdout);

		/* If write error occurs, stop writing but keep reading. */
		if (!werrflg)
		        if (rp_isbad (result = mm_wtxt (buf, len))) {
				werrflg++;
				ll_log( logptr, LLOGTMP, "error from submit");
			}

		len = BUFL;
        }
#ifdef DEBUG
	ll_log( logptr, LLOGBTR, "Finished receiving text." );
#endif
	if (rp_isbad(result)) {
		ll_log (logptr, LLOGTMP, "error reading text");
		printx("\n\terror reading text\n");
		fflush(stdout);
		return(byebye( 1 ));
	}

	if (werrflg) {
		netreply("451-Mail trouble (write error to mailsystem)");
		netreply("451 Please try again later.");
		return(byebye( 1 ));
	}

	printx(" "); /* keep it pretty */
	fflush(stdout);

	/* slurp up the end-of-text dot (we knew we were done anyway) */
	len = BUFL;
	if (rp_isbad(ph_rrec(buf, &len))) {
		ll_log(logptr, LLOGTMP, "error reading '.'");
		printx("\n\terror reading '.'\n");
		fflush(stdout);
		return(byebye( 1 ));
	}

	if( rp_isbad(mm_wtend()) || rp_isbad( mm_rrply( &thereply, &len)))
		netreply("451 Unknown mail trouble, try later");
	else if( rp_isgood(thereply.rp_val)) {
		sprintf (buf, "250 %s", thereply.rp_line);
		netreply (buf);
	}
	else if( rp_gbval(thereply.rp_val) == RP_BNO) {
		sprintf (buf, "554 %s", thereply.rp_line);
		netreply (buf);
	}
	else {
		sprintf (buf, "451 %s", thereply.rp_line);
		netreply (buf);
	}
	sender = (char *) 0;
	numrecipients = 0;
}

/*
 *  Process the TURN command
 */
turn()
{
	accept();
	return(byebye(3));  /* pass the turn message upstairs */
}

/*
 *  Process the RSET command
 */
rset()
{
	mm_end( NOTOK );
	sender = (char *)0;
	mmdfstart();
	accept();
}

mmdfstart()
{
	if( rp_isbad( mm_init() ) || rp_isbad( mm_sbinit() )) {
		ll_log( logptr, LLOGFAT, "can't reinitialize mail system" );
		netreply("421 Server can't initialize mail system (mmdf)");
		return(byebye( 2 ));
	}
	numrecipients = 0;
}

/*
 *  handle the QUIT command
 */
quit()
{
	char	buf[128];
	time_t	timenow;

	time (&timenow);
	sprintf (buf, "221 session complete at %.19s.",
		ctime(&timenow));
	netreply(buf);
	return(byebye( 0 ));
}

byebye( retval )
int retval;
{
	doneflag++;
	switch (retval)
	{
	    case 0:
		mm_sbend();
		mm_end(OK);
		return(RP_DONE);
	    case 1:
		mm_end (NOTOK);
		return (RP_BTNO);
	    case 2:	   
		mm_end (NOTOK);
		return (RP_NO);
	    case 3:
		mm_sbend();
		mm_end(OK);
		return(RP_OK);  /* signal that we should turn line around */
	}
}

/*
 *  Reply that the current command has been logged and noted
 */
accept()
{
	netreply("250 OK");
}

/*
 *  Tell other side that we don't support a command.
 */
reject()
{
	netreply("502 Command not implemented.");
}

/*
 *  Process the HELP command by giving a list of valid commands
 */
help()
{
/*	  register i; */
/*	  register struct comarr *p; */
	char	replybuf[256];

/*	  netreply("214-The following commands are accepted:\r\n214-" );
 *	  for(p = commands, i = 1; p->cmdname; p++, i++) {
 *		  sprintf (replybuf, "%s%s", p->cmdname, ((i%10)?" ":"\r\n214-") );
 *		  netreply (replybuf);
 *	  }
 */
	sprintf (replybuf, "214 Send complaints/bugs to:  %s", supportaddr);
	netreply (replybuf);
}

/*
 *  Send appropriate ascii responses over the network connection.
 */

netreply(string)
char *string;
{
	int len;

	if (rp_isbad( ph_wrec(string, strlen(string)))) {
		ll_log( logptr, LLOGFST,
			"(netreply) error in writing [%s] ...",
			string);
		return(byebye( 1 ));
	}
#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "%s", string);
#endif DEBUG
}
