static char Id[] = "$Id: smtpsrvr.c,v 1.37 2001/05/02 21:27:52 krueger Exp $";
/*
 *                      S M T P S R V R . C
 *
 *              SMTP Mail Server for MMDF under 4.2bsd
 *
 * Eric Shienbrood (BBN) 3 Apr 81 - SMTP server, based on BBN FTP server
 * Modified to talk to MMDF Jun 82 by Donn Neuhengen.
 * Doug Kingston, BRL: Took a machete to large chunks of unnecessary code.
 *      Reimplemented the interface to MMDF and used 4.2BSD style networking.
 *      Usage:  smtpsrvr <them> <us> <channel>
 */

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include "ap.h"
#include "phs.h"
#include "smtp.h"
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <pwd.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_LIBWRAP
#  include <tcpd.h>
#  include <syslog.h>
#else /* HAVE_LIBWRAP */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif /* HAVE_LIBWRAP */
#include "mm_io.h"

#include "ns.h"

#define BYTESIZE        8

extern void ap_7to8 ();
/* mmdf configuration variables */
extern LLog *logptr;
extern char *supportaddr;
extern int errno;               /* Has Unix error codes */
extern int numfds;
extern char *quedfldir;

extern char *getline();
extern struct passwd *getpwmid();
extern char *multcat();
extern Chan *ch_h2chan();

extern int mgt_addipaddr,
           mgt_addipname;
#ifdef HAVE_ESMTP
extern long message_size_limit;
#   ifdef HAVE_ESMTP_8BITMIME
extern int  accept_8bitmime;
#   endif /* HAVE_ESMTP_8BITMIME */
#   ifdef HAVE_ESMTP_DSN
extern int  dsn;
#   endif /* HAVE_ESMTP_DSN */
#endif /* HAVE_ESMTP */
#if defined(HAVE_RBL)
char reject_on_rbl = 0;
#endif /* HAVE_RBL */
extern char *valid_channels; /* list of known channel for smtpsrvr */

smtp_protocol smtp_proto = PRK_UNKNOWN;
int input_source = 0;
#define IN_SRC_NET   0
#define IN_SRC_LOCAL 1

char *progname, *us, *them, *channel;   /* Arguments to program */
char *sender = 0;                       /* address of mail sender */
char *helostr = 0;
char from_host[256];
Chan *chanptr;                          /* pointer to incoming channel */

#define BUFL 600                /* length of buf */
char    buf[BUFL];              /* general usage */
char    netbuf[BUFL];           /* the place that has the valid characters */
char    lastbyte;
char	saveaddr[BUFL];         /* save previous address in expn_save */
int     savecode;               /* save previous result code in expn_save */
int     expn_count;             /* number of expn expansions */
int     netcount = 0;           /* number of valid characters in netbuf */
char    *netptr  = netbuf;      /* next character to come out of netbuf */
char    *arg;                   /* zero if no argument - pts to comm param */
int     dont_mung;              /* used by getline() to limit munging */
int     numrecipients = 0;      /* number of valid recipients accepted */
int     stricked;               /* force rejection of non validated hosts */
#ifdef NODOMLIT
int	themknown=TRUE;		/* do we have symbolic name for them? */
#endif /* NODOMLIT */
int size_checked = FALSE;       /* checked for enough spoolspace let? */
char    *addrfix();
int     vrfy_child;             /* pid of child that handles vrfy requests */
int     vrfy_p2c[2];            /* fd's for vrfy's parent-to-child pipe */
int     vrfy_c2p[2];            /* fd's for vrfy's child-to-parent pipe */
FILE    *vrfy_out;              /* fd for vrfy's parent to write to child */
FILE    *vrfy_in;               /* fd for vrfy's parent to read from child */

/* character defines */
#define CR      '\r'    /* carriage return */
#define LF      '\n'    /* line feed */
#define CNULL   '\0'    /* null */

#ifdef HAVE_LIBWRAP
int     allow_severity = LOG_INFO;      /* run-time adjustable */
int     deny_severity = LOG_WARNING;    /* ditto */
#ifndef STDIN_FILENO
#define STDIN_FILENO    0
#endif
#endif /* HAVE_LIBWRAP */

#ifdef HAVE_ESMTP
void tell_esmtp_options();
#endif /* HAVE_ESMTP */

/****************************************************************
 *                                                              *
 *      C O M M A N D   D I S P A T C H   T A B L E             *
 *                                                              *
 ****************************************************************/

int helo(), mail(), quit(), help(), rcpt(), confirm();
int data(), rset(), reject(), expn(), vrfy();

void rbl_reject();


#define CMDNOOP 0
#define CMDHELO 1
#define CMDMAIL 2
#define CMDDATA 3
#define CMDRCPT 4
#define CMDHELP 5
#define CMDQUIT 6
#define CMDRSET 7
#define CMDEXPN 8
#define CMDVRFY 9
#ifdef HAVE_ESMTP
#  define CMDSEND 10
#  define CMDSOML 11
#  define CMDSAML 12
#  define CMDEHLO 13
#  define CMDETRN 14
#  define CMDVERB 15
#  define CMDONEX 16
#  define CMDXUSR 17
#endif /* HAVE_ESMTP */

struct comarr           /* format of the command table */
{
	char *cmdname;          /* ascii name */
	int (*cmdfunc)();       /* command procedure to call */
    int cmdnr;
    int cmdmode;
} commands[] = {
    {"noop", confirm, CMDNOOP, 0}, /* 0 */
	{"helo", helo   , CMDHELO, 0}, /* 1 */
    {"mail", mail   , CMDMAIL, 0}, /* 2 */
    {"data", data   , CMDDATA, 0}, /* 3 */
	{"rcpt", rcpt   , CMDRCPT, 0}, /* 4 */
    {"help", help   , CMDHELP, 0}, /* 5 */
	{"quit", quit   , CMDQUIT, 0}, /* 6 */
    {"rset", rset   , CMDRSET, 0}, /* 7 */
    {"expn", expn   , CMDEXPN, 0}, /* 8 */
    {"vrfy", vrfy   , CMDVRFY, 0}, /* 9 */
#ifdef HAVE_ESMTP
    {"send", mail   , CMDSEND, 1}, /* 10 */
    {"soml", mail   , CMDSOML, 1}, /* 11 */
    {"saml", mail   , CMDSAML, 1}, /* 12 */
	{"ehlo", helo   , CMDEHLO, 1}, /* 13 */
#  ifdef HAVE_ESMTP_ETRN
	{"etrn", confirm, CMDETRN, 1}, /* 14 */
#  endif /* HAVE_ESMTP_ETRN */
#  ifdef HAVE_ESMTP_VERB
	{"verb", confirm, CMDVERB, 1}, /* 15 */
#  endif /* HAVE_ESMTP_VERB */
#  ifdef HAVE_ESMTP_ONEX
	{"onex", confirm, CMDONEX, 1}, /* 16 */
#  endif /* HAVE_ESMTP_ONEX */
#  ifdef HAVE_ESMTP_XUSR
	{"xusr", confirm, CMDXUSR, 1}, /* 17 */
#  endif /* HAVE_ESMTP_XUSR */
#endif /* HAVE_ESMTP */
	{NULL, NULL     }
    
};

/*
 *              M A I N
 *
 *      Takes commands from the assumed network connection (file desc 0)
 *      under the assumption that they follow the ARPA network mail
 *      transfer protocol RFC 788 and appropriate modifications.
 *      Command responses are returned via file desc 1.
 *
 *      There is a small daemon waiting for connections to be
 *      satisfied on socket 25 from any host.  As connections
 *      are completed by the ncpdaemon, the returned file descriptor
 *      is setup as the standard input (file desc 0) and standard
 *      output (file desc 1) and this program is executed to
 *      deal with each specific connection.  This allows multiple
 *      server connections, and minimizes the system overhead
 *      when connections are not in progress.  File descriptors
 *      zero and one are used to ease production debugging and
 *      to allow the forking of other relavent Unix programs
 *      with comparative ease.
 *
 *              while commands can be gotten
 *                      find command procedure
 *                              call command procedure
 *                              get next command
 *                      command not found
 *
 */
