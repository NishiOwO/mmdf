/*
 *			S M T P D . C
 *
 * Server SMTP daemon, for 4.1a/4.2 BSD by DPK at BRL, 1 Jan 82 (Sigh...)
 *
 *		R E V I S I O N   H I S T O R Y
 *
 *	02/02/83  MJM   Modified to be more robust.
 *
 *	03/18/83  DPK   Modified for 4.1c.
 *
 *	??		Modified for 4.2
 */

#include "util.h"
#include "conf.h"
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "ns.h"

extern	int	errno;

struct	sockaddr_in	addr;

char	*Smtpserver = 0;	/* Actual smtp server process, full path */
char	*Channel = 0;		/* Logical channel mail will arrive on */
int	Maxconnections = 4;	/* Maximum simultaneous connections */
int	numconnections = 0;	/* Number of currently active connections */
int	debug = 0;		/* If nonzero, give verbose output on stderr */
#define	logx	if (debug) log
char	errbuf[BUFSIZ];		/* Logging will be line buffered */
char	programid[40];		/* identification string for log entries */
int	stricked;		/* force full validity of source address */
int     started_by_inetd;

int     effecid,                  /* system number of pgm/file's owner  */
	callerid;                 /* who invoked me?                    */

extern char *inet_ntoa();
LOCFUN mn_mmdf();
RETSIGTYPE sig17();

main (argc, argv)
int argc;
char **argv;
{
	register	int		skt;
			struct	servent	*sp;
			struct	hostent	*hp;
			char		thishost[64];
			char		workarea[32];
			int		i, pid;

	setbuf(stderr, errbuf);
	sprintf(programid, "smtpd(%5.5d): ", getpid());
	getwho (&callerid, &effecid); /* who am I and who is running me?    */
	mmdf_init (argv[0]);
	gethostname(thishost, sizeof(thishost));	/* current hostname */
	if ((sp = getservbyname("smtp", "tcp")) == NULL) {
		fprintf(stderr, "Cannot find service smtp/tcp\n");
		exit(-1);
	}
#ifdef HAVE_NAMESERVER
	/* don't wait forever! */
	ns_settimeo(NS_NETTIME);
#endif /* HAVE_NAMESERVER */

	/*
	 * try to get full name for thishost
	 */
	hp = gethostbyname(thishost);
	if (hp != NULL)	strcpy(thishost, hp->h_name);

	/* Parse args; args will override configuration file... */
	arginit(argc, argv);

	if(started_by_inetd) {
	  char *rmthost;
	  struct sockaddr_in rmtaddr;
	  int     len_rmtaddr = sizeof rmtaddr;
	  int     on = 1;
	
	  mn_mmdf();           /* set up effective and group id's properly */
	  if (getpeername (0, (struct sockaddr *)&rmtaddr, &len_rmtaddr) < 0)
	    bomb( "getpeername failed (errno %d)",errno);

	  (void) setsockopt (0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);
	  hp = gethostbyaddr ( (char *)&rmtaddr.sin_addr,
			       sizeof(rmtaddr.sin_addr), AF_INET );
	  if ((hp == NULL) || !isstr(hp->h_name)) {
	    strcpy(workarea, (char *)inet_ntoa(rmtaddr.sin_addr));
	    rmthost = workarea;
	  } else
	    rmthost = hp->h_name;
	  
	  execl (Smtpserver, stricked ? "rsmtpsrvr" : "smtpsrvr",
		 rmthost, thishost, Channel, (char *)0);
	  bomb( "server exec error (%d)", errno);
	}
	/* now smtpd is running standalone */

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = (short)sp->s_port;

#ifdef never
	if ((i = open("/dev/tty", 2)) >= 0) {
		(void) ioctl(i, TIOCNOTTY, (char *)0);
		(void) close(i);
	}
#endif

	/*
	 * Create a socket to accept a SMTP connection on.
	 * The doloop is due to some sort of IPC bug in 4.1a.
	 */
	logx("opening socket...");
	i = 60;
	do {
		skt = socket(AF_INET, SOCK_STREAM, 0);
		logx("socket returned %d", skt);
		if (skt < 0)
			sleep(5);
	} while (skt < 0 && i-- > 0);
	if (skt < 0) {
		logx("can't open socket (errno %d)", errno);
		exit(99);
	}
	logx("socket open on %d", skt);

	if (bind (skt, (char *)&addr, sizeof addr) < 0) {
		logx("can't bind socket (errno %d)", errno);
		exit(98);
	}
	listen (skt, Maxconnections+1);
	logx("old with uid=%d, euid=%d", getuid(), geteuid());
	logx("old with gid=%d, egid=%d", getgid(), getegid());
	mn_mmdf();           /* set up effective and group id's properly */

	logx("running with uid=%d, euid=%d", getuid(), geteuid());
	logx("running with gid=%d, egid=%d", getgid(), getegid());
#ifdef LINUX
	signal( SIGCHLD, sig17);
#endif
    
	while (1) {
		extern	char			*inet_ntoa();
			struct	sockaddr_in	rmtaddr;
			int			len_rmtaddr = sizeof rmtaddr;
			int			status;
			int			tmpskt;

		/*
		 * Accept a connection.
		 */
		if ((tmpskt = accept(skt, &rmtaddr, &len_rmtaddr)) < 0) {
			logx("accept error (%d)", errno);
			sleep(1);
			continue;
		}

		/* We have a valid open connection, start a server... */
		if ((pid = fork()) < 0) {
			logx("could not fork (%d)", errno);
			close(tmpskt);
		} else if (pid == 0) {
			/*
			 * Child
			 */
			char	*rmt;

			hp = gethostbyaddr(&rmtaddr.sin_addr,
					sizeof(rmtaddr.sin_addr), AF_INET);
			if ((hp == NULL) || !isstr(hp->h_name)) {
				strcpy(workarea, inet_ntoa(rmtaddr.sin_addr));
				rmt = workarea;
			}
			else
				rmt = hp->h_name;

			logx("(%d) %s started", numconnections, rmt);
			dup2(tmpskt, 0);
			dup2(tmpskt, 1);
			if (tmpskt > 1)
				close(tmpskt);
			execl(Smtpserver, stricked? "rsmtpsrvr" : "smtpsrvr",
					rmt, thishost, Channel, (char *)0);
			logx("server exec error (%d)", errno);
			exit(99);
		}

		/*
		 * Parent
		 */
		close(tmpskt);
		numconnections++;

		/*
		 * This code collects ZOMBIES and implements load
		 * limiting by staying in the do loop while the
		 * Maxconnections active.
		 */
#if HAVE_WAIT3
		while (wait3 (&status, numconnections < Maxconnections
			      ? WNOHANG : 0, 0) > 0)
#else /* HAVE_WAIT3 */
		  while (waitpid (-1, &status, numconnections < Maxconnections
				  ? WNOHANG : 0) > 0)
#endif /* HAVE_WAIT3 */
		    numconnections--;
	}
}

