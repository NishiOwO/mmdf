# include  "util.h"
#if defined(HAVE_SGTTY_H)
# include  <sgtty.h>
#else
# include <termio.h>
# include <fcntl.h>
#endif /* HAVE_SGTTY_H */
# include  <sys/stat.h>
# include  "d_proto.h"
# include  "d_returns.h"
# include  "d_structs.h"

/*
 *  Routines for controlling the tty settings on the control line
 *
 *  Jun 81  D. Crocker      Rewritten to work on v6 & v7
 *  Jul 81  D. Crocker      Make explicit TIOCHPCL call
 *  Dec 82  Doug Kinston    Chmods no longer control return values (DPK@BRL)
 *  Jan 84  D. Rockwell     Make d_ttrestore heed d_savalid
 *                          Save d_savfd instead of FILE *
 */

extern int errno;

LOCVAR int d_savfd;            /*  handle on the opened tty          */
LOCVAR char d_savfname[25];      /*  name of tty                       */
#if defined(HAVE_SGTTY_H)
LOCVAR struct sgttyb d_savtty;   /*  where to stash original setting   */
#else /* HAVE_SGTTY_H */
LOCVAR struct termio d_savtty;
#endif /* HAVE_SGTTY_H */
LOCVAR int d_savmode;            /*  protections on tty                */
LOCVAR int d_savalid;


d_ttsave (fp, fname)              /* save the current setting           */
	FILE *fp;
	char fname[];
{
    struct stat statbuf;
    register int retval;

    d_savfd = fileno(fp);
    if (fname != 0)
	(void) strcpy (d_savfname, fname);
    else
	d_savfname[0] = '\0';

#ifdef D_DBGLOG
    d_dbglog("d_ttsave", "saving terminal modes, fd = %d", d_savfd);
#endif D_DBGLOG

#if defined(HAVE_SGTTY_H)
    if ((retval = ioctl (d_savfd, TIOCGETP, &d_savtty)) >= 0)
#else
    if ((retval = ioctl (d_savfd, TCGETA, &d_savtty)) >= 0)
#endif HAVE_SGTTY_H
    {
	if ((retval = fstat (d_savfd, &statbuf)) >= 0)
	{
	    d_savalid++;
	    d_savmode = statbuf.st_mode & ~S_IFMT;
	}
    }
    return (retval);
}


d_ttrestore ()                    /*  restore initial setting           */
{
    int retval;

#ifdef D_DBGLOG
    d_dbglog ("d_ttrestore", "restoring terminal characteristics, fd = %d",
	d_savfd);
#endif D_DBGLOG

    if (d_savalid == 0) {
#ifdef D_DBGLOG
	d_dbglog("d_ttrestore", "didn't have to");
#endif D_DBGLOG
    	return(0);
    }
#if defined(HAVE_SGTTY_H)
    retval = ioctl (d_savfd, TIOCSETP, &d_savtty);
#else
    retval = ioctl (d_savfd, TCSETAW, &d_savtty);
#endif HAVE_SGTTY_H
    if (!isnull (d_savfname[0]))
	chmod( d_savfname, d_savmode );

    d_savalid = 0;
    return (retval);
}