main (argc, argv)
int argc;
char **argv;
{
	register struct comarr *comp;
	char    replybuf[128];
	char *i;
	long atime;
	Chan *curchan;
	char    tmp_buf[LINESIZE];
	char    tmpstr[LINESIZE];
	char    *Ags[NUMCHANS];
	int     n, Agc;
#ifdef HAVE_LIBWRAP
        struct request_info  request;   
#else  /* HAVE_LIBWRAP */
	struct sockaddr_in rmtaddr;
	int    len_rmtaddr = sizeof rmtaddr;
#endif /* HAVE_LIBWRAP */

	progname = argv[0];
#ifdef HAVE_LIBWRAP
        request_init(&request, RQ_DAEMON, argv[0], RQ_FILE, STDIN_FILENO, 0);
        fromhost(&request);
#endif /* HAVE_LIBWRAP */
	mmdf_init( progname, 0);

    if( (valid_channels == NULL && argc != 4) ||
        (valid_channels != NULL && argc != 3 && argc != 4)) {
		ll_log( logptr, LLOGFAT, "wrong number of args!" );
		exit(NOTOK);
	}

	/* force rejection of unknown hosts */
	if (*progname == 'r')
		stricked++;

	/*
	 * first look up address of sender. Used to be in server but
	 * now must be in this routine so that we can send a reject
	 * reply.
	 */
	them = argv[1];
	us = argv[2];
				/* A numeric address - don't like it */
	if(*them >= '0' && *them <= '9')
	    if ( stricked){
		ll_log (logptr, LLOGTMP, "smtpsrvr can't lookup '%s'", them);
		snprintf(replybuf, sizeof(replybuf),
		       "421 %s: Cannot resolve your address. '%s'\r\n",us,them);
		netreply (replybuf);
		exit (-1);
	    }
	    else {   /* make into [x.x.x.x] format */
		them = strdup(them);
	        strncpy(tmpstr, them, sizeof(tmpstr));  
		sprintf(them, "%s", tmpstr);
		/* sprintf(them, "[%s]", tmpstr); */
#ifdef NODOMLIT
		themknown = FALSE;
#endif /* NODOMLIT */
	    }
#ifdef HAVE_LIBWRAP
	eval_user(&request);
	if (STR_EQ(eval_hostname(request.client), paranoid))
	  smtp_refuse(&request, "Paranoid");
	if (!hosts_access(&request)) smtp_refuse(&request, "denied");
	if(mgt_addipaddr && mgt_addipname)
	  snprintf(from_host, sizeof(from_host), "%s [%s]", eval_client(&request),
		  eval_hostaddr(request.client));
	else if(mgt_addipname)
	  snprintf(from_host, sizeof(from_host), "%s ", eval_client(&request));
	else if(mgt_addipaddr)
	  snprintf(from_host, sizeof(from_host), "[%s]", eval_hostaddr(request.client));
    if(strncmp(from_host, "unknown", 7)==0) {
      input_source = IN_SRC_LOCAL;
	  snprintf(from_host, sizeof(from_host), "%s", strdup(them));
    }
	ll_log( logptr, LLOGGEN, "connection from: %s",eval_client(&request));
#else /* HAVE_LIBWRAP */
	/* sprintf(from_host, sizeof(from_host), "%s [IP]", strdup(them));*/
	if (getpeername (0, (struct sockaddr *)&rmtaddr, &len_rmtaddr)>=0) {
	  struct	hostent	*hp;
	  hp = gethostbyaddr ( (char *)&rmtaddr.sin_addr,
			       sizeof(rmtaddr.sin_addr), AF_INET );
	  if ((hp == NULL) || !isstr(hp->h_name)) {
	    snprintf(from_host, sizeof(from_host), "[%s]",
		    (char *)inet_ntoa(rmtaddr.sin_addr));
	  } else
	    snprintf(from_host, sizeof(from_host), "%s [%s]", hp->h_name,
		    (char *)inet_ntoa(rmtaddr.sin_addr));
	} else {
      input_source = IN_SRC_LOCAL;
	  snprintf(from_host, sizeof(from_host), "%s", strdup(them));
    }
#endif /* HAVE_LIBWRAP */
    
	/*
	 * found out who you are I might even believe you.
	 */

	/*
	 * the channel arg is now a comma seperated list of channels
	 * useful for multiple sources ( As on UCL's ether )
	 */
    if(argc==4) strncpy (tmp_buf, argv[3], sizeof(tmp_buf));
    else strncpy (tmp_buf, valid_channels, sizeof(tmp_buf));
    
	Agc = str2arg (tmp_buf, NUMCHANS, Ags, (char *)0);
	channel = Ags[Agc-1];
	for(chanptr = (Chan *)0, n = 0 ; n < Agc ; n++){
		if ( (curchan = ch_nm2struct(Ags[n])) == (Chan *) NOTOK) {
			ll_log (logptr, LLOGTMP, "smtpsrvr (%s) bad channel",
				Ags[n]);
			continue;
		}
		/*
		 * Is this a valid host for this channel ?
		 */
#ifdef HAVE_WILDCARD
		switch(tb_wk2val (curchan -> ch_table, TRUE, them, tmpstr))
#else /* HAVE_WILDCARD */
		switch(tb_k2val (curchan -> ch_table, TRUE, them, tmpstr))
#endif /* HAVE_WILDCARD */
        {
            default:        /* Either NOTOK or MAYBE */
              if ((n != (Agc-1)) || stricked)
				continue;
			/* fall through so we get some channel name to use */
            case OK:
              chanptr = curchan;
              channel = curchan -> ch_name;
              break;
		}
		break;
	}
	time(&atime);

	if (chanptr == (Chan *) 0){
		ll_log (logptr, LLOGTMP, "smtpsrvr (%s) no channel for host",
								    them);
		snprintf (replybuf, sizeof(replybuf),
			"421 %s: Your name, '%s', is unknown to us.\r\n",
			us, them);
		netreply (replybuf);
		exit (-1);
	}
#if defined(HAVE_RBL)
    /* hier muesste reject hin bei RBL-support */
    if( (curchan->ch_table->tb_flags & TB_REJECT) == TB_REJECT)
      reject_on_rbl=1;
#endif /* HAVE_RBL */

	ch_llinit (chanptr);
	ll_log( logptr, LLOGGEN, "OPEN: %s %.19s (%s)",
			them, ctime(&atime), channel);
	phs_note(chanptr, PHS_RESTRT);

	mmdfstart();

	/* say we're listening */
#ifdef HAVE_ESMTP
    if((chanptr->ch_access&CH_ESMTP)==CH_ESMTP)
      snprintf (replybuf, sizeof(replybuf), "220 %s Server ESMTP (Complaints/bugs to:  %s)\r\n",
               us, supportaddr);
    else
#endif /* HAVE_ESMTP */
      snprintf (replybuf, sizeof(replybuf), "220 %s Server SMTP (Complaints/bugs to:  %s)\r\n",
               us, supportaddr);
	netreply (replybuf);

nextcomm:
	while (i = getline())
	{
		if (i == (char *)NOTOK)         /* handle error ??? */
			byebye( 1 );
        
		/* find and call command procedure */
		comp = commands;
		while( comp->cmdname != NULL)   /* while there are names */
		{
		    if (strcmp(buf, comp->cmdname) == 0) /* a winner */
		    {
#ifdef HAVE_ESMTP
              if( (!comp->cmdmode) ||
                  (comp->cmdmode &&
                   ((chanptr->ch_access&CH_ESMTP)==CH_ESMTP))) {
#endif
#if defined(HAVE_RBL)
                if(reject_on_rbl && comp->cmdnr != CMDHELO &&
                   comp->cmdnr != CMDEHLO && comp->cmdnr != CMDQUIT)
                  rbl_reject(curchan->ch_table, them);
                else
#endif
                  (*comp->cmdfunc)(comp->cmdnr);     /* call comm proc */
                goto nextcomm;          /* back for more */
#ifdef HAVE_ESMTP
              }
#endif
            }
        comp++;             /* bump to next candidate */
		}
		netreply("500 Unknown or unimplemented command\r\n" );
		ll_log(logptr, LLOGBTR, "unknown command '%s'", buf);
	}
	byebye(0);
}

