/* name:
	input

function:
	To accept the text of the message and allow ed, msg
	to be called.

algorithm:
	Input header information and text, then ask for a valid command

parameters:
	none

returns:
	With a null terminated letter in the string pointed to by
	the global variable letter.

globals:
	letter	address of the buffer.

calls:
	write	system
	printf
	getchar
	system  system

called by:
	main
 */
/*
**	R E V I S I O N  H I S T O R Y
**
**	03/31/83  GWH	Split the SEND program into component parts
**
**	04/05/83  GWH	Changed execl calls to execlp's.
**
**	11/21/83  GWH	Added directedit option.
**
**	05/29/84  GWH	Fixed a bug that allowed drafts created in an edit
**			mode to be deleted if an interrupt was made by
**			mistake. Also fixed a bug which resulted in the
**			copy flag (cflag) being erroneously set.
**
**	04/02/85 M. Vasoll  Added the spelling checker command.
**
**	01/29/86 TRT	Changed the input scheme so that the user is placed
**			directly into a visual editor, and is free to edit
**			both the headers and body.  Also removed the
**			following options:
**				direct edit, header edit, input more body,
**				delete body, visual edit (same as edit)
**
**	07/20/94 Mike Muuss	Added "sign" and "encrypt" commands.
*/

#include "./s.h"
#include "./s_externs.h"

extern  char *verdate;
extern  int errno;
extern	RETSIGTYPE onint (), onint2(), onint3 ();
int    bccflag;

/* Initialized for PGP 5.0 */
char	sign_cmd[128] = "pgps --armor -f -t";
char	encrypt_cmd[128] = "pgpe --armor -f -t";

#ifndef STDC_HEADERS
extern char	*malloc();
#endif /* STDC_HEADERS */

input ()
{
    int     infile;
    int     n;
    long    dist;
    int     nfd;
    int     stat, slen;
    char    otherbuf[S_BSIZE];
    char    tempbuf[24];
    char    *fromptr, *toptr, *ccptr, *subjptr, *bccptr, *otherptr;
    RETSIGTYPE     (*old1) (),
	    (*old2) (),
	    (*old3) ();
    register int    i;

    printf ("SND  (%s)\n", verdate);

	bccflag = 0;
	dropen(DRBEGIN);

/*	Set up headers to be written into the draft file	*/
	fromptr = from;
	toptr = to;
	ccptr = cc;
	subjptr = subject;
	bccptr = bcc;
	otherptr = otherbuf;

	strcpy(from, signature);
	strcpy(otherptr, fromname);
	strcat(otherptr, fromptr);
	strcpy(fromptr, otherptr);

	strcpy(otherptr, toname);
	strcat(otherptr, toptr);
	strcpy(toptr, otherptr);

	strcpy(otherptr, ccname);
	strcat(otherptr, ccptr);
	strcpy(ccptr, otherptr);

	slen = strlen( subjname ); 
	strcpy(otherptr, subjname);
	strcat(otherptr, subjptr);
	strcpy(subjptr, otherptr);
	
/*	Write the header info to the draft file		*/

	if(write(drffd, fromptr, strlen(fromptr)) < 0)
	{
		printf("Cannot write draft %s\n", drffile);
		s_exit(-1);
	}
	write(drffd, "\n", 1);
	write(drffd, toptr, strlen(toptr));
	write(drffd, "\n", 1);
	write(drffd, ccptr, strlen(ccptr));
	write(drffd, "\n", 1);

/*	If there are blind carbon copies write the necessary header b.s. */
	if(bcc[0] != '\0')
	{
		strcpy(otherptr, bccname);
		strcat(otherptr, bccptr);
		strcpy(bccptr, otherptr);
		write(drffd, bccptr, strlen(subjptr));
		write(drffd, "\n", 1);


		bccflag = 1;
	}
	if( strlen( subject ) > slen )
	{
		char newsubj[S_BSIZE], *p, *pnt;
		int plc;

		newsubj[0] = '\0';
		pnt = subjptr;
		while( (p = strchr(pnt,'\n')) != 0 )
		{
			*p++ = '\0';
			plc = strlen(newsubj);
			sprintf( &newsubj[plc],"%s\n%s",pnt," ");
			pnt = p;
		}
		strcat( newsubj, pnt );
		strcpy( subjptr, newsubj );
	}
	write(drffd, subjptr, strlen(subjptr));
	write(drffd, "\n\n", 2);

/*	If there is a forwarded message		*/
	if (inclfile[0] != '\0')
	{                             /* tack file to end of message        */
		body = TRUE;
		strcpy (bigbuf, inclfile);
		infile = open (bigbuf, 0);
		if (inclfile[0] != '\0')
			inclfile[0] = '\0';
		if (infile < 0)
			printf ("can't open: '%s'\n", bigbuf);
		dropen (DREND);
		body = TRUE;
		while ((i = read (infile, bigbuf, BBSIZE - 1)) > 0)
			write (drffd, bigbuf, i);
		(void) close (infile);
	}

edit:

	fflush (stdout);

        drclose ();

        old1 =  signal (SIGHUP, SIG_IGN); /* ignore signals intended for edit */
        old2 =  signal (SIGINT, SIG_IGN);
        old3 =  signal (SIGQUIT, SIG_IGN);

/*	Crank up the editor now that the draft file is set-up	*/
        if (fork () == 0)
        {
	    signal (SIGHUP, old1);
	    signal (SIGINT, orig);
	    signal (SIGQUIT, old3);

            execlp (editor, editor, drffile, (char *)0);

	    fprintf (stderr,"can't execute\n");
	    s_exit (-1);
        }
        wait (&stat);
        body = TRUE;	/* Don't know but assume he created a draft */
        lastsend = 0;
        signal (SIGHUP, old1);     /* restore signals */
        signal (SIGINT, old2);
        signal (SIGQUIT, old3);

        fflush (stdout);
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);

	dropen(DRBEGIN);
	