d_ttscript (baudrate)             /* tty will be following call script  */
    int baudrate;
{
#if defined(HAVE_SGTTY_H)
    struct sgttyb sttybuf;
    extern int d_scon, d_scoff;
#else
    struct termio sttybuf;
    extern unsigned short d_scbitc, d_scbiti, d_scbito, d_scbitl;
    int rc;
#endif HAVE_SGTTY_H

    if (!isnull (d_savfname[0]))
	chmod( d_savfname, 0600 );

#if !defined(HAVE_SGTTY_H)
    ioctl(d_savfd, TCGETA, &sttybuf);
#endif HAVE_SGTTY_H

    if (baudrate != 0)
    {                             /*  use special speed                 */
#if defined(HAVE_SGTTY_H)
	sttybuf.sg_ispeed = baudrate;
	sttybuf.sg_ospeed = baudrate;
#else
	sttybuf.c_cflag = (baudrate & CBAUD);
#endif HAVE_SGTTY_H
    }
    else
    {                             /*  use current speed                 */
#if defined(HAVE_SGTTY_H)
	sttybuf.sg_ispeed = d_savtty.sg_ispeed;
	sttybuf.sg_ospeed = d_savtty.sg_ospeed;
#else
	sttybuf.c_cflag = (d_savtty.c_cflag & CBAUD);
#endif HAVE_SGTTY_H
    }

#if defined(HAVE_SGTTY_H)
    sttybuf.sg_erase  = '\010';   /* erase = control-H                  */
    sttybuf.sg_kill   = '\030';   /* kill = control-X                   */
    sttybuf.sg_flags  |=  d_scon;
				  /* force on  special bits for script  */
    sttybuf.sg_flags  &=  ~((int) d_scoff);
				  /* force off special bits for script  */
#else
    sttybuf.c_cc[VERASE] = '\010';	/* erase control-h */
    sttybuf.c_cc[VKILL] = '\030';	/* kill control-x */
    sttybuf.c_cc[VTIME] = 0;		/* delay */
    sttybuf.c_cc[VMIN] = 1;		/* hi water mark */

    sttybuf.c_cflag |= d_scbitc;
    sttybuf.c_iflag = d_scbiti;
    sttybuf.c_oflag = d_scbito;
    sttybuf.c_lflag = d_scbitl;
#endif HAVE_SGTTY_H

#if defined(HAVE_SGTTY_H)
    return (ioctl (d_savfd, TIOCSETP, &sttybuf));
#else
    rc = ioctl (d_savfd, TCSETAW, &sttybuf);
    fcntl (d_savfd, F_SETFL, fcntl (d_savfd, F_GETFL, 0) & ~O_NDELAY);
    return (rc);
#endif HAVE_SGTTY_H
}


d_ttproto (baudrate)
    int baudrate;
{
#if defined(HAVE_SGTTY_H)
    struct sgttyb sttybuf;
    extern int d_pron, d_proff;
#else
    struct termio sttybuf;
    extern unsigned short d_prbitc, d_prbiti, d_prbito, d_prbitl;
    int rc;
#endif HAVE_SGTTY_H

    if (!isnull (d_savfname[0]))
	chmod( d_savfname, 0600 );

#if !defined(HAVE_SGTTY_H)
    ioctl (d_savfd, TCGETA, &sttybuf);
#endif HAVE_SGTTY_H

    if (baudrate != 0)
    {                             /*  use special speed                 */
#if defined(HAVE_SGTTY_H)
	sttybuf.sg_ispeed = baudrate;
	sttybuf.sg_ospeed = baudrate;
#else
	sttybuf.c_cflag = (baudrate & CBAUD);
#endif HAVE_SGTTY_H
    }
    else
    {                             /*  use current speed                 */
#if defined(HAVE_SGTTY_H)
	sttybuf.sg_ispeed = d_savtty.sg_ispeed;
	sttybuf.sg_ospeed = d_savtty.sg_ospeed;
#else
	sttybuf.c_cflag = (d_savtty.c_cflag & CBAUD);
#endif HAVE_SGTTY_H
    }

#if defined(HAVE_SGTTY_H)
    sttybuf.sg_erase  = '\010';   /* erase = control-H                  */
    sttybuf.sg_kill   = '\030';   /* kill = control-X                   */
    sttybuf.sg_flags  |= d_pron;  
				  /* force on  special bits for protocol*/
    sttybuf.sg_flags  &=  ~((int) d_proff);
				  /* force off special bits for protocol*/
#else
    sttybuf.c_cc[VERASE] = '\010';	/* erase control-h */
    sttybuf.c_cc[VKILL] = '\030';	/* kill control-x */
    sttybuf.c_cc[VEOL] = '\n';
    sttybuf.c_cc[VEOF] = '\004';

    sttybuf.c_cflag |= d_prbitc;
    sttybuf.c_iflag = d_prbiti;
    sttybuf.c_oflag = d_prbito;
    sttybuf.c_lflag = d_prbitl;
#endif HAVE_SGTTY_H

#if defined(HAVE_SGTTY_H)
    return( ioctl (d_savfd, TIOCSETP, &sttybuf) );
#else
    rc = ioctl (d_savfd, TCSETAW, &sttybuf);
    fcntl (d_savfd, F_SETFL, fcntl (d_savfd, F_GETFL, 0) & ~O_NDELAY);
    return(rc);
#endif HAVE_SGTTY_H
}
