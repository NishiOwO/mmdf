/*
 * $Id: config.h,v 1.1 1999/08/16 11:32:04 krueger Exp $
 *
 * most of the define-switches can be set by running configure.
 * some old defines are still in use
 * please set these configuration-switch here
 *
 */


#ifndef CONFIG_OLD_H
#define CONFIG_OLD_H

/*            This makes the channel programs operate inde-
 *             pendently  from deliver.  It hasn't been used
 *             for years and requires reading  the  code  to
 *             understand   its   effect.    Do  NOT  define
 *             RUNALONE! (it is used for debugging)
 */
/* #undef RUNALONE */

/*             Enables code that is specific for 4.2  BSD  .
 *             All  4.2 BSD sites must define this variable.
 *             This should be used by 4.3 BSD sites as well.
 *             4.3   BSD   sites  will  also  want  the  new
 *             src/smtp/smtpd.4.3.c  for  use  with   INETD.
 *             They  will  have to compile this manually for
 *             now (or copy it to smtpd.4.2.c).
 */
/* #define V4_2BSD */

/*             Enables  code  that does Bell System V tricks
 *             (probably also useful for System III  instal-
 *             lations).  Note: if you are running System V,
 *             you must  also  append  -Drindex=strrchr  and
 *             -Dindex=strchr to CFLAGS.
 */
/* #define SYS5 */

/*             Enables  code  specific to System Vr3.  Note:
 *             if you have the WIN/TCP code you may need  to
 *             add  -I/usr/netinclude to CFLAGS and -lnet to
 *             NETLIBS.
 */
/* #undef SYS5r3 */

/*             Define this translation if  your  C  compiler
 *             doesn't understand voids (e.g. V7).
 */
/* #undef -Dvoid=int */

/*             Define CITATION=n to limit (to n) the  number
 *             of lines of text included in error returns by
 *             rmail and niftp.  n should be at  least  six,
 *             if  CITATION  is  defined  at  all.   If  not
 *             defined, rmail will return the entire message
 *             and niftp will return the first 500 lines.
 */
/* #undef CITATION=n */

#define HAVE_LIBGDBM  /* new */
#define HAVE_MMAP
#define HAVE_LONG_FILE_NAMES
#define TM_IN_SYS_TIME
#undef HAVE_SIGSET
#undef HAVE_SIGSETMASK

/*#undef HAVE_SGTTY_H */

/**********************************************************************/
/*             Enable site depend code
 */

/*             Enable a small fix to slowdown submitm such that the parent
 *             could finish readring the pipe when submit exits. This is
 *             needed on RISC-machines like HP9000s700 an newer.
 */
/* #undef SUBMIT_TOO_FAST */

#endif /* CONFIG_OLD_H */
