#include "util.h"
#include "ll_log.h"

/* Augment ll_io routines to print system call errno value */

#if !HAVE_SYS_ERRLIST_DECL
#if 0
extern	int	sys_nerr;
extern	char	*sys_errlist[];
#endif
#endif /* HAVE_SYS_ERRLIST_DECL */
extern	int	errno;

/* VARARGS3 */
int ll_err(loginfo, level, fmt, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9)
struct ll_struct *loginfo;
int level;
char *fmt, *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	char	newfmt[128];

	(void)snprintf(newfmt, sizeof(newfmt),  "%s%s",  "[SYSERR(%d)%s]  ",  fmt);

	return(ll_log(loginfo,  level,  newfmt, errno,
		(errno >= 0 && errno <= sys_nerr)
			? sys_errlist[errno] : "<- Illegal errno",
		a0, a1, a2, a3, a4, a5, a6, a7, a8, a9));
}