arginit(argc, argv)
int argc;
char **argv;
{
	register	int	ch;
	extern		char	*optarg;
	extern		int	optind;

	while ((ch = getopt(argc, argv, "difn:")) != EOF) {
		switch (ch) {
		case 'd':
			debug++;
			break;

		case 'i':	/* force correctness of addresses */
			started_by_inetd++;
			break;

		case 'f':	/* force correctness of addresses */
			stricked++;
			break;

		case 'n':
			Maxconnections = atoi(optarg);
			if (Maxconnections <= 0) {
				logx("Bad number of connections '%s'", optarg);
				exit(99);
			}
			logx("Maxconnection now %d", Maxconnections);
			break;

		default:
			log("Usage: %s [-dif] [-n #connections] smtpserver channels", argv[0]);
			exit(1);
		}
	}

	if (optind == argc) {
		log("Usage: %s [-dif] [-n #connections] smtpserver channels",
			argv[0]);
		exit(1);
	}

	if (optind < argc) {
		if (access(argv[optind], 01) < 0) {	/* execute privs? */
			logx("Cannot access server program '%s'", argv[optind]);
			exit(99);
		}
		Smtpserver = argv[optind];
		logx("server is '%s'", Smtpserver);
	} else {
		logx("Smtpserver program not specified!");
		exit(99);
	}

	if (++optind < argc) {
		Channel = argv[optind];
		logx("channel is '%s'", Channel);
	} else {
		logx("Channel not specified!");
		exit(99);
	}
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
RETSIGTYPE sig17()
{
  int	status, pid;
  logx("sig17(): got SIGCHLD");
  signal( SIGCHLD, sig17);
#if HAVE_WAIT3
  pid = wait3 (&status, WNOHANG, 0);
#else /* HAVE_WAIT3 */
  pid = waitpid (-1, &status,  WNOHANG);
#endif /* HAVE_WAIT3 */
  logx("sig17():");
  if(pid>0) numconnections--;
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* VARARGS */
log(fmt, a, b, c, d)
char *fmt;
int a, b, c, d;
{
	fputs(programid, stderr);
	fprintf(stderr, fmt, a, b, c, d);
	fputc('\n', stderr);
	fflush(stderr);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
bomb( fmt, a, b, c, d )
char *fmt;
{
        fputs( "451 ", stdout );
        fprintf( stdout, fmt, a, b, c, d );
        fputs( "\r\n", stdout );
        fflush( stdout );
        exit (99);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
LOCFUN
        mn_mmdf ()		  /* setuid to mmdf: bypass being root  */
{				  /* get sys id for mmdf; setuid to it  */
    extern char *pathsubmit;     /* submit command file name           */
    extern char *cmddfldir;      /* directory w/mmdf commands          */
    char    temppath[LINESIZE];
    struct stat    statbuf;

#ifdef DEBUG
    logx ("mn_mmdf(); effec==%d", effecid );
#endif

/*  the following is a little strange, doing a stat on the object
 *  file, because setuid-on-execute does not work when the caller
 *  is root, as will happen when this is started by the rc file.
 *  hence, the effective id, from a getuid, will show root & not mmdf.
 *
 *  the goal is to have this process be name-independent of the caller,
 *  so that returned mail comes from mmdf and not the invoker.
 *
 *  in pickup mode, the id of the caller has to be retained, since
 *  pobox channels use that to determine access rights to mail.
 *
 *  All sets gid to mmdf's gid  --  <DPK@BRL>
 */

    if (effecid == 0)
    {
	getfpath (pathsubmit, cmddfldir, temppath);

	if (stat (temppath, &statbuf) < OK)
	    bomb ("Unable to stat %s", temppath);
				  /* use "submit" to get mmdf id        */

	if (setgid (statbuf.st_gid) == NOTOK)
	    bomb ("Can't setgid to mmdf (%d)", statbuf.st_gid);
	if (setuid (statbuf.st_uid) == NOTOK)
	    bomb ("Can't setuid to mmdf (%d)", statbuf.st_uid);

	effecid = statbuf.st_uid; /* mostly needed for return mail      */
    }
}
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
