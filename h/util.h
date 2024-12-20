/* 
 *  include Config-Switches
 */
#ifndef DIDUTIL
#define DIDUTIL

#include "config.h"

#include <stdio.h>                /* minus the ctype stuff */
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#include <sys/types.h>	/* ONLY include this file from here, any other file
			 * which needs sys/types.h should #include "util.h".
			 * Not all Unix systems have tricks so that you
			 * can include sys/types.h arbitrary numbers of
			 * of times. -- DSH
			 */
#include <errno.h>
#include <string.h>

/* declarations that should have been in the system files */

#ifdef DECLARE_SPRINTF
extern SPRINTF_RETURN sprintf ();
#endif /* DECLARE_SPRINTF */

#if !defined(__STDC__) || defined(DECLARE_GETPWNAM)
extern struct passwd *getpwnam(), *getpwuid();
#else
#  ifdef HAVE_LIBIO_H
#    include <libio.h>
#  endif /* HAVE_LIBIO_H */
#endif /* not LINUX */

#if !defined(HAVE_DEF_SIGSYS)
#  define SIGSYS SIGUNUSED
#endif

/* */

extern jmp_buf timerest;

/* some common logical values */

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif
#ifndef YES
#define YES     1
#endif
#ifndef NO
#define NO      0
#endif
#ifndef OK
#define OK      0
#endif
#ifndef DONE
#define DONE    1
#endif
#ifndef NOTOK
#define NOTOK   -1
#endif
#ifndef	MAYBE
#define	MAYBE	-2
#endif

# define ML_FRESH  0
# define ML_MSG    1
# define ML_HEADER 2
# define ML_TEXT   3

/* some C compilers do not support 'global' statics properly */

#ifndef LOCFUN
#define LOCFUN static             /* function local to module           */
#endif
#ifndef LOCVAR
#define LOCVAR static             /* variable local to module           */
#endif

/* stdio extensions */

#define lowtoup(chr) (islower(chr)?toupper(chr):chr)
#define uptolow(chr) (isupper(chr)?tolower(chr):chr)
#define min(a,b) ((b<a)?b:a)
#define isstr(ptr) ((ptr) != 0 && (ptr)[0] != '\0')
#define isnull(chr) ((chr) == '\0')

/* fix for systems that can't accept null pointers passed to printf */
#define seenull(ptr)	(((ptr)!=(char *)0)?(ptr):"(null)")

#define FOREVER   for (;;)

union pipunion
{
    int     pipcall[2];
    struct pipstruct
    {
	int     prd;
	int     pwrt;
    } pip;
};

typedef union pipunion Pip;

#ifdef v6
long	siz2lon();
#define st_gsize(ino_ptr) ((long)(siz2lon((struct stat *)ino_ptr)))
#else
#define st_gsize(inode) ((long)((struct stat *)inode)->st_size)
#endif

#define gwaitval(val)   ((val) >> 8)
				  /* get exit() value from child        */
				  /* val is the argument to wait()      */
				  /* this macro expects to extract the  */
				  /* high byte from the "returned"      */
				  /* value                              */
#endif /* DIDUTIL */
