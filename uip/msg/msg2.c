/*
 *			M S G 2 . C
 *
 * This is the second part of the MSG program.
 *
 * Functions -
 *	rdnxtfld	read next field from input
 *	prhdr		print header on stdout
 *	hdrfile		print header on outfp
 *	hdrout		print header on specified FILE p
 *	gomsg		goto a specific message
 *	delmsg		delete specified message
 *	undelmsg	un-delete specified message
 *	keepmsg		mark specified message for keeping
 *	getfn		
 *	cpyiter
 *	cppipe
 *	cpopen
 *	dolstmsg	list the selected messages, maybe separated
 *	lstmsg		list one message, maybe separated
 *	lstbdy		list body of one message
 *	movmsg		"move" a message
 *	putmsg		"put" a message
 *	writmsg		write specified message onto given FD
 *	writbdy		write body of specified message onto given FD
 *	prmsg		print specified message onto terminal
 *	ansiter		top-level iteration for "answer"
 *	ansqry		enquire who to send answer to
 *	ansmsg		create header for answer
 *	fwditer		top-level iteration for "forward"
 *	fwdmsg		copy one forwarded message, with markers
 *	fwdpost		add trailer to forwarded message
 *	srchkey		Keyword search for stripping obnoxious header lines.
 *	filout		Filters control characters before writing on terminal.
 *	makedrft	Make a file containing one or more messages
 *
 *		R E V I S I O N   H I S T O R Y
 *
 *	06/08/82  MJM	Split the enormous MSG program into pieces.
 *
 *	09/10/82  MJM	Modified to use STDIO for output to files, too.
 *
 *	11/14/82  HW	Added keyword filter to ignore Via, Remailed, etc.
 *
 *	08/27/98  MJM	Eliminated gets(), re-added message percentage print.
 */
#include "util.h"
#include "mmdf.h"
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef HAVE_SGTTY_H
#  include <sgtty.h>
#endif /* HAVE_SGTTY_H */
#include "./msg.h"

extern FILE *popen();
static int	curline;
static int	pcntdn;		/* percent done */
static int	dobpage = TRUE;	/* do "page back" */
static long	msgsize;

/*
 *			R D N X T F L D
 *
 * read next field from input
 */
rdnxtfld()
{
	register char   c;

	do  {
	    c = ttychar();
	}  while( (isspace( c) || c == ch_erase) && c != '\n' );

	return(c);
}

/*
 *			P R H D R
 */
prhdr()
{
	hdrout( stdout, TRUE);
}

/*
 *			H D R F I L E
 */
hdrfile()
{
	hdrout( outfp, FALSE);
}




hdrout( fp,crflag)
FILE *fp;
int crflag;
{
	char lbuf[LINESIZE];

	sprintf( lbuf, "%4d%c%c%c%c%c%c%5ld: %-9.9s %-15.15s %.30s%s\n",
		msgno,
		mptr->flags & MSG_NEW ? 'N' : ' ',
		mptr->flags & MSG_DELETED ? 'D' : ' ',
		mptr->flags & MSG_KEEP ? 'K' : ' ',
		mptr->flags & MSG_ANSWERED ? 'A' : ' ',
		mptr->flags & MSG_FORWARDED ? 'F' : ' ',
		mptr->flags & MSG_PUT ? 'P' : ' ',
		mptr->len,
		mptr->datestr,
		mptr->from,
		mptr->subject,
		crflag ? "\r" : ""
	);
	dobpage = FALSE;	/* filout will ignore back page command */
	pcntdn = -1;		/* filout will not print % */
	msgsize = 1;
	filout( lbuf, fp );
	dobpage = TRUE;
}

/*--------------------------------------------------------------------*/

gomsg()
{
	status.ms_curmsg = msgno;		  /* set the current message number     */
}

delmsg()
{
	mptr->flags |= MSG_DELETED;
	mptr->flags &= ~MSG_KEEP;
}

undelmsg()
{
	mptr->flags &= ~(MSG_DELETED|MSG_KEEP);
}

keepmsg()
{
	mptr->flags |= MSG_KEEP;
	mptr->flags &= ~MSG_DELETED;
}

/*
 *			G E T F N
 *
 * Get a file name for the user.
 */
