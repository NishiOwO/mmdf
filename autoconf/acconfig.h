/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/*
 * $Id: acconfig.h,v 1.5 1999/08/12 13:15:31 krueger Exp $
 *
 */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

/* Define if using alloca.c.  */
#undef C_ALLOCA

/* Define if type char is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
#undef __CHAR_UNSIGNED__
#endif

/* Define if the closedir function returns void instead of int.  */
#undef CLOSEDIR_VOID

/* Define to empty if the keyword does not work.  */
#undef const

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define for DGUX with <sys/dg_sys_info.h>.  */
#undef DGUX

/* Define if you have <dirent.h>.  */
#undef DIRENT

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#undef GETGROUPS_T

/* Define if the `getloadavg' function needs to be run setuid or setgid.  */
#undef GETLOADAVG_PRIVILEGED

/* Define if the `getpgrp' function takes no argument.  */
#undef GETPGRP_VOID

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef gid_t

/* Define if you have alloca, as a function or macro.  */
#undef HAVE_ALLOCA

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#undef HAVE_ALLOCA_H

/* Define if you don't have vprintf but do have _doprnt.  */
#undef HAVE_DOPRNT

/* Define if your system has its own `getloadavg' function.  */
#undef HAVE_GETLOADAVG

/* Define if you have the getmntent function.  */
#undef HAVE_GETMNTENT

/* Define if the `long double' type works.  */
#undef HAVE_LONG_DOUBLE

/* Define if you support file names longer than 14 characters.  */
#undef HAVE_LONG_FILE_NAMES

/* Define if you have a working `mmap' system call.  */
#undef HAVE_MMAP

/* Define if system calls automatically restart after interruption
   by a signal.  */
#undef HAVE_RESTARTABLE_SYSCALLS

/* Define if your struct stat has st_blksize.  */
#undef HAVE_ST_BLKSIZE

/* Define if your struct stat has st_blocks.  */
#undef HAVE_ST_BLOCKS

/* Define if you have the strcoll function and it is properly defined.  */
#undef HAVE_STRCOLL

/* Define if your struct stat has st_rdev.  */
#undef HAVE_ST_RDEV

/* Define if you have the strftime function.  */
#undef HAVE_STRFTIME

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#undef HAVE_SYS_WAIT_H

/* Define if your struct tm has tm_zone.  */
#undef HAVE_TM_ZONE

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#undef HAVE_TZNAME

/* Define if you have <unistd.h>.  */
#undef HAVE_UNISTD_H

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
#undef HAVE_UTIME_NULL

/* Define if you have <vfork.h>.  */
#undef HAVE_VFORK_H

/* Define if you have the vprintf function.  */
#undef HAVE_VPRINTF

/* Define if you have the wait3 system call.  */
#undef HAVE_WAIT3

/* Define as __inline if that's what the C compiler calls it.  */
#undef inline

/* Define if int is 16 bits instead of 32.  */
#undef INT_16_BITS

/* Define if long int is 64 bits.  */
#undef LONG_64_BITS

/* Define if major, minor, and makedev are declared in <mkdev.h>.  */
#undef MAJOR_IN_MKDEV

/* Define if major, minor, and makedev are declared in <sysmacros.h>.  */
#undef MAJOR_IN_SYSMACROS

/* Define if on MINIX.  */
#undef _MINIX

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef mode_t

/* Define if you don't have <dirent.h>, but have <ndir.h>.  */
#undef NDIR

/* Define if you have <memory.h>, and <string.h> doesn't declare the
   mem* functions.  */
#undef NEED_MEMORY_H

/* Define if your struct nlist has an n_un member.  */
#undef NLIST_NAME_UNION

/* Define if you have <nlist.h>.  */
#undef NLIST_STRUCT

/* Define if your C compiler doesn't accept -c and -o together.  */
#undef NO_MINUS_C_MINUS_O

/* Define to `long' if <sys/types.h> doesn't define.  */
#undef off_t

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef pid_t

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
#undef _POSIX_1_SOURCE

/* Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* Define as the return type of signal handlers (int or void).  */
#undef RETSIGTYPE

/* Define if the setvbuf function takes the buffering type as its second
   argument and the buffer pointer as the third, as on System V
   before release 3.  */
#undef SETVBUF_REVERSED

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
#undef STACK_DIRECTION

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
#undef STAT_MACROS_BROKEN

/* Define if you have the ANSI C header files.  */
#undef STDC_HEADERS

/* Define on System V Release 4.  */
#undef SVR4

/* Define if you don't have <dirent.h>, but have <sys/dir.h>.  */
#undef SYSDIR

