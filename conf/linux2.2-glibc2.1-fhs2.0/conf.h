/* 
 *  include Config-Switches
 */
#include "config.h"

#
/*          This contains the most site-dependent definitions             */

#define USERSIZE    8		/* max length of a user system name   */
#define	MAILIDSIZ  64		/* max length of a mailid,
				 * must be at least USERSIZE.		*/
#define MSGNSIZE   64           /* max length of a message file name  */
#define FILNSIZE  512           /* max length of full file name       */
#define ADDRSIZE 8192		/* max length of an address           */
#define LINESIZE 8192		/* max length of a line/record        */

#define	NUMCHANS   16		/* max number of channels */ /* I have 11 */
#define NUMTABLES  32	/* max number of tables */	/* I have 21 */
#define NUMDOMAINS 12	/* max number of domains */   /* I have 6 */

/************  DIAL IN/OUT (CH_PHONE / SLAVE) TAILORING  *****************/

#define DL_TRIES    2             /* number of re-connects OK for phone */
#define DL_RCVSZ    255           /* longest line receivable            */
#define DL_XMTSZ    255           /* longest line sendable              */

/****************  JNTMAIL Tailoring *******************/

#undef	JNTMAIL
#undef	BOTHEND
#undef	VIATRACE

/********************* Privileged UIDs *************************/

/*
 *  This can be a macro as give here or you can provide a function.
 *  It should always authorize root and mmdf.
 *  The global variable effecid is assumed to have the MMDF uid in it.
 */

#define	PRIV_USER(x) ((x)==0 || (x)==1 \
	|| (x)==5 || (x)==55715 || (x)==55724 \
	|| (x)==effecid || (x)==1170 || (x)==240)