/*	Rip the header information from the draft file and
**	take care of any aliases
**	Algorithm:
**	Read each header line (function grabaddr), keeping track of the 
**	number of characters read.  Handle the aliases (function doalias).
**	Open a new file descriptor which points to the beginning of the
**	draft file.  Read the draft file from the point where the header
**	information leaves off while writing to the draft from the beginning.
**	This effectively reads the header info, then shifts the body of the
**	message up to the top of the draft, overwriting the header.
*/

	dist = gethdr( fromptr, toptr, ccptr, bccptr, subjptr );

	doalias( toptr );
	doalias( ccptr );
	doalias( bccptr );

	if(chmod(drffile, 0600) < 0)
	{
		fprintf(stderr,"Unable to make draft writable %s.\n", drffile);
		s_exit(-1);
	}
	if((nfd = open(drffile, 2, 600)) < 0)
	{
		fprintf(stderr,"Could not open draft %s\n", drffile);
		s_exit(-1);
	}
	if(lseek(nfd, 0L, 0) < 0)
	{
		fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		s_exit(-1);
	}
	if(lseek(drffd, dist-1, 0) < 0)
	{
		fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		s_exit(-1);
	}

	dist = 0L;
	while((i = read(drffd, bigbuf, BBSIZE -1)) > 0)
		dist += write(nfd, bigbuf, i);

	if(truncate(drffile, dist) < 0)
	{
		fprintf(stderr,"Truncate failed on draft %s\n", drffile);
		s_exit(-1);
	}

	if(chmod(drffile, 0400) < 0)
		fprintf(stderr,"Unable to make draft %s writable.\n",drffile);
	
	(void) close( nfd );

    setjmp (savej);
    nsent = 0;
