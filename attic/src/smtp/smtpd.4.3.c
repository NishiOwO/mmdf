/*
 *			S M T P D . C
 *
 *	Server process that is designed to be called by INETD,
 *	the Internet Daemon of Daemons.  It calls SMTPSRVR
 *	with the proper arguments.
 *
 *		R E V I S I O N   H I S T O R Y
 */

#include "util.h"
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ns.h"

extern	int errno;
extern	char *inet_ntoa();

struct sockaddr_in addr;

int	facist = 0;		/* If set, tell smtpsrvr to dump unknowns */
char	*Smtpserver = 0;	/* Actual smtp server process, full path */
char	*Channel = 0;		/* Logical channel mail will arrive on */
char	obuf[BUFSIZ];		/* Logging will be line buffered */

main (argc, argv)
int	argc;
char	**argv;
{
	register int	skt;
	register struct hostent *hp;
	char	thishost[32];
	char	workarea[32];
	char *rmthost;
	struct sockaddr_in rmtaddr;
	int	len_rmtaddr = sizeof rmtaddr;
	int	status;
	int	on = 1;

#ifdef HAVE_NAMESERVER
	/* don't lose connection because NS takes too long */
	ns_settimeo(NS_NETTIME);
#endif /* HAVE_NAMESERVER */
	setbuf( stdout, obuf );
	gethostname( thishost, sizeof(thishost));	/* current hostname */

	/* Parse args; args will override configuration file... */
	arginit( argc, argv );

	if (getpeername (0, (struct sockaddr *)&rmtaddr, &len_rmtaddr) < 0)
		bomb( "getpeername failed (errno %d)",errno);

	(void) setsockopt (0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof on);

	hp = gethostbyaddr ( &rmtaddr.sin_addr,
		sizeof(rmtaddr.sin_addr), AF_INET );
	if ((hp == NULL) || !isstr(hp->h_name)) {
		strcpy(workarea, inet_ntoa(rmtaddr.sin_addr));
		rmthost = workarea;
	} else
		rmthost = hp->h_name;


	execl (Smtpserver, facist ? "rsmtpsrvr" : "smtpsrvr",
		rmthost, thishost, Channel, (char *)0);
	bomb( "server exec error (%d)", errno);
}

extern int optind;

arginit(argc, argv)
int	argc;
char	**argv;
{
	register	int	ch;

	while ((ch = getopt(argc, argv, "f")) != EOF) {
		switch (ch) {
		case 'f':
			facist++;
			break;

		default:
			bomb("Usage: %s [-f] smtpserver channels", argv[0]);
			break;
		}
	}

	if (optind == argc)
		bomb("Usage: %s [-f] smtpserver channels", argv[0]);

	if (access(argv[optind], 01) < 0)		/* execute privs? */
		bomb("Cannot access server program '%s'", argv[optind]);
	Smtpserver = argv[optind];
	Channel = argv[++optind];
}

/* VARARGS */
bomb( fmt, a, b, c, d )
char *fmt;
{
	fputs( "451 ", stdout );
	fprintf( stdout, fmt, a, b, c, d );
	fputs( "\r\n", stdout );
	fflush( stdout );
	exit (99);
}