getfn( s, f, def)
char    *s;		/* Prompt string to provoke user */
char	*f;		/* place to put resulting answer */
char	*def;		/* optional default */
{
	char    tmpbuf[LINESIZE];
	char	name[LINESIZE];
	struct passwd *getpwuid(), *getpwnam();
	register char *t, *n;
	register struct passwd *pw;

	tt_norm();
	fputs( s, stdout);
	fflush( stdout);

	strcpy( oldfile, f);

	if( fgets( tmpbuf, sizeof(tmpbuf), stdin) == NULL || strlen(tmpbuf) <= 1 )  {
		if( def == (char *)0)
			error( "no filename specified\r\n");
		else  {
			strcpy( f, def);
			printf( "%s...\r\n", f);
		}
	}  else  {
		/* Remove trailing \n */
		char	*endp = strchr(tmpbuf, '\n');
		if( endp )  *endp = '\0';

		/* Remove leading whitespace */
		for( t = tmpbuf; isspace(*t); t++ )
			;
		if( *t == '~' ) {
			/* Expand */
			for( n = name; *++t && *t != '/'; *n++ = *t )
				;
			*n = '\0';
			if( name[0] == '\0' )
				pw = getpwuid(getuid());
			else
				pw = getpwnam(name);
			if( pw == NULL )
				error("~name not found\r\n");
			(void) strcpy(f, pw->pw_dir);
			(void) strcat(f, t);
		} else
			strcpy(f, t);
	}

	nxtchar = '\n';		  /* note the last character typed      */
	tt_raw();
}

/*
 *			C P Y I T E R
 */
cpyiter( fn, iterfl, post)
int    ( *fn)();		/* per-message function               */
int	iterfl;			/* To iterate or not to iterate	      */
int    ( *post)();		/* post-process function              */
{
	if( outfile[0] == '|')		/* output filter, not file */
		cppipe();		/* get the output pipe */
	else
		cpopen();		/* get the output file */

	tt_norm();
	if( iterfl == DOIT )
		doiter( fn);
	else
		(*fn)();
	tt_norm();

	if(post != 0)
		(*post)();

	if(outfile[0] == '|') {	/* collect the child */
		pclose( outfp );	/* done sending to file/pipe */
		signal( SIGINT, onint );
	} else
		lk_fclose (outfp, outfile, NULL, NULL);

	outfd = -1;
	tt_raw();
}

/*
 *			C P P I P E
 */
cppipe()
{
	char	buf[LINESIZE];

	fflush(stdout);
	if ((outfp = popen(&outfile[1], "w")) == NULL)
		error( "problem starting pipe command\r\n");

	outfd = -1;
	return;
}

/*
 *			C P O P E N
 *
 * open a file to copy into
 */
cpopen()
{

	/* EXCLUSIVE open the file, as we are writing */
	if( (outfd = lk_open( outfile, 1, (char *)0, (char *)0, 5)) < 0)  {
		switch( errno)  {
		case ENOENT:
			if( !autoconfirm)  {
				printf( "Create '%s'", outfile);
				if( !confirm((char *)0,DOLF))
					error( "");
			}
			if( (outfd = creat( outfile, sentprotect)) < 0)  {
				printf( "can't create '%s'", outfile);
				error( "\r\n");
			}

			close( outfd);
			/* EXCLUSIVE open, since writing */
			if( (outfd = lk_open( outfile, 1, (char *)0, (char *)0, 5)) < 0)  {
				printf( "can't open '%s'", outfile);
				error( "\r\n");
			}
			break;

		case ETXTBSY:
			printf( "'%s' is busy; try later", outfile);
			error( "\r\n");

		default:
			perror(outfile);
			error( "\r");
		}
	}

	/*
	 * Set up for STDIO output using the exclusive
	 * write-only FD.  The fclose( outfd ) will flush
	 * the buffers.  The "a" append is necessary in
	 * the event that the file already exists.
	 */
	if( (outfp = fdopen( outfd, "a" )) == NULL ) {
		error( "can't fdopen outfile\n" );
		outfd = -1;
	}
}

/*
 *			D O L S T M S G
 *
 * list the selected messages, maybe separated
 */
