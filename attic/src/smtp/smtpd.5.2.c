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
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

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
	gethostname(thishost, sizeof(thishost));	/* current hostname */
	if ((sp = getservbyname("smtp", "tcp")) == NULL) {
		fprintf(stderr, "Cannot find service smtp/tcp\n");
		exit(-1);
	}
#ifdef NAMESERVER
	/* don't wait forever! */
	ns_settimeo(NS_NETTIME);
#endif /* NAMESERVER */

	/*
	 * try to get full name for thishost
	 */
	if ((hp = gethostbyname(thishost)) != NULL)
		strcpy(thishost, hp->h_name);

	/* Parse args; args will override configuration file... */
	arginit(argc, argv);

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

	while (1) {
		extern	char			*inet_ntoa();
			struct	sockaddr_in	rmtaddr;
			int			len_rmtaddr = sizeof rmtaddr;
			int			tmpskt;
			int			status;

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

			logx("%s started", rmt);
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
		while (waitpid (-1, &status, numconnections < Maxconnections
		    ? WNOHANG : 0) > 0)
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

	while ((ch = getopt(argc, argv, "dfn:")) != EOF) {
		switch (ch) {
		case 'd':
			debug++;
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
			log("Usage: %s [-df] [-n #connections] smtpserver channels", argv[0]);
			exit(1);
		}
	}

	if (optind == argc) {
		log("Usage: %s [-df] [-n #connections] smtpserver channels",
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