/*
**	MAIN COMMAND INTERPRETER LOOP
*/
    for (;;)
    {
    	char *cp;

	signal (SIGINT, onint3);
	printf ("Command or ?: ");
	fflush (stdout);
	aborted = FALSE;
	if (fgets (bigbuf, BBSIZE, stdin) == NULL)
	    goto byebye;
    	if (cp = strchr(bigbuf, '\n'))
    	    *cp = '\0';

/*	"Set" option	*/
	if( strncmp( "set", bigbuf, 3) == 0){
		char *av[NARGS];

		i = sstr2arg(bigbuf, 20, av, " \t");
		if( i == 1 ) {	/* tell him what the current options are */
			printf(" Sign_cmd  - '%s'\n", sign_cmd);
			printf(" Encrypt_cmd '%s'\n", encrypt_cmd);
			printf(" Copyfile  - '%s'\n", copyfile);
			printf(" Signature - '%s'\n", signature);
			printf(" Aliases   - '%s'\n", aliasfilename);
			printf(" Editor    - '%s'\n", editor);
			printf(" Checker   - '%s'\n", checker);
			printf(" Subargs   - '%s'\n", subargs);

			if(qflag == 0 && cflag == 1)
				printf(" File copy option is on\n");
			if(qflag == 1 && cflag == 0)
				printf(" File copy on confirmation only\n");
			continue;
		}
		if( (i = picko( av[1] ) ) < 0) {
			fprintf(stderr,"Option '%s' is invalid.\n", av[1]);
			continue;
		}
		select_o( i, &av[1] );
		continue;
	}
	compress (bigbuf, bigbuf);
				  /* so prefix works on multi-words     */
	if (bigbuf[0] == '\0')
	    continue;             /* just hit newline                   */

	signal (SIGINT, onint);

	for (i = 0; !isnull (bigbuf[i]); i++)
	    bigbuf[i] = uptolow (bigbuf[i]);

	if (prefix ("bcc", bigbuf))
	{
	    getaddr (bccname, bcc, YES, locname);
	    bccflag = 1;	
	    continue;
	}

/*	"Edit" option (grandfathered to also accept "v" for visual edit) */

	if (prefix ("edit body", bigbuf) || prefix("visual edit body", bigbuf))
	{
		dropen( DRBEGIN );
    		if((tmpfd = open(tmpdrffile, 2, 0600)) < 0)
    		{
    			fprintf(stderr,"Cannot open temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}

    		if(lseek(tmpfd, 0L, 0) < 0)
    		{
    			fprintf(stderr,"Lseek failed on temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}

    		if(chmod(drffile, 0600) < 0)
    			fprintf(stderr,"Could not make draft %s writable\n", drffile);

		/* 	Write the header info to the temporary draft	*/
		dist = 0L;
		dist += write(tmpfd, fromname, strlen( fromname ));
    		dist += write(tmpfd, from, strlen( from ));
    		dist += write(tmpfd, "\n", 1);
    		dist += write(tmpfd, toname, strlen( toname ));
    		dist += write(tmpfd, to, strlen( to ));
    		dist += write(tmpfd, "\n", 1);
    		dist += write(tmpfd, ccname, strlen( ccname ));
    		dist += write(tmpfd, cc, strlen( cc ));
    		dist += write(tmpfd, "\n", 1);
    		if(bccflag)
    		{
    			dist += write(tmpfd, bccname, strlen( bccname ));
    			dist += write(tmpfd, bcc, strlen( bcc ));
    			dist += write(tmpfd, "\n", 1);
    		}
    		dist += write(tmpfd, subjname, strlen( subjname ));
    		dist += write(tmpfd, subject, strlen( subject ));
    		dist += write(tmpfd, "\n", 1);

		if(lseek(drffd, 0L, 0) < 0)
		{
		 	fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		 	s_exit(-1);
		}
    		if(lseek(tmpfd, 0L, 2) < 0)
    		{
    			fprintf(stderr,"Lseek failed on temporary draft\n");
    			s_exit(-1);
    		}

		/*	Write the body into the temp. draft	*/
    		while((i = read(drffd, bigbuf, BBSIZE-1)) > 0)
    			dist += write(tmpfd, bigbuf, i);

    		if(truncate( tmpdrffile, dist) < 0)
    		{
    			fprintf(stderr,"Could not truncate temporary draft\n");
    			s_exit(-1);
    		}

    		if(lseek(tmpfd, 0L, 0) < 0)
    			fprintf(stderr,"Lseek failed on temporary draft\n");
    		if(lseek(drffd, 0L, 0) < 0)
    			fprintf(stderr,"Lseek failed on draft %s\n", drffile);
    		
    		dist = 0L;
		/*	Copy temp. draft which contains headers and
		**	body back into draft file
		*/
    		while((i = read(tmpfd, bigbuf, BBSIZE-1)) > 0)
    			dist += write(drffd, bigbuf, i);

    		if(truncate( drffile, dist ) < 0)
    			fprintf(stderr,"Could not truncate draft %s\n", drffile);
    		if(truncate( tmpdrffile, 0 ) < 0)
    			fprintf(stderr,"Could not truncate temporary draft\n");
    		if(close( tmpfd ) < 0)
    			fprintf(stderr,"Could not close temporary draft\n");
    		if(lseek(drffd, 0L, 0) < 0)
    			fprintf(stderr,"Lseek failed on draft %s\n", drffile);
    		if(chmod(drffile, 0400) < 0)
    			fprintf(stderr,"Could not protect draft %s\n", drffile);
		/*	Fire-up the editor	*/
		goto edit;
	}

/*	"File include" option	*/
	if (prefix ("file include", bigbuf))
	{
	    printf ("File: ");
	    gather (bigbuf, BBSIZE - 1);
	    if (bigbuf[0] == '\0')
		continue;

	    infile = open (bigbuf, 0);
	    if (inclfile[0] != '\0')
	    {                     /* and include file                   */
		inclfile[0] = '\0';
	    }
	    if (infile < 0)
	    {
		fprintf(stderr,"can't open: '%s'\n", bigbuf);
		continue;
	    }
	    dropen (DREND);
	    body = TRUE;
	    while ((i = read (infile, bigbuf, BBSIZE - 1)) > 0)
		write (drffd, bigbuf, i);
	    (void) close (infile);
	    printf (" ...included\n");
	    continue;
	}
/*	"Program run" option	*/
	if (prefix ("program run", bigbuf))
	{
	    printf ("Program: ");
	    fflush (stdout);
	    if (fgets (tempbuf, BBSIZE, stdin) == NULL)
		continue;
	    if (cp = index(bigbuf, '\n'))
    		*cp = '\0';
	    drclose ();

	    /* ignore signals intended for subprocess */
	    old1 = signal (SIGHUP, SIG_IGN);
	    old2 = signal (SIGINT, SIG_IGN);
	    old3 = signal (SIGQUIT, SIG_IGN);

	    if (fork () == 0)
	    {
		signal (SIGHUP, old1);
		signal (SIGINT, orig);
		signal (SIGQUIT, old3);
		execlp ("sh", "send-shell", "-c", tempbuf, (char *)0);
		fprintf(stderr,"can't execute\n");
		s_exit (-1);
	    }
	    wait (&stat);
	    signal (SIGHUP, old1);     /* restore signals */
	    signal (SIGINT, old2);
	    signal (SIGQUIT, old3);
	    dropen (DREND);
	    continue;
	}
/*	"quit" option	*/
	if (prefix ("quit", bigbuf) ||
		prefix ("bye", bigbuf))
	{
    byebye: 
	    if ( body && !nsent)
	    {
		printf ("Without sending draft");
		if (confirm ())
		    return;
		else
		    continue;
	    }
	    drclose ();
	    return;
	}
/*	"review message" option	*/
	if (prefix ("review message", bigbuf))
	{
		dropen( DRBEGIN );
		/*	Put header info into temp. draft	*/
		if((nfd = open(tmpdrffile, 2, 0600)) < 0)
		{
			fprintf(stderr,"Can't open temporary file\n");
			s_exit(-1);
		}
		if(lseek(nfd, 0L, 0) < 0)
		{
			fprintf(stderr,"Lseek failed on temporary file\n");
			s_exit(-1);
		}
		if(write(nfd, fromname, strlen( fromname )) < 0)
		{
			fprintf(stderr,"Cannot write temporary file\n");
			s_exit(-1);
		}
		write(nfd, from, strlen( from ));
		write(nfd, "\n", 1);
		if(to[0] != '\0')
		{
			write(nfd, toname, strlen( toname ));
			write(nfd, to, strlen( to ));
			write(nfd, "\n", 1);
		}
		else if(bccflag)
		{
			write(nfd, toname, strlen( toname ));
			write(nfd, "list: ;", 7);
			write(nfd, "\n", 1);
		}
		if(cc[0] != '\0')
		{
			write(nfd, ccname, strlen( ccname ));
			write(nfd, cc, strlen( cc ));
			write(nfd, "\n", 1);
		}
		if(bccflag)
		{
			write(nfd, bccname, strlen( bccname ));
			write(nfd, bcc, strlen( bcc ));
			write(nfd, "\n", 1);
		}			
		if(subject[0] != '\0')
		{
			write(nfd, subjname, strlen( subjname ));
			write(nfd, subject, strlen( subject ));
			write(nfd, "\n", 1);
		}

		if(lseek(drffd, 0L, 0) < 0)
			fprintf(stderr,"Lseek failed on draft %s\n", drffile);
	/*	Write the body into the temp. file	*/
		while((i = read(drffd, bigbuf, BBSIZE - 1)) > 0)
			write(nfd, bigbuf, i);
		drclose();
	   if(pflag)
	   {
		(void) close( nfd );
			
		    /* ignore signals intended for subprocess */
		    old1 = signal (SIGHUP, SIG_IGN);
		    old2 = signal (SIGINT, SIG_IGN);
		    old3 = signal (SIGQUIT, SIG_IGN);
		/*	Start-up "more" program on the temp. draft	*/
		    if (fork () == 0)
		    {
			signal (SIGHUP, old1);
			signal (SIGINT, orig);
			signal (SIGQUIT, old3);
			execlp ("more", "-c", tmpdrffile, 0);
			fprintf(stderr,"can't execute\n");
			s_exit (-1);
		    }
		    wait (&stat);
		    signal (SIGHUP, old1);     /* restore signals */
		    signal (SIGINT, old2);
		    signal (SIGQUIT, old3);

	   }
	   else	/*	If the paging option is off, just show it */
	   {
		if( lseek(nfd, 0L, 0) < 0 )
			fprintf(stderr, "Lseek failed on temporary draft.\n");

		while((i = read(nfd, bigbuf, BBSIZE - 1)) > 0)
			write( 1, bigbuf, i);

		(void) close( nfd );
	   }
	   if(truncate(tmpdrffile, 0) < 0)
		fprintf(stderr,"truncate failed on temp. draft\n");

	   fflush( stdout );
	   continue;
	}
/*	"Check spelling" option	*/
	if (prefix ("check spelling", bigbuf))
	{
	    drclose ();

	    old1 =  signal (SIGHUP, SIG_IGN); /* ignore signals intended for cheker */
	    old2 =  signal (SIGINT, SIG_IGN);
	    old3 =  signal (SIGQUIT, SIG_IGN);

	    if (fork () == 0)
	    {
		signal (SIGHUP, old1);
		signal (SIGINT, orig);
		signal (SIGQUIT, old3);
		execlp (checker, checker, drffile, (char *)0);
		fprintf(stderr,"can't execute\n");
		s_exit (-1);
	    }
	    wait (&stat);
	    signal (SIGHUP, old1);     /* restore signals */
	    signal (SIGINT, old2);
	    signal (SIGQUIT, old3);
	    dropen (DREND);
	    continue;
	}
/*	"Post message" option	*/
	if (prefix ("post message", bigbuf) ||
		prefix ("send message", bigbuf))
	{
	    if (lastsend && lastsend == body)
	    {
		printf ("Without changing anything");
		if (!confirm ())
		    break;
	    }
	    signal (SIGINT, onint2);
	    if( qflag == 1 ){
		printf(" Do you want a file copy of this message ?  ");
		fflush(stdout);
		fgets(tempbuf, sizeof(tempbuf), stdin );
		if( tempbuf[0] == 'y' || tempbuf[0] == 'Y' || tempbuf[0] == '\n')
			cflag = 1;
	    	else
	    		cflag = 0;
	    }
	    post ();
	    if ( qflag == 1 )
		cflag = 0;

/* *** auto-exit, upon successfull send, since draft is saved *** */

	    goto byebye;
	}

/*	Sign message with digital signature  -MJM */

	if (prefix ("sign message with digital signature", bigbuf) )
	{
		dropen( DRBEGIN );
    		if((tmpfd = open(tmpdrffile, 2, 0600)) < 0)
    		{
    			fprintf(stderr,"Cannot open temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}
    		if(chmod(drffile, 0600) < 0)
    			fprintf(stderr,"Could not make draft %s writable\n", drffile);

		/* Skip over leading blank line in input file. */
		if(lseek(drffd, 1L, 0) < 0)
		{
		 	fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		 	s_exit(-1);
		}
    		/* Process from draft file (no headers) to temp draft file */
		if( run_filter( sign_cmd, drffd, tmpfd, 0 ) < 0 )  {
			/* Signature failed, don't change draft */
			fprintf(stderr,"Digital signature failed, draft unchanged\n");
			continue;
		}

    		if(lseek(tmpfd, 0L, 0) < 0)
    		{
    			fprintf(stderr,"Lseek failed on temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}
		if(lseek(drffd, 0L, 0) < 0)
		{
		 	fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		 	s_exit(-1);
		}

		/* Start draft file with blank line again */
		write( drffd, "\n", 1 );
    		dist = 1L;

		/* Copy temp. draft (no headers) back into draft file */
    		while((i = read(tmpfd, bigbuf, BBSIZE-1)) > 0)
    			dist += write(drffd, bigbuf, i);

    		if(truncate( drffile, dist ) < 0)
    			fprintf(stderr,"Could not truncate draft %s\n", drffile);
    		if(truncate( tmpdrffile, 0 ) < 0)
    			fprintf(stderr,"Could not truncate temporary draft\n");
    		if(close( tmpfd ) < 0)
    			fprintf(stderr,"Could not close temporary draft\n");
    		if(lseek(drffd, 0L, 0) < 0)
    			fprintf(stderr,"Lseek failed on draft %s\n", drffile);
    		if(chmod(drffile, 0400) < 0)
    			fprintf(stderr,"Could not protect draft %s\n", drffile);
		printf("\n");
		continue;
	}

/*	Encrypt message body  -MJM */

	if (prefix ("encrypt message body", bigbuf) )
	{
		/* XXX Perhaps confirmation should be requested? */
		dropen( DRBEGIN );
    		if((tmpfd = open(tmpdrffile, 2, 0600)) < 0)
		{
    			fprintf(stderr,"Cannot open temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}
    		if(chmod(drffile, 0600) < 0)
    			fprintf(stderr,"Could not make draft %s writable\n", drffile);


		/* Skip over leading blank line in input file. */
		if(lseek(drffd, 1L, 0) < 0)
		{
		 	fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		 	s_exit(-1);
		}

    		/* Process from draft file (no headers) to temp draft file */
		if( run_filter( encrypt_cmd, drffd, tmpfd, 1 ) < 0 )  {
			/* Signature failed, don't change draft */
			fprintf(stderr,"Encryption failed, draft unchanged\n");
			continue;
		}

    		if(lseek(tmpfd, 0L, 0) < 0)
    		{
    			fprintf(stderr,"Lseek failed on temporary draft file %s\n", tmpdrffile);
    			s_exit(-1);
    		}
		if(lseek(drffd, 0L, 0) < 0)
		{
		 	fprintf(stderr,"Lseek failed on draft %s\n", drffile);
		 	s_exit(-1);
		}

		/* Add leading blank line back, on output file. */
		write( drffd, "\n", 1 );
    		dist = 1L;

		/* Copy temp. draft (no headers) back into draft file */
    		while((i = read(tmpfd, bigbuf, BBSIZE-1)) > 0)
    			dist += write(drffd, bigbuf, i);

    		if(truncate( drffile, dist ) < 0)
    			fprintf(stderr,"Could not truncate draft %s\n", drffile);
    		if(truncate( tmpdrffile, 0 ) < 0)
    			fprintf(stderr,"Could not truncate temporary draft\n");
    		if(close( tmpfd ) < 0)
    			fprintf(stderr,"Could not close temporary draft\n");
    		if(lseek(drffd, 0L, 0) < 0)
    			fprintf(stderr,"Lseek failed on draft %s\n", drffile);
    		if(chmod(drffile, 0400) < 0)
    			fprintf(stderr,"Could not protect draft %s\n", drffile);
		printf("\n");
		continue;
	}

/*	help	*/

	if (prefix ("?", bigbuf))
	{
	    printf ("bcc\n");
	    printf ("bye\n");
	    printf ("check spelling\n");
	    printf ("edit body\n");
	    printf ("file include\n");
	    printf ("program run\n");
	    printf ("quit\n");
	    printf ("review message\n");
	    printf ("send message\n");
	    printf ("set [option] [option value]\n");
	    printf ("sign message with digital signature\n");	/* MJM */
	    printf ("encrypt message body\n");			/* MJM */
	    continue;
	}

	printf (" unknown command (type ? for help)\n");
    }				  /* end of loop */
}

do_review (name, data)            /* send out a header field            */
    char name[],                  /* name of field                      */
	*data;                    /* value-part of field                */
{
    register int ind;
    register char *curptr;

    for (curptr = data, ind = 0; ind >= 0; curptr += ind + 1)
    {
	if ((ind = strindex ("\n", curptr)) >= 0)
	    curptr[ind] = '\0';   /* format lines properly              */

	printf (hdrfmt, (curptr == data) ? name : "", curptr);
	if (ind >= 0)
	    curptr[ind] = '\n';   /* put it back                        */
    }
}

doalias( bufptr )
char *bufptr;
{
	char tbuf[512], *bptr;

	tbuf[0] = '\0';
	bptr = tbuf;
	aliasmap( bptr, bufptr, host );
	strcpy( bufptr, bptr );
}

/*
 *			G E T H D R
 *
 * This routine seeks to the beginning of the message in the disk file,
 * reads in the header, and scans it to find the values of the various
 * fields.  Fields which the caller is not interested are passed as
 * NULLs.
 *
 */
gethdr( fromstr, tostr, ccstr, bccstr, subjstr )
char *fromstr;		/* where to save From     field */
char *tostr;		/* where to save To       field */
char *ccstr;		/* where to save cc       field */
char *bccstr;		/* where to save BCC	  field */
char *subjstr;		/* where to save Subj     field */
{
	char line[LINESIZE];
	char curhdr[S_BSIZE];
	int cnt = 0;

	if( fromstr != 0)
		fromstr[0] = '\0';
	if( tostr != 0)
		tostr[0] = '\0';
	if( ccstr != 0)
		ccstr[0] = '\0';
	if( bccstr != 0)
		bccstr[0] = '\0';
	if( subjstr != 0)
		subjstr[0] = '\0';
	
	dropen( DRBEGIN );
	curhdr[0] = '0';
	while((cnt += gethdrline( line )) > 0 &&
		line[0] != '\0' &&
		strcmp( delim1, line) != 0 &&
		strcmp( delim2, line) != 0 )  
	{
		if( fromstr != 0 && isnull( fromstr[0]))
			if( gothdr( curhdr, "From:", line, fromstr))
				continue;

		if( tostr != 0)
			if( gothdr( curhdr, "To:", line, tostr))
				continue;

		if( ccstr != 0)
			if( gothdr( curhdr, "cc:", line, ccstr))
				continue;

	 	if( bccstr != 0)
	 		if( gothdr( curhdr, "BCC:", line, bccstr))
	 			continue;
	 			
		if( subjstr != 0)
			if( gothdr( curhdr, "Subject:", line, subjstr))
				continue;

		/*
		 * If we got here, the line was of a type that we don't
		 * care about.  Zap curhdr, as it will not have been updated
		 * unless the line was processed by gothdr(), and we want
		 * to avoid processing extraneous continuations.
		 */
		 strcpy( curhdr, "boring-header-line:" );
	}
	if(*bccstr != '\0')
		bccflag = 1;
	return( cnt );
}

/*
 *			G O T H D R
 */
gothdr( curname, name, src, dest)
char *curname;		/* current field name */
char *name;		/* test field name */
char *src;		/* test input line */
char *dest;		/* where to put data part at end */
{
	unsigned int destlen;
	char *ptr;

	if( isspace( *src ) )
	{
		/* This is a continuation line.  Check "remembered" field */
		if( strcmp( curname, name ) != 0 )
			return( FALSE );

		/* Continuation of correct header line.  Retain "remembered"
		/* field.  Get rid of name & extra space from input line. */
/*		compress( src, src);	*/

		/*** FALL OUT ***/
	}
	else if (prefix( src, name ))
	{
		/* New line and field name matches, lets go! */

		/* Remember the "current field" */
		strcpy( curname, name);

		/* get rid of name & extra space */
		compress( &src[strlen( name )], src);

		/*** FALL OUT ***/
	}
	else
	{
		/* New line but field does not match, skip it */
		return(FALSE);
	}

	/* Common code for TRUE replies */

	if( isnull( dest[0]))  {
		/* first entry */
		strcpy( dest, src);
	}  else  {
		/* add to end of field, if room */
		destlen = strlen( dest);
		if( (strlen( src) + destlen) < S_BSIZE - 2)
			sprintf( &dest[destlen], "\n %s%c", src, '\0');
		else
		{
			ptr = &src[S_BSIZE - 2];
			while(*ptr != ' ' && *ptr != ',')
				ptr--;
			*ptr = '\0';
			sprintf( &dest[destlen], "\n %s%c",src, '\0');
			fprintf(stderr,"WARNING: Header line exceeds buffer size!\n");
			fprintf(stderr,"\tIt was truncated to legal length.\n");
		}
	}
	return( TRUE);
}
gethdrline( s )
char *s;
{
	int c, i;
	
	i = 0;
	while( read( drffd, s, 1 ) > 0 && *s != '\n' )
	{
		s++;
		i++;
	}
	*s = '\0';
	return( i+1 );
}

#ifndef HAVE_TRUNCATE

#define BUFSIZE		1024

static char tmp_file[20];

truncate(path, length)
char *path;
long length;
{
	char tmpf[sizeof tmp_file];
	register fd1, fd2, n;
	char buf[BUFSIZE];
    strcpy(tmp_file, "/tmp/truncXXXXXX");

	if ((fd1 = open(path, 0)) < 0)
		return -1;
	strncpy(tmpf, sizeof(tmpf), tmp_file);
	mktemp(tmpf);
	if ((fd2 = creat(tmpf, 0600)) < 0) {
		close(fd1);
		return -1;
	}
	close(fd2);
	if ((fd2 = open(tmpf, 2)) < 0) {
		close(fd1);
		unlink(tmpf);
		return -1;
	}
	while (length > 0) {
		n = length > BUFSIZE ? BUFSIZE : length;
		if ((n = read(fd1, buf, n)) <= 0)
			break;
		write(fd2, buf, n);
		length -= n;
	}
	close(fd1);
	if ((fd1 = creat(path, 0666)) < 0) {
		close(fd2);
		unlink(tmpf);
		return -1;
	}
	lseek(fd2, 0L, 0);
	while ((n = read(fd2, buf, BUFSIZE)) > 0)
		write(fd1, buf, n);
	close(fd1);
	close(fd2);
	unlink(tmpf);
	return 0;
}

#endif /* HAVE_TRUNCATE */


/*
 *			R U N _ F I L T E R
 *
 *  Run the given string as a filter, with given stdin and stdout.
 *  Append recipient names in canonical format, if desired.
 *  The string may have embedded spaces separating arguments.
 *
 *  Returns -
 *	 0	Success
 *	-1	Failure, output file is not useful.
 *
 *  Written by Mike Muuss, ARL, 20-July-94.
 *  Public Domain, Distribution Unliminted.
 */
int
run_filter( prog, in, out, names )
char	*prog;		/* Command string, perhaps with spaces */
int	in;		/* file descriptor */
int	out;		/* file descriptor */
int	names;		/* non-zero for listing recipient names */
{
	int     (*old1) ();
	int	(*old2) ();
	int	(*old3) ();
	int	pid;
	int	stat;
	int	i;
	int	first_i;
	int	j;

        old1 =  signal (SIGHUP, SIG_IGN); /* ignore signals intended for edit */
        old2 =  signal (SIGINT, SIG_IGN);
        old3 =  signal (SIGQUIT, SIG_IGN);

	if( (pid = fork()) == -1 )  {
		perror("run_filter fork()");
		return -1;
	}
	if( pid == 0 )  {
		char	*argv[1024];
		char	*argv2[512];
        	char	*cp;

		/* Space separated options become separate args. */
        	i = 0;
        	cp = prog;
		for(;;)  {
			argv[i++] = cp;
        		if( (cp = strchr( cp, ' ' )) == (char *)NULL )  break;
        		*cp++ = '\0';
        	}
		first_i = i;
		if( names )  {
			j = 0;
			/* Only include FROM address if a file copy is being made */
			if( qflag || cflag )
				j = chop_addr( j, argv2, from );
			j = chop_addr( j, argv2, to );
			j = chop_addr( j, argv2, cc );
			/* Including BCC list to encryption might disclose their presence to explicit recipients */
			if( bccflag )
				fprintf(stderr,"WARNING: bcc recipients will not be able to decrypt this message\n");

			/* Now merge them into final argv[] with -r */
			for( pid=0; pid < j; pid++ )  {
				argv[i++] = "-r";
				argv[i++] = argv2[pid];
			}
        	}
		argv[i] = (char *)NULL;

		for(pid=0; pid < i; pid++)
			fprintf(stderr, "%s ", argv[pid]);
		fprintf(stderr, "\n");

		dup2( in, 0 );			/* new stdout */
		dup2( out, 1 );			/* new stdout */

        	/* Leave stderr alone, close the most likely fd's */
        	for( i=3; i<20; i++ )  close(i);

		signal (SIGHUP, old1);
		signal (SIGINT, old2);
		signal (SIGQUIT, old3);

		execvp( argv[0], argv );
        	/* Returns only on error */

        	perror(prog);
		fprintf (stderr,"can't execute\n");
		s_exit (-1);
        }
	stat = 0;
	if( wait(&stat) == -1 || stat != 0 )  {
		fprintf (stderr,"ERROR: '%s' failed, exit code %d (0x%x)\n", prog, stat, stat);
		return -1;
	}

        body = TRUE;	/* Don't know but assume he created a draft */
        lastsend = 0;

        signal (SIGHUP, old1);     /* restore signals */
        signal (SIGINT, old2);
        signal (SIGQUIT, old3);

	return 0;	/* OK */
}

/*
 *			C H O P _ A D D R
 *
 *  Given a comma separated list of addresses, chop them apart.
 *  Each address is added to the provided argv[] array, starting
 *  at location 'argc'.
 *
 *  Addresses are stored in "canonical form", <user@host> with no
 *  leading or trailing decorations.
 *  This form is used to prevent accidental partial matches in the
 *  digital signature program's "keyring" file, which might otherwise
 *  happen with, e.g., "smith@arl.mil" and "tsmith@arl.mil".
 *  The strings "<smith@arl.mil>" and "<tsmith@arl.mil>" will match uniquely.
 *
 *  Returns updated argc.
 *
 *  It is the caller's responsibility to free the dynamic strings.
 *  In practice, the caller is usually a child process, and just
 *  dumps the whole address space with an exec().
 *
 *  Written by Mike Muuss, ARL, 20-July-94.
 *  Public Domain, Distribution Unliminted.
 */
int
chop_addr( argc, argv, str )
int	argc;
char	**argv;
char	*str;
{
	/* adrptr is global external */
	char		buf[ADDRSIZE];
	char		*cp;
	char		*bp;

	for( adrptr = str;; )  {
		switch( next_address(buf) )  {
		case -1:
			/* All addresses have been processed */
			return argc;
		case 0:
			/* Empty address component */
			continue;
		default:
			/* Now, convert to canonical form */
			/* Just the part inside the angle brackets */
			if( (cp = strchr( buf, '>' )) )  {
				cp[1] = '\0';
			}
			if( (cp = strchr( buf, '<' )) )  {
				argv[argc++] = strdup(cp);
				break;
			}
			/* Address had no angle brackets, add them */
			if( !(cp = malloc( strlen(buf)+3 )) )  {
				perror("chop_addr() malloc");
				return argc;
			}
			*cp = '<';
			/* Strip off any leading spaces in buf */
			bp = buf;
			while( isascii(*bp) && isspace(*bp) )  bp++;
			strcpy( cp+1, bp );
			bp = cp + strlen(cp)-1;
			while( isascii(*bp) && isspace(*bp) )  bp--;
			bp[1] = '>';
			bp[2] = '\0';

			argv[argc++] = cp;
			break;
		}
	}
	/* NOTREACHED */
}
