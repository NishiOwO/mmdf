/*
**		S _ E X T E R N S
**
**  This file contains the external declarations of global variables
**  for the SEND program.
**
**
**	R E V I S I O N  H I S T O R Y
**
**	03/31/83  GWH	Split the SEND program into component parts
**
*/

extern int sentprotect;
extern char hdrfmt[], toname[], BCtoname[], ccname[], BCccname[];
extern char subjname[], fromname[], datename[], bccname[], shrtfmt[];

extern jmp_buf savej;

extern int drffd, tmpfd, nsent, badflg;

extern char *adrptr;

extern char bigbuf[], signature[], host[], from[], to[], bcc[], cc[];
extern char subject[], stdobuffer[], inclfile[], drffile[], sentfile[], tmpdrffile[];

extern char body, lastsend, aborted, inrdwr, toflag, ccflag, subjflag;
extern char *verdate;
extern int cflag, wflag;
extern int rflag;
extern int qflag, pflag;
extern char copyfile[], editor[], checker[];
extern char subargs[];
extern char replyto[];
extern char aliasfilename[];
extern struct header *headers;

extern RETSIGTYPE (*orig) ();

extern int errno;
extern char *compress ();

/* mmdf globals */

extern char *locname, *locdomain;
extern char *delim1;
extern char *delim2;

/* end of mmdf globals */
