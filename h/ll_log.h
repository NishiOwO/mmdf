/*      Definitions for the llog status-recording package                 */

#include "util.h"

#define LLOGCLS 01		  /* Keep log closed, except when writing */
#define LLOGCYC 02		  /* Cycle to beginning when log full     */
#define LLOGWAT 04		  /* Wait, if log locked and LLOGCLS      */
#define LLOGSOME 05		  /* Occasional logging                   */
#define LLOGTIMED 014		  /* used timed logs with locking         */

#define LLOGFAT  -1		  /* Fatal error                          */
#define LLOGTMP   5		  /* Temporary (minor?) error             */
#define LLOGGEN  10		  /* General information                  */
#define LLOGBST  20		  /* Basic statistics                     */
#define LLOGFST  25		  /* Full statistics                      */
#define LLOGPTR  40               /* Trace of overall program phases      */
#define LLOGBTR  50		  /* Trace of basic program acitivity     */
#define LLOGFTR  55		  /* Full program trace                   */
#define LLOGMAX 126		  /* Maximum possible value               */

#define LLTIMEOUT (5 * 60)	  /* default time out period if LLOGTIMED */


struct  ll_struct
{
    char   *ll_file;               /* path name to logging file            */
    char   *ll_hdr;                /* text to put in opening line          */
    int     ll_level;              /* don't print entries higher than this */
    int     ll_msize;              /* max size for log, in 100's of blocks */
    int     ll_stat;               /* assorted switches                    */
    int     ll_timmax;		   /* how long can fd be in use?           */
    time_t  ll_timer;		   /* how long has fd been in use?         */
    int     ll_fd;                 /* holds file descriptor                */
    FILE   *ll_fp;                 /* Standard IO stream pointer           */
};

typedef struct ll_struct Llog;

typedef struct ll_struct LLog;

extern int ll_open ();
extern int ll_close ();
extern void ll_hdinit ();
extern int ll_init ();
extern int ll_log ();