/* Define if you don't have <dirent.h>, but have <sys/ndir.h>.  */
#undef SYSNDIR

/* Define if `sys_siglist' is declared by <signal.h>.  */
#undef SYS_SIGLIST_DECLARED

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if your <sys/time.h> declares struct tm.  */
#undef TM_IN_SYS_TIME

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef uid_t

/* Define for Encore UMAX.  */
#undef UMAX

/* Define for Encore UMAX 4.3 that has <inq_status/cpustats.h>
   instead of <sys/cpustats.h>.  */
#undef UMAX4_3

/* Define if you do not have <strings.h>, index, bzero, etc..  */
#undef USG

/* Define vfork as fork if vfork does not work.  */
#undef vfork

/* Define if the closedir function returns void instead of int.  */
#undef VOID_CLOSEDIR

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#undef WORDS_BIGENDIAN

/* Define if lex declares yytext as a char * by default, not a char[].  */
#undef YYTEXT_POINTER

/************************************************************************/

/* Define if sprintf does not return number of printed chars */
#undef BSD_SPRINTF

/* Define if you have the flock function.  */
#undef HAVE_FLOCK
#undef HAVE_F_SETLK
#undef HAVE_LOCK_FILES

/* Define if you have struct __res_state instead of struct state */
#undef HAVE_RES_STATE

/* Define if you have _NFILE */
#undef HAVE_NFILE

/* Define if you have the gdbm library (-lgdbm).  */
#undef HAVE_LIBGDBM  /* new */

/*****************************************************************************/
/*             Enable 8bit-clean mode when getting messages over smtp.
 */
#undef HAVE_8BIT

/*             Enables  the  d_assign code in the dial pack-
 *             age.  This code calls the program /bin/assign
 *             to  gain exclusive access to a file.  It does
 *             not appear that the  d_assign  code  is  ever
 *             used so don't bother defining HAVE_ASSIGN.
 */
#undef HAVE_ASSIGN

/*             Many  sites  are  running  their systems with
 *             TTYs  in  more  secure  mode  than  generally
 *             writable.  Usually these systems use the exe-
 *             cute bit to  indicate  write  permission  and
 *             have  a  privileged  program make the access.
 *             If you are such a  site,  you  will  want  to
 *             investigate  the effect of HAVE_SECURETTY and
 *             modify it if necessary. Vanilla sites  should
 *             not define HAVE_SECURETTY. BRL VAX UNIX sites
 *             must define HAVE_SECURETTY.
 */
#undef HAVE_SECURETTY

/*             Most sites should enable DEBUG=1 unless there
 *             is  a  serious  crunch  for space.  This will
 *             give you fairly  detailed  debugging  of  the
 *             system  if you need it.  Setting DEBUG=2 will
 *             include  even  more  debugging  for   address
 *             parser  and reformatting code.  If you do not
 *             enable DEBUG=1,  you  will  seriously  affect
 *             your  ability  to trace problems later.  Once
 *             you have your system up an  running  reliably
 *             you  can recompile it without DEBUG of either
 *             kind if you want the space and the very minor
 *             performance increase.
 */
#undef DEBUG

/*             The  same  caution  applies  for D_LOG as for
 *             DEBUG.  This variable controls logging in the
 *             dial  package.   If you are tight on space or
 *             if you don't use the dial package  (no  phone
 *             or pobox channels) you may safely omit D_LOG.
 */
#undef D_LOG

/*             This controls more debug logging for the dial
 *             package.  Again, if you have the space and if
 *             you use the dial package  (PhoneNet),  define
 *             it  to  allow  extensive  tracing if problems
 *             arise later.
 */
#undef D_DBGLOG

/*            This makes the channel programs operate inde-
 *             pendently  from deliver.  It hasn't been used
 *             for years and requires reading  the  code  to
 *             understand   its   effect.    Do  NOT  define
 *             RUNALONE! (it is used for debugging)
 */
#undef RUNALON

/*             Prevents the dial package (PhoneNet protocol)
 *             from  being  compiled.   This  saves  a  fair
 *             amount  of  space  and  compile time.  Define
 *             HAVE_DIAL if you do not intend to use the  phone
 *             or  pobox  channels.   (You will also need to
 *             take `dial' out of Makefile.lib, see  below.)
 */
#undef HAVE_DIAL

/*             If  you  defined NODUP2, then you should also
 *             define NOFCNTL if you don't have the fcntl(x,
 *             F_DUPFD) system call either.
 */
#undef HAVE_FCNTL_F_DUPFD