#ifdef HAVE_LIBWRAP
smtp_refuse(request, what)
struct request_info *request;
char *what;
{ 
  char replybuf[256];
  
  /*  snprintf (replybuf, sizeof(replybuf), "220 %s Server SMTP (Complaints/bugs to:  %s)\r\n",
	   us, supportaddr);
  netreply (replybuf);*/
  ll_log( logptr, LLOGGEN, "Connection Refused (%s) from: %s",
	  what, eval_client(request));
  snprintf(replybuf, sizeof(replybuf), "451 Connection Refused (%s) from: %s\r\n", what, 
	  eval_client(request));
  netreply(replybuf);
  byebye(1);
}
#endif /* HAVE_LIBWRAP */

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
	dont_mung
	netcount=
	netptr =
	buf=
*/

char *
getline()
{
	register char *inp;     /* input pointer in netbuf */
	register char *outp;    /* output pointer in buf */
	register int c;         /* temporary char */
	extern char *progname;

	inp = netptr;
	outp = buf;
	arg = 0;

	do
	{
		if( --netcount <= 0 )
		{
			if (setjmp(timerest)) {
				ll_log( logptr, LLOGTMP,
					"%s net input read error (%d)",
					them, errno);
				return((char *)NOTOK);
			}
			s_alarm (NTIMEOUT);
			netcount = read (0, netbuf, 512);
			s_alarm( 0 );
			if (netcount == 0)      /* EOF */
				return( 0 );
			if (netcount < 0) {     /* error */
				ll_log( logptr, LLOGTMP,
					"%s net input read error (%d)",
					them, errno);
				return((char *)NOTOK);
			}
			inp = netbuf;
		}
		c = *inp++ & 0377;

#ifndef HAVE_8BIT
		if (c == '\r' ||        /* ignore CR */
		    c >= 0200)          /* or any telnet codes that */
#else /* HAVE_8BIT */
		if (c == '\r')        /* ignore CR */
#endif /* HAVE_8BIT */
			continue;       /*  try to sneak through */
		if (dont_mung == 0 && arg == NULL)
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
		lastbyte = *outp++ = c;
	} while( c != '\n' && outp < &buf[BUFL] );

	if( dont_mung == 0 )
		*--outp = 0;            /* null term the last token */

	/* scan off blanks in argument */
	if (arg) {
		while (*arg == ' ')
			arg++;
		if (*arg == '\0')
			arg = 0;        /* if all blanks, no argument */
	}
	if (dont_mung == 0)
		ll_log( logptr, LLOGFTR, "'%s', '%s'", buf,
			arg == 0 ? "<noarg>" : arg );

	/* reset netptr for next trip in */
	netptr = inp;
	/* return success */
	return (outp);
}

char *get_value(field)
char *field;
{
  int  argc;
  char *argv[5];
  char nn[1024];

  argc = sstr2arg(field, 5, argv, "=");
  if(argc<2) return (NULL);
  return(argv[1]);
}

/*
 *  Process the HELO command
 */
#ifdef __STDC__
helo(int cmdnr)
#else
helo(cmdnr)
int cmdnr;
#endif
{
	char replybuf[128];

/*
 * Use this if you wish to be forgiving of hosts who don't announce
 * their full domain name:
 *
 *      (void) snprintf(replybuf, sizeof(replybuf), "%s.%s.%s", them, locname, locdomain);
 *      if(arg == 0 || !lexequ(arg, them) && !lexequ(arg, replybuf))
 *              snprintf(replybuf, sizeof(replybuf), "250 %s - you are a charlatan\r\n", us);
 *      else  {                
 *		if (lexequ(arg, replybuf))
 *                      them = strdup(replybuf);
 *              snprintf (replybuf, sizeof(replybuf), "250 %s\r\n", us);
 *      }                      
 *      netreply (replybuf);   
 *
 * -- DSH (suggested by: Hans van Staveren <sater@cs.vu.nl>)
 */

    if(smtp_proto != PRK_UNKNOWN) {
#ifdef HAVE_ESMTP
      snprintf(replybuf, sizeof(replybuf), "503 %s Duplicate HELO/EHLO\r\n", us);
#else
      snprintf(replybuf, sizeof(replybuf), "503 %s Duplicate HELO\r\n", us);
#endif
      netreply (replybuf);
    } else {
      if(arg == 0) {
        snprintf(replybuf, sizeof(replybuf), "501 %s requires domain address\r\n",
                commands[cmdnr].cmdname);
        netreply(replybuf);
      } else {
        if(arg!=0) helostr = strdup(arg);
        /*ll_log( logptr, LLOGFTR, "helostr \"%s\" #1\n", helostr );*//*is ok -u*/
#ifdef HAVE_ESMTP
        if(cmdnr==CMDEHLO) {
          smtp_proto = PRK_ESMTP;
          if(!lexequ(arg, them))
            snprintf(replybuf, sizeof(replybuf), "250-%s - you are a charlatan\r\n", us);
          else 
            snprintf (replybuf, sizeof(replybuf), "250-%s\r\n", us);
          netreply (replybuf);
          tell_esmtp_options();
        } else {
#endif /* HAVE_ESMTP */
          smtp_proto = PRK_SMTP;
          if(!lexequ(arg, them))
            snprintf(replybuf, sizeof(replybuf), "250 %s - you are a charlatan\r\n", us);
          else 
            snprintf (replybuf, sizeof(replybuf), "250 %s\r\n", us);
          netreply (replybuf);
#ifdef HAVE_ESMTP
        }
#endif
      }
    }
    
}

/*
 *      mail
 *
 *      handle the MAIL command  ("MAIL from:<user@host>")
 */
#define MCMDSIZE  0
#define MCMDBODY  1
#define MCMDRET   2
#define MCMDENVID 3

struct comarr2           /* format of the command table */
{
    char *cmdname;          /* ascii name */
    int (*cmdfunc)();       /* command procedure to call */
    int cmdnr;
    void *valueptr;
};

#ifdef HAVE_ESMTP

int mfsize(argc, argv, sizeptr)
int argc;
char **argv;
long *sizeptr;
{
  static char    replybuf[256];
  
  if(argc<2) {
    netreply("555 missing SIZE parameter\r\n");
    return NOTOK;
  }
  sscanf(argv[1], "%d", sizeptr);
  
  if(*sizeptr<=0) {
    snprintf(replybuf, sizeof(replybuf),
             "555 malformed SIZE clause %s\r\n", argv[1]);
    netreply(replybuf);
    return NOTOK;
  }
  ll_log( logptr, LLOGFTR, "mail from: size=%d", sizeptr);
  if (message_size_limit > 0 && *sizeptr > message_size_limit) {
    snprintf(replybuf, sizeof(replybuf),
             "552 Message size exceeds maximum permitted\r\n");
    netreply(replybuf);
    return NOTOK;
  }
  return OK;
}

