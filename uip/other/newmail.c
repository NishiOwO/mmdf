/*
 * newmail - check for new mail
 *
 * The users mailbox is checked to see if they have new mail. The determination
 * is done in two steps.
 *
 *	1.	If a binary file from msg is found, the mailbox size from the
 *		header is compared with size of the actual mailbox.
 *	2.	Otherwise, the mailbox is scanned to find a message without a
 *		Status: line in the header.
 *
 * AUTHOR
 *	Edward C. Bennett <edward@engr.uky.edu>
 *      Mod by:  Mark Vasoll <vasoll@a.cs.okstate.edu> to fully understand
 *               MMDF's idea of the local maildrop location
 */
#include "util.h"
#include <sys/stat.h>
#include <pwd.h>
#include "../msg/msg.h"

char	mboxbuf[BUFSIZ], binboxbuf[BUFSIZ];

main()
{
	FILE		*fp, *fopen();
	struct	stat	statb;
	struct	passwd	*pw;
	char		*ctime(), *getlogin(), *thedir, *thefile;
	int		fd, new;

	thedir = mldfldir;

	if (mldfldir == NULL || *mldfldir == '\0') {
		if ((pw = getpwuid (getuid())) == NULL) {
			printf ("newmail: uid = %d nonexistant, get help\n",
				getuid());
			exit (1);
		}
		thedir = pw->pw_dir;
	}

	thefile = mldflfil;

	if (mldflfil == NULL || *mldflfil == '\0')
		thefile = getlogin();


	sprintf(mboxbuf, "%s/%s", thedir, thefile);
	if (stat(mboxbuf, &statb) == -1 || statb.st_size == 0)
		exit(0);	/* No mail at all */

	sprintf(binboxbuf, "%s/._%s", thedir, thefile);
	if ((fd = open(binboxbuf, 0)) != -1)
		new = uses_msg(fd, &statb);
	else if ((fp = fopen(mboxbuf, "r")) != NULL)
		new = uses_mail(fp);

	if (new)
		printf("You have new mail\n");
	else
		printf("You have mail\n");

	exit(0);
}

/*
 * uses_msg - compare old and new mailbox sizes.
 *
 * The header from the binary box is read and the size field is compared with
 * the size of the actual mailbox. If the mailbox is bigger we have new mail.
 */
uses_msg(fd, statp)
int fd;
struct stat *statp;
{
	struct	status	status;	/* from msg.h */

	read(fd, &status, sizeof(status));

	if (statp->st_size > status.ms_eofpos)
		return(1);

	return(0);
}

#define	NEITHER		0
#define	INHEADER	1
#define	INBODY		2

/*
 * uses_mail - scan the mailbox for Status: header lines.
 *
 * The mailbox is read and the message headers examined. As soon as we find
 * a message without a Status: line we know we have new mail.
 */
uses_mail(fp)
FILE *fp;
{
	char	buf[BUFSIZ];
	int	seen_status, state;

	state = NEITHER;
	while (fgets(buf, BUFSIZ, fp) != NULL) {
		if (strcmp(buf, delim1) == 0) {
			switch (state) {
			case NEITHER:
				state = INHEADER;
				seen_status = 0;
				break;
			case INBODY:
				state = NEITHER;
				break;
			}
			continue;
		}
		if (strlen(buf) == 1)
			if (seen_status == 0)
				return(1);
			else {
				state = INBODY;
				continue;
			}

		if (state == INHEADER && strncmp(buf, "Status:", 7) == 0)
			seen_status++;
	}

	return(0);
}