dolstmsg()
{
	doiter ( lstmsg);
}

/*
 *			L S T M S G
 *
 * list one message, maybe separated
 */
lstmsg()
{
	if( lstsep && lstmore)
		fputs( "\f\n", outfp );
	lstmore = TRUE;

	if( prettylist) {
		fprintf(outfp, "(Message # %d: %ld bytes", msgno, mptr->len );
		if( mptr->flags & MSG_DELETED )	fprintf(outfp, ", Deleted");
		if( mptr->flags & MSG_PUT )	fprintf(outfp, ", Put");
		if( mptr->flags & MSG_NEW )	fprintf(outfp, ", New");
		if( mptr->flags & MSG_KEEP )	fprintf(outfp, ", KEEP");
		if( mptr->flags & MSG_ANSWERED )	fprintf(outfp, ", Answered");
		if( mptr->flags & MSG_FORWARDED )	fprintf(outfp, ", Forwarded");
		fprintf(outfp, ")\n");
	}
        
	writmsg();
	mptr->flags &= ~MSG_NEW;		/* Message seen */
}

/*
 *			L S T B D Y
 *
 * list one message body, maybe separated
 */
lstbdy()
{
	if( lstsep && lstmore)
		fputs( "\f\n", outfp );
	lstmore = TRUE;

	writbdy();
	mptr->flags &= ~MSG_NEW;		/* Message seen */
}

/*
 *			M O V M S G
 */
movmsg()
{
	putmsg();
	delmsg();
}

/*
 *			P U T M S G
 */
putmsg()
{
	register int len;

	len = strlen( delim1);
	fwrite( delim1, sizeof(char), len, outfp );
	writmsg();
	fwrite( delim2, sizeof(char), len, outfp );
	/* Check for running out of disk space, etc. */
	if( fflush( outfp ) != 0 )
		error("Error writing message to file\r\n");
	mptr->flags |= MSG_PUT;
}

/*
 *			W R I T M S G
 */
writmsg()
{
	long size;
	int count;
	char tmpbuf[32*1024];
	int len = strlen(delim1);

	/* Verify that the message delimeter is intact */
	fseek( filefp, (long)(mptr->start-len), 0);
	tmpbuf[0] = '\0';
	if( fread( tmpbuf, len, 1, filefp ) != 1 )
		error( "error reading message delimeter\r\n");
	tmpbuf[len] = '\0';
	if( strcmp( tmpbuf, delim1 ) != 0 )  {
		/* Build useful error message in tempbuf, then abort */
		sprintf(tmpbuf, "Mailbox delimeter corrupted at file offset=%ld.\n\
Message was From: %s\nSubject: %s\nDate: %s\n\
Recommend you exit MSG, remove the binary box, and restart.\n",
			mptr->start, mptr->from, mptr->subject, mptr->datestr);
		error( tmpbuf );
	}

	for( size = mptr->len; size > 0; size -= count)  {
		if( size <( sizeof tmpbuf))
			count = size;
		else
			count =( sizeof tmpbuf);

		if( fread( tmpbuf, sizeof( char), count, filefp) < count)
			error( "error reading\r\n");
		if( fwrite( tmpbuf, sizeof(char), count, outfp ) < count )
			error( "error writing to file\r\n");
	}
}

/*
 *			W R I T B D Y
 */
writbdy()
{
	long size;
	int srcstat;
	char line[LINESIZE];

	fseek( filefp,( long)( mptr->start), 0);
	size = mptr->len;

	srcstat = SP_HNOSP;
	while( size > 0)  {
		if( xfgets( line, sizeof( line), filefp) == NULL )
			break;
		size -= strlen( line );

		if( srcstat == SP_HNOSP && *line == '\n' ) {
			/* Don't output the separating blank line */
			srcstat = SP_BODY;
			continue;
		}

		if( srcstat == SP_BODY )
			fputs( line, outfp );

	}
}

/*
 *			P R M S G
 */