#ifdef HAVE_ESMTP_8BITMIME
int mfbody(argc, argv, valueptr)
int argc;
char **argv;
void *valueptr;
{
  static char    replybuf[256];

  if (!accept_8bitmime) return OK;
  
  if(argc<2) {
    netreply("555 missing BODY parameter\r\n");
    return NOTOK;
  }
  if( lexequ(argv[1], "8BITMIME")) {
  } else {
    if( lexequ(argv[1], "7BIT")) {
    } else {
      snprintf(replybuf, sizeof(replybuf),
               "555 unknown BODY type %s\r\n", argv[1]);
      netreply(replybuf);
      sender = NULL;
      return NOTOK;
    }
  }
  ll_log( logptr, LLOGFTR, "mail from: body=%s", argv[1]);

  netreply("BODY\n");
  return OK;
}
#endif /* HAVE_ESMTP_8BITMIME */

#ifdef HAVE_ESMTP_DSN
int mfret(argc, argv, valueptr)
int argc;
char **argv;
void *valueptr;
{
  static char    replybuf[256];

  if (!dsn) return OK;
  
  if(argc<2) {
    netreply("555 missing RET parameter\r\n");
    return NOTOK;
  }
  if( lexequ(argv[1], "HDRS")) {
    /*dsn_ret_hdrs = 1;*/
  } else {
    if( lexequ(argv[1], "FULL")) {
      /*dsn_ret_full = 1;*/
    } else {
      snprintf(replybuf, sizeof(replybuf),
               "555 unknown RET type %s\r\n", argv[1]);
      netreply(replybuf);
      return NOTOK;
    }
  }
  ll_log( logptr, LLOGFTR, "mail from: ret=%s", argv[1]);

  netreply("DSN RET\n");
  return OK;
}

int mfenvid(argc, argv, dsn_envid)
int argc;
char **argv;
char *dsn_envid;
{
  static char    replybuf[256];

  if (!dsn) return OK;
  if(argc<2) {
    netreply("555 missing ENVID parameter\r\n");
    return NOTOK;
  }
  strncpy(dsn_envid, argv[1], 32);
  ll_log( logptr, LLOGFTR, "mail from: envid=%s", dsn_envid);
  return OK;
}
#endif /* HAVE_ESMTP_DSN */
#endif /* HAVE_ESMTP */