/*             Enables   the   nameserver  lookup  code  for
 *             accessing domain servers.The  HAVE_NAMESERVER
 *             support   is   new.  If  turned  on  it  will
 *             automatically compile the support for  4.2BSD
 *             otherwise the 'fake' code  will  be  compiled
 *             Currently  there  is  only support for 4.2BSD
 *             networking.  See TB_NS below.
 */
#undef HAVE_NAMESERVER
#undef HAVE__GETSHORT
#undef HAVE_GETSHORT

/*             Enables   the   nameserver  lookup  code  for
 *             accessing domain servers.The  HAVE_NAMESERVER
 *             support   is   new.  If  turned  on  it  will
 *             automatically compile the support for  4.2BSD
 *             otherwise the 'fake' code  will  be  compiled
 *             Currently  there  is  only support for 4.2BSD
 *             networking.  See TB_NS below.
 */
#undef HAVE_NAMESERVER

/*             Prevents    Domain    Literals    (such    as
 *             [10.0.0.59])  from  appearing  in  addresses.
 *             Since  doesn't currently handle Domain Liter-
 *             als  properly, use of this option is strongly
 *             advised.
 */
#undef NODOMLIT

/*             Enables intpretation of dots as delimiters on
 *             the  LHS  of an @ in addresses.  For  example,
 *             "user.host\@yourdomain".  Since % or
 *             822-routing  is  preferred  here, only enable
 *             LEFTDOTS if you have historical need to. 
 */
#undef LEFTDOTS

/*             Define  this variable if your machine doesn't
 *             allow variable numbers  of  arguments  to  be
 *             passed  to  routines  the way Vaxes (and some
 *             others) do. If you have varargs.h don't  care
 *             about NO_VARARGS
 */
#undef HAVE_VARARGS_H
#undef NO_VARARGS

/*             Define CITATION=n to limit (to n) the  number
 *             of lines of text included in error returns by
 *             rmail and niftp.  n should be at  least  six,
 *             if  CITATION  is  defined  at  all.   If  not
 *             defined, rmail will return the entire message
 *             and niftp will return the first 500 lines.
 */
#undef CITATION

/*             Define HAVE_DBMCACHEDUMP if your  version  of
 *             dbm(3)  builds  databases  in  core  and then
 *             dumps them to disk with dbmcachedump().
 */
#undef HAVE_DBMCACHEDUMP

/*             Define  STATSORT  if you want deliver to sort
 *             the mail queue based on a stat(2) of the mes-
 *             sage  text file instead of reading the times-
 *             tamp that is stored in  the  message  address
 *             file.   Using  stat()  is much more efficient
 *             but it assumes the modified date of the  text
 *             file  hasn't  been  changed since the message
 *             was queued.  This is usually a  safe  assump-
 *             tion  so  defining  STATSORT  is recommended.
 *             Tailor the CONFIGDEFS  line  to  your  site's
 *             requirements and system type. 
 */
#undef STATSORT

/*             Enable the yp_match lookup code for accessing tables over
 *             NIS.
 */
#undef HAVE_NIS

/*
 */
#undef HAVE_NN

/*             Enable code to disable source-routing of addresses
 *             Should not be done according to RFC 1123, but needed for 
 *             sendmail-host that don't accept source-routes to avoid
 *             spam-mails.
 */
#undef HAVE_NOSRCROUTE

/*             Enable a small fix to slowdown submitm such that the parent
 *             could finish readring the pipe when submit exits. This is
 *             needed on RISC-machines like HP9000s700 an newer.
 */
#undef SUBMIT_TOO_FAST

/* Define if your system has the define SIGSYS */
#undef HAVE_DEF_SIGSYS

/* Define if you have the <fcntl.h> header file.  */
#undef HAVE_FCNTL_H

/* Define if you have the <sgtty.h> header file.  */
#undef HAVE_SGTTY_H

/* Define if you have CBREAK in <sgtty.h> header file.  */
#undef HAVE_SGTTY_CBREAK

/* Define if you have TIOCSETN in <sgtty.h> header file.  */
#undef HAVE_SGTTY_TIOCSETN

/* Define if ioctl(1, TIOCSBRK, 0) works */
#undef HAVE_IOCTL_TIOCSBRK

/* Define if you have TIOCSETN in <termio.h> header file.  */
#undef HAVE_TERMIO_TIOCSETN

#undef HAVE_TSECURETTY

/* Define if your system has sys_errlist[] declared */
#undef HAVE_SYS_ERRLIST_DECL

#undef HAVE_UNION_WAIT
#undef LINUX
#undef HPUX
#undef MKDIR_HAVE_SECOND_ARG



/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