prmsg()
{
	char line[LINESIZE];
	int	srcstat;

	tt_norm();

	msgsize = mptr->len;
	status.ms_curmsg = msgno;

	printf( "(Message # %d: %ld bytes", msgno, msgsize );
	if( mptr->flags & MSG_DELETED )	printf(", Deleted");
	if( mptr->flags & MSG_PUT )	printf(", Put");
	if( mptr->flags & MSG_NEW )	printf(", New");
	if( mptr->flags & MSG_KEEP )	printf(", KEEP");
	if( mptr->flags & MSG_ANSWERED )	printf(", Answered");
	if( mptr->flags & MSG_FORWARDED )	printf(", Forwarded");
	printf(")\n");
	fflush( stdout );

	if(quicknflag == ON )
		mptr->flags &= ~MSG_NEW;		/* Message seen */

	fseek( filefp,( long)( mptr->start), 0);
	curline = 0;

	srcstat = SP_HNOSP;
	while( msgsize > 0)  {
		if( xfgets( line, sizeof( line), filefp) == NULL )
			break;

		msgsize -= strlen( line );
		pcntdn = 100-((100*msgsize)/mptr->len);
		curline++;
		if( *line == '\n' )
			srcstat = SP_BODY;
		/* filter obnoxious lines */
		if( keystrip == ON && srcstat != SP_BODY ) {
			if( !(srcstat == SP_HSP &&
			    (*line == '\t' || *line == ' ')))
				if( srchkey( line, keywds) == 0 ) {
					filout( line, stdout );
					srcstat = SP_HNOSP;
				}
				else
					srcstat = SP_HSP;
		}
		else
			filout( line, stdout);

	}
	tt_raw();
	mptr->flags &= ~MSG_NEW;		/* Message seen */
}

/*--------------------------------------------------------------------*/

/*
 *			A N S I T E R
 *
 * Top level for "answer" command.
 */
ansiter()
{
	sndto[0] =
	    sndcc[0] =
	    sndsubj[0] = '\0';
	ansnum = 0;

	doiter( prhdr);
	anstype = ansqry();

	doiter( ansmsg);

	if( ansnum == 0)
		printf( "no messages to answer\r\n");
	else
		if( isnull( sndto[0]))
			printf( "no From field in header\r\n");
		else {
			xeq( 'a');		/* execute an answer command */
			doiter( ansend );	/* Set the A flag */
		}
}

/*
 *			E D A N S I T E R
 *
 * Top level for "answer" command, to drop into EMACS 2-window mode.
 */
edansiter()
{
	register int fd;

	sndto[0] =
	    sndcc[0] =
	    sndsubj[0] = '\0';
	ansnum = 0;

	doiter( prhdr);
	anstype = ansqry();

	doiter( ansmsg);

	if( ansnum == 0)  {
		printf( "no messages to answer\r\n");
		return;
	}
	if( isnull( sndto[0]) )  {
		printf( "no From field in header\r\n");
		return;
	}

	/* Prepare the work files */
	if( (fd = creat(draft_work, sentprotect)) < 0 )  {
		perror( draft_work );
		return;
	}
	close(fd);

	makedrft();

	/* Invoke EDITOR in 2-window mode */
	xeq( 'e' );

	/* Feed results into SEND */
	xeq( 'A' );

	/* Set the A flag */
	doiter( ansend );

	/* Clean up */
	unlink( draft_original );
	/* draft_work left behind in case he wants another look at it */
}

/*
 *			A N S Q R Y
 * who to send answer to
 */
ansqry()
{

again:
	printf( "copies to which original addresses: ");
	nxtchar = echochar();
	nxtchar = uptolow( nxtchar);

	switch( nxtchar)  {

	case '\n':
	case '\r':
		if( verbose)
			printf( "from\r\n");
		return( ANSFROM);

	case 'a':
		if( verbose)
			printf( "ll\r\n");
		return( ANSALL);

	case 'c':
		if( verbose)
			printf( "c'd\r\n");
		return( ANSCC);

	case 'f':
		if( verbose)
			printf( "rom\r\n");
		return( ANSFROM);

	case 't':
		if( verbose)
			printf( "o\r\n");
		return( ANSTO);

	case '?':
		if( verbose)
			printf( "\r\n");
		printf( "all\r\n");
		printf( "cc'd\r\n");
		printf( "from [default]\r\n");
		printf( "to\r\n");
		goto again;

	case '\004':
	default:
		error( " ?\r\n");
	}
	/* NOTREACHED */
}


