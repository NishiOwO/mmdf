# include  "util.h"
#if defined(HAVE_SGTTY_H)
#  include  <sgtty.h>
#else HAVE_SGTTY_H
#  include <termio.h>
#endif HAVE_SGTTY_H
# include  "d_proto.h"
# include  <stdio.h>
# include  "d_structs.h"



struct dialports  *d_prtpt;    /*  pointer to currently user port in  */
				/*  'd_prts'                          */

struct directlines  *d_ptline;  /*  pointer to currently used line in  */
                                /*  'd_lines'                          */
FILE    *d_prtfp = (FILE *) EOF; /*  port stdio file pointer  */

int  d_lckfd = -1,             /*  lock file descriptor  */
     d_baudrate,                /*  index of baud rate for this connection  */
     d_didial,                  /*  set to non-zero if this is a dial-up  */
                                /*  connection.                           */
     d_errno;                   /*  error code  */

char  d_uname[10],           /*  login name of user  */
      d_upath[50];           /*  login directory path name of user  */


/*
 *     this structure defines the baud rate strings that will be accpeted
 *     in phone number specifications.
 */

struct speedtab  d_spdtab[] =
  {
#ifdef B0
  "0",       B0,
#endif B0
#ifdef B50
  "50",      B50,
#endif B50
#ifdef B75
  "75",      B75,
#endif B75
#ifdef B110
  "110",     B110,
#endif B110
#ifdef B134
  "134.5",   B134,
  "134",     B134,
#endif B134
#ifdef B150
  "150",     B150,
#endif B150
#ifdef B200
  "200",     B200,
#endif B200
#ifdef B300
  "300",     B300,
#endif B300
#ifdef B600
  "600",     B600,
#endif B600
#ifdef B1200
  "1200",    B1200,
#endif B1200
#ifdef B1800
  "1800",    B1800,
#endif B1800
#ifdef B2400
  "2400",    B2400,
#endif B2400
#ifdef B4800
  "4800",    B4800,
#endif B4800
#ifdef B7200
  "7200",    B7200,
#endif B7200
#ifdef B9600
  "9600",    B9600,
#endif B9600
#ifdef B19200
  "19200",   B19200,
#else
#ifdef EXTA
  "19200",   EXTA,
#endif EXTA
#endif B19200
#ifdef B38400
  "38400",   EXTB,
#else
#ifdef EXTB
  "38400",   EXTB,
#endif EXTB
#endif B38400
  0,         0,
  };




/*
 *     array of ascii baud rate names indexed by the speed.  used to print
 *     understandable diagnostics.
 */

int d_xretry = NSENDTRY;     /*  Init # of retries to make  */
int d_toack = DACKWAIT;      /*  Init time out time for ack */
int d_todata = DATAWAIT;     /*  Init time out for data  */

#ifndef HAVE_SGTTY_H
unsigned short d_prbitc = PORTPONC;	/* terminal protocol bit on */
unsigned short d_prbiti = PORTPONI;
unsigned short d_prbito = PORTPONO;
unsigned short d_prbitl = PORTPONL;

unsigned short d_scbitc = PORTSONC;	/* terminal script bits on */
unsigned short d_scbiti = PORTSONI;
unsigned short d_scbito = PORTSONO;
unsigned short d_scbitl = PORTSONL;

#else

int d_pron = PORTPON;       /*  terminal protocol bit on  */
int d_proff = PORTPOFF;     /*  terminal protocal bit off */
int d_scon = PORTSON;       /*  terminal script bits on   */
int d_scoff = PORTSOFF;     /*  terminal script bits off  */

#endif SYS5

int d_nbuff = 1;	    /*  The number of messages to send out before
			     *  requiring an ACK.  Initially, get an ACK
			     *  immediately.
			     */
int d_wpack;		    /*  Non-zero if the window specification
			     *  packet should be sent.
			     */
