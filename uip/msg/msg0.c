#include "util.h"
#include "mmdf.h"
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#ifdef HAVE_SGTTY_H
#  include <sgtty.h>
#endif /* HAVE_SGTTY_H */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif /* HAVE_SYS_IOCTL_H */
#include "./msg.h"

struct status status;

/* Stat buffers, used for comparing times */
struct stat statb1;			/* Reference buffer */
struct stat statb2;			/* Temporary buffer */

char	ch_erase;

int     nottty;
unsigned int    msgno;			/* message currently being processed */

/* user search key for from/subject/text msg sequences */
char    key[80];
char	*gc;

char    filename[FILENAMESIZE];
char	outfile[OUTFILESIZE];
char	defmbox[DEFMBOXSIZE];
char	oldfile[OLDFILESIZE];
char	*homedir;		/* Full path to home directory */

char    maininbox[MAININBOXSIZE];
char	binarybox[BINARYBOXSIZE];		/* Binary map file filename */
char	msgrcname[MSGRCNAMESIZE];		/* User options filename */
char    defoutfile[DEFOUTFILESIZE];
char    username[USERNAMESIZE];
char    ismainbox;		/* current file is receiving .mail  */
char    nxtchar;
char    lstsep;			/* separate messages by formfeed   */
char    lstmore;		/* second, or more listed message  */
char	autoconfirm;		/* bypass asking user confirmation */
char	anstype;		/* what addresses to send answers to  */
char	binaryvalid;		/* TRUE if incore binary box is valid */
RETSIGTYPE	(*orig) ();		/* DEL interrupt value on entry */

int     outfd;

int exclfd;		/* overwrit() exclusive use fd */
jmp_buf savej;

unsigned int ansnum;
unsigned int fwdnum;

FILE *fpmsgrc;			/* User options FILEp */

char    ttyobuf[BUFSIZ];
char    sndto[MSG_BSIZE];
char    sndcc[MSG_BSIZE];
char    sndsubj[MSG_BSIZE + 9];

int	istty;
extern int	wmsgflag;
int	linecount;
char	*ushell, *ueditor;
char	*nullstr;		/* A null string */

char	twowinfil[TWOWINFILSIZE];		/* filter for 2-window-answer mode */
char	draft_work[DRAFT_WORKSIZE];		/* Work file for 2-window answer */
char	draft_original[DRAFT_ORIGINALSIZE];	/* Original message(s) for 2-window ans */
char	draftorig[DRAFTORIGSIZE];		/* Message saving file */