/*
 *			A N S M S G
 *
 * get create To, & Subject fields
 */
ansmsg()
{
	char tmpfrom[MSG_BSIZE],
	tmprply[MSG_BSIZE],
	tmpsender[MSG_BSIZE],
	tmpto[MSG_BSIZE],
	tmpcc[MSG_BSIZE],
	tmpsubj[MSG_BSIZE];
	register unsigned int ind;
	int llenleft;

	ansnum++;
	status.ms_curmsg = msgno;

	tmpfrom[0] =
	    tmprply[0] =
	    tmpsender[0] =
	    tmpto[0] =
	    tmpcc[0] =
	    tmpsubj[0] = '\0';

	gethead( NODATE, tmpfrom, tmpsender, tmprply,
	(anstype & ANSTO) ? tmpto : NOTO,
	(anstype & ANSCC) ? tmpcc : NOCC, tmpsubj);

	if( !isnull( tmprply[0]))
		/* send to Reply-To */
		sprintf( &sndto[strlen( sndto)], ",%s%c", tmprply, '\0');
	else if( !isnull( tmpfrom[0]))
		/* send to From, if no Reply-To */
		sprintf( &sndto[strlen( sndto)], ",%s%c", tmpfrom, '\0');

	if( !isnull( tmpto[0]))
		sprintf( &sndcc[strlen( sndcc)], ",%s%c", tmpto, '\0');

	if( !isnull( tmpcc[0]))
		sprintf( &sndcc[strlen( sndcc)], ",%s%c", tmpcc, '\0');

	if( !isnull( tmpsubj[0]))  {
		/* save the destination */
		if( equal( "re:", tmpsubj, 3))
			for( ind = 3; isspace( tmpsubj[ind]); ind++);
		else
			if( equal( "reply to:", tmpsubj, 9))
				for( ind = 9; isspace( tmpsubj[ind]); ind++);
			else
				ind = 0;

		if( ansnum  == 1)
			/* not the first message */
			sprintf( sndsubj, "Re:  %s%c", &tmpsubj[ind], '\0');
		else {
			/* append more addresses */
			if( (llenleft = sizeof(sndsubj) - strlen(sndsubj))
			    -strlen(&tmpsubj[ind])-9 > 0 )
				sprintf( &sndsubj[strlen( sndsubj)],
					"\n     %s%c", &tmpsubj[ind], '\0');
			else if( llenleft > 7 )
				sprintf( &sndsubj[strlen( sndsubj)],
				"\n...%c", '\0');
		}
	}
}
/*--------------------------------------------------------------------*/
ansend() {		/* Set the A flag */

	mptr->flags |= MSG_ANSWERED;
}
/*--------------------------------------------------------------------*/
/*
 *			F W D I T E R
 *
 * Top level for forward command.
 */
fwditer()
{
	char *mktemp();

	status.ms_curmsg = msgno;
	fwdnum = 0;
	sndto[0] =
	    sndcc[0] =
	    sndsubj[0] = '\0';

	doiter( prhdr);

	strcpy( outfile, "/tmp/send.XXXXXX");
	mktemp( outfile);

	autoconfirm = TRUE;
	cpyiter( fwdmsg, DOIT, fwdpost);
	autoconfirm = FALSE;

	xeq( 'f');
	unlink( outfile);
}

/*
 *			F W D M S G
 *
 * copy one forwarded message
 */
fwdmsg()
{
	char line[MSG_BSIZE];
	int	llenleft;
	
	fwdnum++;

	line[0] = '\0';		/* build a subject line */
	gethead( NODATE, NOFROM, NOSNDR, NORPLY, NOTO, NOCC, line);

	if( !isnull( line[0]))  {
		/* If we had a subject line, bracket the subject info */
		if( isnull( sndsubj[0]) )
			/* the first subject line */
			sprintf( sndsubj, "[%s:  %s]%c",
				mptr->from, line, '\0');
		else {
			/* not the first line */
			if( (llenleft = sizeof(sndsubj) - strlen(sndsubj))
			    -strlen(line)-SIZEFROM-10 > 0 )
				sprintf( &sndsubj[strlen( sndsubj)],
				"\n[%s:  %s]%c", mptr->from, line, '\0');
			else if( llenleft > 7 )
				sprintf( &sndsubj[strlen( sndsubj)],
				"\n...%c", '\0');

		}
	}

	fprintf( outfp, "\n----- Forwarded message # %d:\n\n", fwdnum);

	writmsg();

	mptr->flags |= MSG_FORWARDED;
}

