/*
**		S . H
**
**  
**	This file contains definitions required for the SEND program.
**
**	R E V I S I O N  H I S T O R Y
**
**	03/31/83  GWH	Split the SEND program into component parts
**
**	10/19/83  GWH	Added a counter to the ALIAS structure to prevent
**			disaster in local aliasfile loops.
**
*/

#include "util.h"
#include "mmdf.h"
#include "ch.h"
#include <pwd.h>
#include <signal.h>
#include "cnvtdate.h"
#include "./s_tailor.h"

#define DREND           2
#define DRBEGIN         0

#define HDRPRT          YES
#define HDRSIL          NO

#define FLDREMV         0
#define FLDSAME         1
#define FLDOVER         2
#define FLDADD          3
#define	ITMRMV		4

#define BBSIZE		513
#define	S_BSIZE		1024

/* Define somethings for use in "getuinfo" */

#define COPYFILE	0
#define DRAFTDIR	1
#define SIGNATURE	2
#define NOSIGNATURE	3
#define REPLYTO		4
#define ALIASES		5
#define SUBARGS		6
#define EDITOR		7
#define VEDITOR		8
#define CFILE		9
#define QFILE		10
#define NOFILE		11
#define PAGING		12
#define CHECKER		13
#define ADDHEADER	14
#define SIGN_CMD	15
#define ENCRYPT_CMD	16

/* Macro definition for use in s_get.c */

#define MAX(a,b) (a > b ? a : b )
struct RCOPTION {
	char *name;
	int idnum;
};
struct header {
	char	*hname;
	char	hdata[S_BSIZE];
	struct	header *hnext;
};

#define	NARGS	20	/* Maximum number of args to set and in .sendrc lines */
#define M_BSIZE 256

#ifdef DEFINE_STUFFS
/* Everything below was previously situated in s_main.c   */
/* Moved here when sprintf was changed to snprintf        */
/* and strcpy to strncpy                                  */
char    aliasfilename[128],
        bigbuf[BBSIZE],              /* buffer for text of msg */
        drffile[S_BSIZE],
        from[S_BSIZE],
        host[64],                 /* default hostname */
        inclfile[FILNSIZE],
        signature[S_BSIZE],            /* hold the signature */
        tmpdrffile[S_BSIZE];

/* Set the default user settable options */

char editor[128];
char checker[128];
char copyfile[128];
char subargs[128];
#else
#include "./s_externs.h"
#endif
