/*
 *	M S G T A I L O R . C
 */

#define NOEXTERNS
#include "./msg.h"

#ifdef pdp11
#	define NMSGS	500
#else
#	define NMSGS	200
#endif pdp11

char    *savmsgfn = "mbox";
char	*resendprog = "|/usr/local/resend ";
char	*sndname = "/usr/local/send"; /* Overridden by .msgrc sendprog */

char	*dflshell = "sh";	/* Overridden by getenv("SHELL"); */
char	*dfleditor = "vi";	/* Overridden by getenv("EDITOR"); */

int	pagesize = 22;		/* default line count */
int	linelength = 80;	/* default line size */

int	paging = OFF;		/* Internal paging of long messages */
int	verbose = ON;		/* Command completion (raw mode) */
int	wmsgflag = OFF;		/* Save messages when calling send */
int	keystrip = ON;		/* Strip header lines on display */
int	bdots = ON;		/* Binary box dots */
int	mdots = ON;		/* Mailbox dots */
int	bprint = ON;		/* Reading... messages */
int	quicknflag = OFF;	/* Turn off NEW before message prints */
int	quickexit = ON;		/* Don't stay in msg if mailbox is empty */
int	prettylist = ON;	/* Print message numbers in listings */
int	filoutflag = ON;	/* Filter text (display controls as ^X) */
int	binsavflag = ON;	/* Save binary box on exec's & pauses */
int	doctrlel = OFF;		/* Page on control-L */

/***** Do not tailor below this line *****/
int	Nmsgs = NMSGS;
struct message *msgp[NMSGS];
struct message *mptr;