/*
 *			F W D P O S T
 */
fwdpost()
{
	fprintf( outfp, "\n----- End of forwarded messages\n" );
}
/*
 *			S R C H K E Y
 */
srchkey( line, keypt)
	char *line, *keypt[];
{
	register int	n;
	register char	*pkey, *pline;

	for( n = 0; keypt[n] != 0; n++ ) {
		pkey = keypt[n];
		pline = line;

		while( *pkey != '\0' ) {

			if( *pline != *pkey && *pline - 'A' + 'a' != *pkey )
				goto trynext;
			pline++;
			pkey++;
		}
		return(1);
trynext: ;
	}
	return(0);
}
/* ------------------------------------------------------------------ 
 *			F I L O U T
 *
 *	Filter most control chars from text - Prevents letter bombs
 *	Also does paging
 *	****** linecount must be initialized before calling filout ******
 */
filout(line, ofp)
	char *line;
	FILE *ofp;
{
	register int cmd;
	char tmpbuf[40];

	if( paging ) {
		linecount += (strlen( line ) / linelength ) + 1;

		if( doctrlel && strindex( "\014", line) >= 0) {
			/* Got formfeed */
			tt_raw();
			if( !confirm("Formfeed. Continue?",NOLF))
				error("");
			tt_norm();
			linecount = 0;
		}

		if ( linecount >= pagesize - 2 && msgsize != 0 ) {
			tt_raw();
			if( pcntdn < 0 )
				sprintf(tmpbuf,"Continue?");
			else
				sprintf(tmpbuf,"Continue (%d%%)?",pcntdn);
			if( (cmd = confirm(tmpbuf,NOLF)) == FALSE )
				error("");
			tt_norm();
			switch( cmd ) {

			case '\n':		/* One more line */
				linecount = pagesize;
				break;
				
			case 'b':		/* Back one page */
				if( dobpage == TRUE ) {
					fprintf(stdout,"...Back 1 page...\n");
					skipln(curline-2*pagesize);
					linecount = 0;
					return;
				}
				/* else fall through */
				
			default:
				linecount = 0;
				break;
			}
		}
	}

	if( filoutflag == OFF ) {
		fputs( line, ofp );
		return;
	}
	
	for( ; *line != '\000'; line++ ) {
		if( *line >= ' ' && *line <= '~' )
			putc( *line, ofp );
		else {
			switch ( *line ) {

			case '\007':	/* Bel */
			case '\t':
			case '\n':
			case '\r':
			case '\b':
				putc( *line, ofp );
				break;

			default:
				putc( '^', ofp );
				putc( *line + '@', ofp );
				break;
			}
		}
	}
}
/*
 *  			M A K E D R F T
 *  Make a file containing one or more messages
 */
makedrft() {

	register int fd;

	if( (fd = creat(draft_original, sentprotect)) < 0 )  {
		perror( draft_original );
		return;
	}
	close( fd );

	/* LIST messages into the draft_original file */
	lstsep = TRUE;
	strcpy( outfile, draft_original );
	cpyiter( lstmsg, DOIT, (int(*)()) 0 );

}
/* Returns the number of the "current" message or does an
 *  error() return if no messages in box
 */
curno() {

	if( status.ms_curmsg == 0)  {
		if( status.ms_nmsgs != 0)
			status.ms_curmsg = 1;
		else
		if( status.ms_curmsg > status.ms_nmsgs)
			error( "no current message\r\n");
	}

	return(status.ms_curmsg);
}
/* Seek to the beginning of a message and then skip to the given line */
skipln(line)
int line;
{
	char linebuf[LINESIZE];

	fseek( filefp, mptr->start, 0);
	msgsize = mptr->len;
	curline = 0;

	if( line <= 0 )
		return;

	do {
		xfgets( linebuf, sizeof(linebuf), filefp);
		msgsize -= strlen(linebuf);
	} while( ++curline < line );
}