#ifdef HAVE_NOSRCROUTE
extern int ap_outtype;
#endif
#ifdef __STDC__
mail(int cmdnr)
#else
mail(cmdnr)
int cmdnr;
#endif
{
    static char    replybuf[256];
    static char    info[1024];
    static char    *value;
    char           *lastdmn;
    struct rp_bufstruct thereply;
#ifdef HAVE_NOSRCROUTE
    int            ap_outtype_save;
#endif
    int	len, infolen=0, infoboo=0;
    AP_ptr  domain, route, mbox, themap, ap_sender;
    char           *argv[25];
    char           *subargv[5];
    int            Agc, SubAgc, i;
#ifdef HAVE_ESMTP
    register struct comarr2 *comp;
    long           size=-1;
#  ifdef HAVE_ESMTP_DSN
    char           dsn_envid[32];
#  endif /* HAVE_ESMTP_DSN */
    struct comarr2           /* format of the command table */
      mailfromcommands[] = {
        {"size",  mfsize,  MCMDSIZE,  NULL  },      /* 0 */
#ifdef HAVE_ESMTP_8BITMIME
        {"body",  mfbody,  MCMDBODY,  NULL},                 /* 1 */
#endif /* HAVE_ESMTP_8BITMIME */
#ifdef HAVE_ESMTP_DSN
        {"ret",   mfret,   MCMDRET,   NULL},                  /* 2 */
        {"envid", mfenvid, MCMDENVID, (void *)(&dsn_envid) }, /* 3 */
#endif /* HAVE_ESMTP_DSN */
        {NULL, NULL, -1, NULL     }
      };
        mailfromcommands[0].valueptr = (void *)&size;
#endif /* HAVE_ESMTP */
 
	if (arg == 0 || *arg == 0) {
		netreply("501 No argument supplied\r\n");
		return;
	} else if( sender ) {
		netreply("503 MAIL command already accepted\r\n");
		return;
	} else if ( !equal(arg, "from:", 5) && !equal(arg, "from ", 5)) {
		netreply("501 No sender named\r\n");
		return;
	}

    ll_log( logptr, LLOGFTR, "mail from: '%s'", arg );

    if( (value = strchr(arg, ':'))==NULL) {
      snprintf(replybuf, sizeof(replybuf),
               "555 malformed MAIL FROM clause %s\r\n", arg);
      netreply(replybuf);
      return;
    }

    /* Scan FROM: parts of arg */
    Agc = sstr2arg(value+1, 25, argv, " ");

    if(Agc==NOTOK) {
      netreply("451 internal error\r\n");
      return;
    }

#ifdef HAVE_ESMTP
    i=0;
    for(i=0; i<Agc; i++) {
      SubAgc = sstr2arg(argv[i], 5, subargv, "=");
      if(SubAgc==NOTOK) {
        netreply("451 internal error\r\n");
        return;
      }
      comp = mailfromcommands;
      while( comp->cmdname != NULL)   /* while there are names */
      {
        if (equal(subargv[0], comp->cmdname, strlen(comp->cmdname))) /* a winner */
        {
          if( (*comp->cmdfunc)(SubAgc, subargv, comp->valueptr) == NOTOK) {
             /* call comm proc */
            sender = NULL;
            return;
          }
          break;
        }
        comp++;             /* bump to next candidate */
      }
      if(comp->cmdname == NULL) { /* command not found */
        if(i==0) { 
          sender = argv[0];
          sender = addrfix( sender );
        } else {
          if(Agc>0) {
            if(sender==NULL) netreply("501 No sender named\r\n");
            else {
              if(strlen(argv[i])>0) {
                sender = NULL;
                snprintf(replybuf, sizeof(replybuf),
                         "555 Unknown MAIL TO: option %s\r\n", argv[i]);
                netreply(replybuf);
                /* "501 bad parameter\r\n"*/
              }
            }
            return;
          }
        }
      } /* end if command not found */
      if( (i==0) && (sender==NULL) ) {
        netreply("501 No sender named\r\n");
        return;
      }
    }
#else /* HAVE_ESMTP */
    sender = argv[0];
    sender = addrfix( sender );
#endif /* HAVE_ESMTP */
    
    if (!size_checked &&
        rp_gval(check_disc_space(5000, quedfldir))!=RP_BOK) {
      snprintf(replybuf, sizeof(replybuf),
               "452 space shortage, please try later\r\n");
      netreply(replybuf);
      sender = NULL;
      return;
    }

#ifdef HAVE_ESMTP
    if (1 /*smtp_check_spool_space*/) {
      if (rp_gval(check_disc_space(size + 5000, quedfldir)) != RP_BOK) {
        snprintf(replybuf, sizeof(replybuf),
                 "452 space shortage, please try later\r\n");
        netreply(replybuf);
        sender = NULL;
        return;
      }
      size_checked = TRUE;    /* No need to check again below */
    }
#endif /* HAVE_ESMTP */
	/*
	 * If the From part is not the same as where it came from
	 * then add on the extra part of the route.
	 */

#ifdef NODOMLIT
	if(themknown && ((ap_sender = ap_s2tree(sender)) != (AP_ptr)NOTOK))
#else
	if ((ap_sender = ap_s2tree(sender)) != (AP_ptr)NOTOK)
#endif /* NODOMLIT */
    {
      
		/*
		 * this must be a bit of a sledge hammer approach ??
		 */
		ap_t2parts(ap_sender, (AP_ptr *)0, (AP_ptr *)0,
						&mbox, &domain, &route);
		themap = ap_new(APV_DOMN, them);
		if(ap_dmnormalize(themap, (Chan *)0) == MAYBE)
			goto tout;

		if(route != (AP_ptr)0){
			/*
			 * only normalize the bits that we need
			 */
			if(ap_dmnormalize(route, (Chan *)0) == MAYBE)
				goto tout;
			lastdmn = route->ap_obvalue;
			}
		else if(domain != (AP_ptr)0) {
			if(ap_dmnormalize(domain, (Chan *)0) == MAYBE)
				goto tout;
			lastdmn = domain->ap_obvalue;
		}
		else /* is this a protocol violation? - add themap as domain? */
			lastdmn = (char *)0;

		/*
		 * Check the from part here. Make exceptions for local machines
		 */
		if (	lexequ(themap->ap_obvalue, lastdmn)
		     || islochost(themap->ap_obvalue, lastdmn))
				ap_free(themap);
		else {
			if (route != (AP_ptr)0)
				themap->ap_chain = route;
            route = themap;
		}
#if 0
        if(domain==(AP_ptr)0)
          sender = ap_p2s((AP_ptr)0, (AP_ptr)0, mbox, route, (AP_ptr)0);
        if ((ap_sender = ap_s2tree(sender)) != (AP_ptr)NOTOK){
          netreply("test1\r\n");
          ap_t2parts(ap_sender, (AP_ptr *)0, (AP_ptr *)0,
                     &mbox, &domain, &route);
          netreply("test1\r\n");
        }
#endif
#ifdef HAVE_NOSRCROUTE1
                ap_outtype_save = ap_outtype;
                ap_outtype |= AP_NOSRCRT;
#endif
		sender = ap_p2s((AP_ptr)0, (AP_ptr)0, mbox, domain, route);
#ifdef HAVE_NOSRCROUTE1
                ap_outtype = ap_outtype_save;
#endif
		if(sender == (char *)MAYBE){    /* help !! */
	tout:;
			sender = (char *) 0;
			netreply("451 Nameserver timeout during parsing\r\n");
			return;
		}
		strcpy(arg, sender);
		free(sender);
		sender = arg;
	}

#define STRCAP(STRCAP_STRINGX)        STRCAP_STRINGX[sizeof(STRCAP_STRINGX)-1]='\0'
#define INFOBOO       if(infoboo<0) STRCAP(info); else infolen+=infoboo

	/* Supply necessary flags, "tiCHANNEL" will be supplied by winit */
	if (*sender == '\0') {
      /* No return mail */
      /*  until mailing list fix is done -- er.. *WHAT* mailing list fix -- [DSH]
          snprintf( info, sizeof(info), "mvqdh%s*k%d*", them, NS_NETTIME );
       */
      infoboo=snprintf( info, sizeof(info), "mvqh%s*k%d*", them, NS_NETTIME );
      INFOBOO;
      sender = "Orphanage";           /* Placeholder */
    } else {
      /*  until mailing list fix is done 
          snprintf( info, sizeof(info), "mvdh%s*k%d*", them, NS_NETTIME ); */
      infoboo=snprintf( info, sizeof(info), "mvh%s*k%d*", them, NS_NETTIME );
      INFOBOO;
    }
    /*ll_log( logptr, LLOGFTR, "helostr \"%s\" #2\n", helostr );*//*is ok -u*/

    /* info must have correct string in it already at this point -u@q.net */
    if(helostr) {
      infoboo=snprintf( info+infolen, sizeof(info)-infolen, "H%s*", helostr );
      INFOBOO;
    }
    if(from_host[0]) {
      infoboo=snprintf( info+infolen, sizeof(info)-infolen, "F%s*", from_host );
      INFOBOO;
    }
#ifdef HAVE_ESMTP
    infoboo=snprintf( info+infolen, sizeof(info)-infolen, "p%d*", smtp_proto);
    INFOBOO;
#endif
  
    /* bug found: snprintf glibc2.1.1 clobbers destination before copy source.
       was recent addition (no wonder!) -u@q.net */
    /*ll_log( logptr, LLOGFTR, "calling mm_winit(%s, %s, %s)\n", channel, info, sender);*/
    
	if( rp_isbad( mm_winit(channel, info, sender))) {
		netreply("451 Temporary problem initializing\r\n");
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else if( rp_isbad( mm_rrply( &thereply, &len ))) {
		netreply( "451 Temporary problem initializing\r\n" );
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else if( rp_gbval( thereply.rp_val ) == RP_BNO) {
      switch (rp_gval(thereply.rp_val))
      {
          case RP_BADR:
            snprintf (replybuf, sizeof(replybuf), "550 %s\r\n", thereply.rp_line);
            break;
          case RP_BCHN:
            snprintf (replybuf, sizeof(replybuf), "550 %s\r\n", thereply.rp_line);
            break;
          default:
            snprintf (replybuf, sizeof(replybuf), "501 %s (%o)\r\n", thereply.rp_line, thereply.rp_val&0xff);
      }
      netreply (replybuf);
      sender = (char *) 0;
      mm_end( NOTOK );
      mmdfstart();
	} else if( rp_gbval( thereply.rp_val ) == RP_BTNO) {
		snprintf (replybuf, sizeof(replybuf), "451 %s\r\n", thereply.rp_line);
		netreply (replybuf);
		sender = (char *) 0;
		mm_end( NOTOK );
		mmdfstart();
	} else {
      snprintf(replybuf, sizeof(replybuf), "250 %s... Sender ok\r\n", sender);
      netreply (replybuf);
    }
	numrecipients = 0;
}

islochost(them, from)
char *them, *from;
{
	int lo, lt;
	extern char *locfullmachine, *locfullname;

	if (locfullmachine == (char *)0)
		return(0);
	lo = strlen(locfullname);
	lt = strlen(them);
	return(
		/* return true if they claim to be us */
		lexequ(from, locfullname)
		/* and are one of our local machines */
	     && lt > lo
	     && them[lt - lo - 1] == '.'
	     && lexequ(&them[lt - lo], locfullname)
	);
}

/*
 *  Process the RCPT command  ("RCPT TO:<forward-path>")
 */
#ifdef __STDC__
rcpt(int cmdnr)
#else
rcpt(cmdnr)
int cmdnr;
#endif
  {
	register char *p;
	struct rp_bufstruct thereply;
	char    replybuf[256];
	int     len;

	/* parse destination arg */
	if( sender == (char *)0 ) {
		netreply("503 You must give a MAIL command first\r\n");
		return;
	} else if (arg == (char *)0 || !equal(arg, "to:", 3)) {
		netreply("501 No recipient named.\r\n");
		return;
	}
	compress(arg, arg);
	if ( (strlen(arg)==3 && equal(arg, "to:", 3)) ||
	     (strlen(arg)==4 && equal(arg, "to: ", 4)) ) {
	  netreply("501 No recipient named.\r\n");
	  return;
	}
	p = strchr( arg, ':' ) + 1;
	p = addrfix( p );

#if notdef
#ifdef HAVE_DSN
    if (strcmpic(name, "ORCPT") == 0) orcpt = string_copy(value);
    else if (strcmpic(name, "NOTIFY") == 0)
    {
      if (strcmpic(value, "NEVER") == 0) flags |= rf_notify_never; else
      {
        char *p = value;
        while (*p != 0)
        {
          char *pp = p;
          while (*pp != 0 && *pp != ',') pp++;
          if (*pp == ',') *pp++ = 0;
          if (strcmpic(p, "SUCCESS") == 0) flags |= rf_notify_success;
          else if (strcmpic(p, "FAILURE") == 0) flags |= rf_notify_failure;
          else if (strcmpic(p, "DELAY") == 0) flags |= rf_notify_delay;
          p = pp;
        }
      }
    }
#endif /* HAVE_DSN */
#endif
	if (setjmp(timerest)) {
		netreply( "451 Mail system problem\r\n" );
		return;
	}
	s_alarm (DTIMEOUT);
	if( rp_isbad( mm_wadr( (char *)0, p ))) {
		if( rp_isbad( mm_rrply( &thereply, &len )))
			netreply( "451 Mail system problem\r\n" );
		else {
			snprintf (replybuf, sizeof(replybuf), "451 %s\r\n", thereply.rp_line);
			netreply (replybuf);
		}
	} else {
		if( rp_isbad( mm_rrply( &thereply, &len )))
			netreply("451 Mail system problem\r\n");
		else if( rp_gbval( thereply.rp_val ) == RP_BNO) {
			snprintf (replybuf, sizeof(replybuf), "550 %s\r\n", thereply.rp_line);
			netreply (replybuf);
		}
		else if( rp_gbval( thereply.rp_val ) == RP_BTNO) {
			snprintf (replybuf, sizeof(replybuf), "451 %s\r\n", thereply.rp_line);
			netreply (replybuf);
		}
		else {
			snprintf (replybuf, sizeof(replybuf), "250 %s... Recipient ok\r\n", p);
			netreply (replybuf);
			numrecipients++;
		}
	}
	s_alarm (0);
}

/*
 *      ADDRFIX()  --  This function takes the SMTP "path" and removes
 *      the leading and trailing "<>"'s which would make the address
 *      illegal to RFC822 mailers.  Note that although the spec states
 *      that the angle brackets are required, we will accept addresses
 *      without them.   (DPK@BRL, 4 Jan 83)
 */
char *
addrfix( addrp )
char *addrp;
{
	register char   *cp;

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
#ifdef __STDC__
data(int cmdnr)
#else
data(cmdnr)
int cmdnr;
#endif
{
	static char prevbyte = '\n';
	register char *p, *bufptr;
	time_t  tyme;
	int     errflg, werrflg;
	struct rp_bufstruct thereply;
	int     len, msglen;

	errflg = werrflg = msglen = 0;
	if (numrecipients == 0) {
		netreply("503 No recipients have been specified.\r\n");
		return;
	}

	if (setjmp(timerest)) {
		netreply( "451 Mail system problem\r\n" );
		return;
	}
	s_alarm (DTIMEOUT);
	if( rp_isbad(mm_waend())) {
		netreply("451 Unknown mail system trouble.\r\n");
		return;
	}
	s_alarm (0);

	netreply ("354 Enter Mail, end by a line with only '.'\r\n");

	dont_mung = 1;      /* tell getline only to drop cr */
#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "... body of message ..." );
#endif
	while (1) {             /* forever */
		if ((p = getline()) == 0) {
			p = "\n***Sender closed connection***\n";
			mm_wtxt( p , strlen(p) );
			errflg++;
			break;
		}
		if (p == (char *)NOTOK) {
			p = "\n***Error on net connection***\n";
			mm_wtxt( p , strlen(p) );
			if (!errflg++)
				ll_log(logptr, LLOGTMP,
					"netread error from host %s", them);
			break;
		}

		/* are we done? */
		if (prevbyte == '\n' && buf[0] == '.')
			if (buf[1] == '\n')
				break;          /* yep */
			else
				bufptr = &buf[1];       /* skip leading . */
		else
			bufptr = &buf[0];

		prevbyte = lastbyte;		/* lastbyte is terminating byte
						   of previous getline() */

		/* If write error occurs, stop writing but keep reading. */
		if (!werrflg) {
			if (setjmp(timerest)) {
				netreply( "451 Mail system problem\r\n" );
				return;
			}
			s_alarm (DTIMEOUT);
			msglen += (len = p-bufptr);
			if( rp_isbad(mm_wtxt( bufptr, len ))) {
				werrflg++;
				ll_log( logptr, LLOGTMP, "error from submit");
			}
			s_alarm (0);
		}
	}
	dont_mung = 0;  /* set getline to normal operation */
#ifdef DEBUG
	ll_log( logptr, LLOGBTR, "Finished receiving text." );
#endif

	if (werrflg) {
		netreply("451-Mail trouble (write error to mailsystem)\r\n");
		netreply("451 Please try again later.\r\n");
		byebye( 1 );
	}
	if (errflg) {
	    time (&tyme);
	    byebye(1);
	}

	if (setjmp(timerest)) {
		netreply( "451 Mail system problem\r\n" );
		return;
	}
	s_alarm (DTIMEOUT);
	if( rp_isbad(mm_wtend()) || rp_isbad( mm_rrply( &thereply, &len)))
		netreply("451 Unknown mail trouble, try later\r\n");
	else if( rp_isgood(thereply.rp_val)) {
		snprintf (buf, sizeof(buf), "250 %s\r\n", thereply.rp_line);
		netreply (buf);
		phs_msg(chanptr, numrecipients, (long) msglen);
	}
	else if( rp_gbval(thereply.rp_val) == RP_BNO) {
		snprintf (buf, sizeof(buf), "554 %s\r\n", thereply.rp_line);
		netreply (buf);
	}
	else {
		snprintf (buf, sizeof(buf), "451 %s\r\n", thereply.rp_line);
		netreply (buf);
	}
	s_alarm (0);
	sender = (char *) 0;
	numrecipients = 0;
}

/*
 *  Process the RSET command
 */
#ifdef __STDC__
rset(int cmdnr)
#else
rset(cmdnr)
int cmdnr;
#endif
{
	mm_end( NOTOK );
	sender = (char *)0;
	mmdfstart();
	confirm();
}

mmdfstart()
{
	if( rp_isbad( mm_init() ) || rp_isbad( mm_sbinit() )) {
		ll_log( logptr, LLOGFAT, "can't reinitialize mail system" );
		netreply("421 Server can't initialize mail system (mmdf)\r\n");
		byebye( 2 );
	}
	numrecipients = 0;
}

/*
 *  handle the QUIT command
 */
#ifdef __STDC__
quit(int cmdnr)
#else
quit(cmdnr)
int cmdnr;
#endif
{
	time_t  timenow;

	time (&timenow);
	snprintf (buf, sizeof(buf), "221 %s says goodbye to %s at %.19s.\r\n",
		us, them, ctime(&timenow));
	netreply(buf);
	byebye( 0 );
}

byebye( retval )
int retval;
{
	if (retval == OK) {
		mm_sbend();
		phs_note(chanptr, PHS_REEND);
	}
	mm_end( retval == 0 ? OK : NOTOK );
	exit( retval );
}

/*
 *  Reply that the current command has been logged and noted
 */
#ifdef __STDC__
confirm(int cmdnr)
#else
confirm(cmdnr)
int cmdnr;
#endif
{
	netreply("250 OK\r\n");
}

/*
 *  Process the HELP command by giving a list of valid commands
 */
#ifdef __STDC__
help(int cmdnr)
#else
help(cmdnr)
int cmdnr;
#endif
{
	register int i;
	register struct comarr *p;
	char    replybuf[256];

	netreply("214-The following commands are accepted:\r\n214-" );
	for(p = commands, i = 1; p->cmdname; p++, i++) {
#ifdef HAVE_ESMTP
      if((!p->cmdmode) || (p->cmdmode &&
                           (chanptr->ch_access&CH_ESMTP)==CH_ESMTP))
#endif
      {
        snprintf (replybuf, sizeof(replybuf), "%s%s", p->cmdname, ((i%10)?" ":"\r\n214-") );
		netreply (replybuf);
      }
	}
	snprintf (replybuf, sizeof(replybuf), "\r\n214 Send complaints/bugs to:  %s\r\n", supportaddr);
	netreply (replybuf);
}

/*
 *  Send appropriate ascii responses over the network connection.
 */

netreply(string)
char *string;
{
	if (setjmp(timerest)) {
		byebye( 1 );
	}
	s_alarm (NTIMEOUT);
	if(write(1,string, strlen(string)) < 0){
		s_alarm( 0 );
		ll_log( logptr, LLOGFST,
			"(netreply) error (%d) in writing [%s] ...",
			errno, string);
		byebye( 1 );
	}
	s_alarm( 0 );
#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "%s", string);
#endif /* DEBUG */
}

/*
 *      expn
 *
 *      handle the EXPN command  ("EXPN user@host")
 */
#ifdef __STDC__
expn(int cmdnr)
#else
expn(cmdnr)
int cmdnr;
#endif
{

	if (arg == 0 || *arg == 0) {
		netreply("501 No argument supplied\r\n");
		return;
	}

	expn_count = 0;
	expn_str( addrfix(arg) );  /* feed fixed address into the expander */
	expn_dump();               /* dump the saved (last) address */
	return;
} 

expn_str( arg )
register char *arg;
{
	char    buf[LINESIZE];
	char	alstr[LINESIZE];
	char    comment[LINESIZE];
	register char    *p, *q;
	FILE    *fp;
	int	ret, gotalias, flags;
	Chan    *thechan;
	struct passwd	 *pw;

	gotalias=1;
	if ((ret = aliasfetch(TRUE, arg, buf, &flags)) != OK) {
		if (ret == MAYBE) {
			expn_save(MAYBE,"%s (Nameserver Timeout)",arg);
			return;
		}
		if ((pw = getpwmid (arg)) != (struct passwd *) NULL) {
			expn_save(OK,"%s <%s@%s>", pw->pw_gecos,
				pw->pw_name,us);
			return;
		}
		strncpy(buf, arg, sizeof(buf));
		gotalias--;
	}

	strncpy(alstr, arg, sizeof(alstr));

	/* just say OK if it is a private alias */
	if (gotalias && (flags & AL_PUBLIC) != AL_PUBLIC) {
		expn_save(OK,"<%s@%s>",alstr,us);
		return;
	}

	/* check for a simple comma-separated list of addresses */
	if ((p = strchr(buf, ',')) != 0 && strindex (":include:", buf) < 0 &&
	    strchr (buf, '<') == 0 && strchr (buf, '|') == 0 && gotalias)
	{
		/* q is start of substring; it lags behind p */
		for (q=buf; p != 0; q=p, p=strchr(p, ',')) {
			*p++ = '\0';  /* null the comma */
			expn_str(q);
		}
		return;
	}

	/* check for a simple RHS with no @'s and no /'s (e.g. foo:bar) */
	if (strchr (buf, '/') == 0)
	{
		/* check for a simple RHS (e.g. foo:bar) */
		if ((p = strchr (buf, '@')) == 0)
		{
			if( buf[0] == '~' ) {
				if ((pw=getpwmid (buf[1])) != (struct passwd *) NULL) {
					expn_save(OK,"%s <%s@%s>", pw->pw_gecos,
						pw->pw_name,us);
					return;
				}
				expn_save(NOTOK,"%s (Unknown address)",arg);
				return;
			}
			if (gotalias)
				return(expn_str(buf));  /* recurse */
			else {
				expn_save(NOTOK,"%s (Unknown address)",arg);
				return;
			}

		}

		/* check for aliases of the form: user@domain */
		*p++ = '\0';
		thechan=ch_h2chan(p,1);
		switch ((int) thechan) {
			case OK:
				return(expn_str(buf));  /* user@US -- recurse */
			case NOTOK:
			case MAYBE:
				break;
			default:
				/* check if list/bboard type channel */
				if (isstr(thechan->ch_host) &&
				    (thechan = ch_h2chan(thechan->ch_host,1))
								== (Chan *) OK)
					return(expn_str(buf));  /* recurse */
		}
		if (thechan == (Chan *) MAYBE) {
			expn_save(MAYBE,"%s@%s (Nameserver Timeout)",buf,p);
			return;
		}
		expn_save(OK,"<%s@%s>",buf,p);
		return;
	}

	if (!gotalias) {
		expn_save(NOTOK,"%s (Unknown Address)", arg);
		return;
	}

	/* Assume if multiple entries, that only the first one is used. */
	if ((q = strchr (buf, ',')) != 0)
		*q = '\0';

	/* check for aliases of the form: [user]|program */
	if ((q = strchr (buf, '|')) != 0) {
		*q++ = '\0';
		expn_save (OK,"%s@%s (Mail piped into process: %s)", 
			  alstr, us, q);
		return;
	}

	if ((q = strchr (buf, '/')) == 0) {
	        expn_save(NOTOK,"%s (Bad format for alias is %s)",alstr,buf);
		return;
	}

	/* check for < and :include: */
	if (strchr (buf, '<') != 0 || strindex (":include:", buf) >= 0) {
		if ((p = strchr (buf, '@')) != 0) {
			*p++ = '\0';
			if (ch_h2chan (p, 1)  != (Chan *) OK) {  /* not local */
				expn_save (OK,"<%s@%s>",buf,p);
				return;
			}

		}
		if ((fp = fopen (q, "r")) == NULL) {
			expn_save (NOTOK,"%s@%s (Unable to open file %s)",
				  alstr,us,q);
			return;
		}
		while (fgets (buf, LINESIZE, fp) != NULL) {
			*(buf+strlen(buf)-1) = '\0';
			expn_str(buf);
		}
		fclose (fp);
		return;
	}

	/* assume alias of the form:  [user]/file */
	*q++ = '\0';
	expn_save (OK,"%s@%s (Mail filed into %s)",
			alstr, us, q);
	return;


}

expn_save(code,pattern,a1,a2,a3,a4,a5,a6,a7)
int code;
char *pattern,
     *a1,*a2,*a3,*a4,*a5,*a6,*a7;
{
	if (expn_count > 0) {
		snprintf(buf, sizeof(buf),"250-%s\r\n",saveaddr);
		netreply(buf);
	}

	snprintf(saveaddr, sizeof(saveaddr),pattern,a1,a2,a3,a4,a5,a6,a7);
	if (expn_count == 0)
		savecode=code;
	else
		if (code != savecode)
			savecode=OK;
	expn_count++;
}

expn_dump()
{

	if (expn_count == 1 && savecode == NOTOK)
		snprintf(buf, sizeof(buf),"550 %s\r\n", saveaddr);
	else
		snprintf(buf, sizeof(buf),"250 %s\r\n", saveaddr);

	netreply(buf);
}

/*
 *      vrfy
 *
 *      handle the VRFY command  ("VRFY user@host")
 */
#ifdef __STDC__
vrfy(int cmdnr)
#else
vrfy(cmdnr)
int cmdnr;
#endif
{
	register int fd;
	register char *cp;
	char linebuf[LINESIZE];
	char replybuf[LINESIZE];
	struct rp_bufstruct thereply;
	int len;

	if (arg == 0 || *arg == 0) {
		netreply("501 No argument supplied\r\n");
		return;
	}

	if (!vrfy_child) {

		/* Start up the verifying child */

		if (pipe (vrfy_p2c) < OK || pipe (vrfy_c2p) < OK) {
			netreply("550 Mail system problem1.\r\n");
			return;
		}

		switch (vrfy_child = fork()) {

		    case NOTOK:
			ll_log(logptr, LLOGFST, "Fork of vrfy child failed.");
			netreply("550 Mail system problem2.\r\n");
			return;

		    default:
			/* parent */
			vrfy_out = fdopen(vrfy_p2c[1],"w");
			vrfy_in  = fdopen(vrfy_c2p[0],"r");
			break;

		    case 0:
			/* child */
#ifdef HAVE_DUP2
			dup2(vrfy_p2c[0],0);
			dup2(vrfy_c2p[1],1);
#else /* HAVE_DUP2 */
			(void) close(0);
			(void) close(1);
#ifdef HAVE_FCNTL_F_DUPFD
			fcntl(vrfy_p2c[0], F_DUPFD, 0);
			fcntl(vrfy_c2p[1], F_DUPFD, 1);
#else /* HAVE_FCNTL_F_DUPFD */
			/* something else */
#endif /* HAVE_FCNTL_F_DUPFD */
#endif /* HAVE_DUP2 */
			for (fd = 2; fd < numfds; fd++)
				(void)close(fd);

			if (rp_isbad(mm_init()) || rp_isbad(mm_sbinit()) ||
			    rp_isbad(mm_winit ((char *)0, "vmr", (char *)0)) ||
			    rp_isbad(mm_rrply( &thereply, &len )) ||
			    rp_isbad(thereply.rp_val)) {
				ll_log(logptr, LLOGFST, 
					"Vrfy child couldn't start up submit.");
				exit(1);
			}

			while (fgets (linebuf, LINESIZE, stdin)) {
				if (cp = strrchr(linebuf, '\n'))
					*cp-- = 0;
				verify(linebuf);
			}

			mm_end(NOTOK);
			exit(0);

		}

	}

	if (setjmp(timerest)) {
		ll_log(logptr,LLOGFST,"Timeout waiting for vrfy child.");
		netreply("550 Mail system problem.\r\n");
		vrfy_kill();
		return;
	}

	s_alarm(DTIMEOUT);
	fprintf(vrfy_out,"%s\n",arg);
	fflush(vrfy_out);
	if (ferror(vrfy_out)) {
		ll_log(logptr,LLOGFST,"Unable to write to vrfy child.");
		vrfy_kill();
		netreply("550 Mail system problem.\r\n");
		return;
	}
	if (fgets (linebuf, LINESIZE, vrfy_in) == NULL) {
		ll_log(logptr,LLOGFST,"Unable to read from vrfy child.");
		vrfy_kill();
		netreply("550 Mail system problem.\r\n");
		return;
	}
	s_alarm(0);

	if (cp = strrchr(linebuf, '\n'))
		*cp-- = 0;
	snprintf(replybuf, sizeof(replybuf),"%s\r\n",linebuf);
	netreply(replybuf);

	return;
}

verify(p)
char *p;
{
	register char *l, *r;
	struct rp_bufstruct thereply;
	char replybuf[256];
	int len;

	if (setjmp(timerest)) {
		ll_log(logptr,LLOGFST,
			         "Timeout in vrfy child waiting for submit.");
	        exit(1);
	}
	s_alarm (DTIMEOUT);
	if( rp_isbad( mm_wadr( (char *)0, p ))) {
		if( rp_isbad( mm_rrply( &thereply, &len ))) {
			ll_log(logptr,LLOGFST,
			      "Error in vrfy child reading reply from submit.");
		       exit(1);
		} else {
			snprintf (replybuf, sizeof(replybuf), "550 %s\r\n", thereply.rp_line);
			vrfyreply (replybuf);
		}
	} else {
		if( rp_isbad( mm_rrply( &thereply, &len ))) {
			ll_log(logptr,LLOGFST,
			     "Error in vrfy child reading reply from submit.");
			exit(1);
		} else if (rp_isbad(thereply.rp_val)) {
			snprintf (replybuf, sizeof(replybuf), "550 %s\r\n", thereply.rp_line);
			vrfyreply (replybuf);
		} else {
			if ((l=strchr(thereply.rp_line, '"')) &&
			    (r=strrchr(thereply.rp_line, '"')) &&
			    (l != r) ) {
				*l = '<';
			        *r = '>';
			}
			if (l && r && !strchr(l,'@')) {
			    *l++ = '\0';
			    *r++ = '\0';
			    snprintf (replybuf, sizeof(replybuf), "250 %s<%s@%s>%s\r\n",
 				   thereply.rp_line, l, us, r);
			} else
			    snprintf (replybuf, sizeof(replybuf), "250 %s\r\n", thereply.rp_line);
			vrfyreply (replybuf);
		}
	}
	s_alarm (0);

	return;
}

vrfyreply(string)
char *string;
{
	if (setjmp(timerest)) {
		exit(1);
	}
	s_alarm (NTIMEOUT);
	if(write(1,string, strlen(string)) < 0) {
		s_alarm(0);
		ll_log( logptr, LLOGFST,
			"(vrfyreply) error (%d) in writing [%s] ...",
			errno, string);
		exit(1);
	}
	s_alarm(0);
#ifdef DEBUG
	ll_log( logptr, LLOGFTR, "child writing: %s", string);
#endif /* DEBUG */
}

vrfy_kill()
{
	ll_log( logptr, LLOGFST, "Killing vrfy child.");

	if (vrfy_child) {
		kill (vrfy_child, 9);
		vrfy_child = 0;
	}

	if (vrfy_in != (FILE *) EOF && vrfy_in != NULL) {
		(void) close (fileno (vrfy_in));
		fclose(vrfy_in);
	}

	if (vrfy_out != (FILE *) EOF && vrfy_out != NULL) {
		(void) close (fileno (vrfy_out));
		fclose(vrfy_out);
	}

	vrfy_in = vrfy_out = NULL;

	return;
}
#ifdef HAVE_ESMTP
void tell_esmtp_options()
{
  char replybuf[LINESIZE];
  
  netreply("250-EXPN\r\n");
  
#  ifdef HAVE_ESMTP_VERB
  netreply("250-VERB\r\n");
#  endif /* HAVE_ESMTP_VERB */
#  ifdef HAVE_ESMTP_8BITMIME
  if(accept_8bitmime) netreply("250-8BITMIME\r\n");
#  endif /* HAVE_ESMTP_8BITMIME */
  if(message_size_limit>0)
    snprintf(replybuf, LINESIZE, "250-SIZE %d\r\n", message_size_limit);
  else snprintf(replybuf, LINESIZE, "250-SIZE\r\n");
  netreply(replybuf);
#  ifdef HAVE_ESMTP_DSN
  if(dsn) netreply("250-DSN\r\n");
#  endif /* HAVE_ESMTP_DSN */
#  ifdef HAVE_ESMTP_ONEX
  netreply("250-ONEX\r\n");
#  endif /* HAVE_ESMTP_ONEX */
#  ifdef HAVE_ESMTP_ETRN
  if(smtp_etrn_hosts) netreply("250-ETRN\r\n");
#  endif /* HAVE_ESMTP_ETRN */
#  ifdef HAVE_ESMTP_PIPE
  netreply("250-PIPELINING\r\n");
#  endif /* HAVE_ESMTP_PIPE */
#  ifdef HAVE_ESMTP_XUSR
  netreply("250-XUSR\r\n");
#  endif /* HAVE_ESMTP_XUSR */
  netreply("250 HELP\r\n");
}

#endif /* HAVE_ESMTP */


#if defined(HAVE_NAMESERVER) && defined(HAVE_RBL)
/*************************************************************/
extern void reject_message();

void rbl_reject(tblptr, host)
Table *tblptr;
char *host;
{
  char replybuf[LINESIZE];
  
  reject_message(tblptr, host, replybuf, sizeof(replybuf));
  netreply(replybuf);
  
}
#endif /* not HAVE_NAMESERVER and HAVE_RBL */
