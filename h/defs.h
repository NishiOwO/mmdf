/*
 *  $Id: defs.h,v 1.1 1998/10/07 13:13:31 krueger Exp $
 *
 *  Included by config.h
 *  Contains site specific dependencies based on the defines in
 *  config.h and those used by the compiler.
 */

/* Included from unistd.h:
 * Both System V and BSD have `setpgrp' functions, but with different
   calling conventions.  The BSD function is the same as POSIX.1 `setpgid'
   (above).  The System V function takes no arguments and puts the calling
   process in its on group like `setpgid (0, 0)'.

   New programs should always use `setpgid' instead.

   The default in GNU is to provide the System V function.  The BSD
   function is available under -D_BSD_SOURCE with -lbsd-compat.  */
#if defined(__FAVOR_BSD)
#  undef HAVE_SETPGRP
#endif
