/*
 * $Id: config.h,v 1.9 1998/05/25 07:45:15 krueger Exp $
 *
 * please set every configuration-switch here
 *
 */


#ifndef CONFIG_OLD_H
#define CONFIG_OLD_H

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
#define DEBUG 1

/*             The  same  caution  applies  for D_LOG as for
 *             DEBUG.  This variable controls logging in the
 *             dial  package.   If you are tight on space or
 *             if you don't use the dial package  (no  phone
 *             or pobox channels) you may safely omit D_LOG.
 */
/* #undef D_LOG */

/*             This controls more debug logging for the dial
 *             package.  Again, if you have the space and if
 *             you use the dial package  (PhoneNet),  define
 *             it  to  allow  extensive  tracing if problems
 *             arise later.
 */
/* #undef D_DBGLOG */

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
#define V4_2BSD

/*             Enables  code  that does Bell System V tricks
 *             (probably also useful for System III  instal-
 *             lations).  Note: if you are running System V,
 *             you must  also  append  -Drindex=strrchr  and
 *             -Dindex=strchr to CFLAGS.
 */
/* #undef SYS5 */

/*             Enables  code  specific to System Vr3.  Note:
 *             if you have the WIN/TCP code you may need  to
 *             add  -I/usr/netinclude to CFLAGS and -lnet to
 *             NETLIBS.
 */
/* #undef SYS5r3 */

/*             Prevents    Domain    Literals    (such    as
 *             [10.0.0.59])  from  appearing  in  addresses.
 *             Since  doesn't currently handle Domain Liter-
 *             als  properly, use of this option is strongly
 *             advised.
 */
#define NODOMLIT

/*             Enables intpretation of dots as delimiters on
 *             the  LHS  of an @ in addresses.  For example,
 *             "user.host@yourdomain".  Since % or 822-rout-
 *             ing  is  preferred here, only enable LEFTDOTS
 *             if you have historical need to. 
 */
/* #undef LEFTDOTS */

/*             Define this translation if  your  C  compiler
 *             doesn't understand voids (e.g. V7).
 */
/* #undef -Dvoid=int */

/*             Define  this variable if your machine doesn't
 *             allow variable numbers  of  arguments  to  be
 *             passed  to  routines  the way Vaxes (and some
 *             others) do.
 *    set by configure in config.h.in
 */
/* #undef NO_VARARGS */

/*             Define CITATION=n to limit (to n) the  number
 *             of lines of text included in error returns by
 *             rmail and niftp.  n should be at  least  six,
 *             if  CITATION  is  defined  at  all.   If  not
 *             defined, rmail will return the entire message
 *             and niftp will return the first 500 lines.
 */
/* #undef CITATION=n */

/*             Define  DBMCACHE  if  your  version of dbm(3)
 *             builds databases in core and then dumps  them
 *             to disk with dbmcachedump().
 */
/* #undef DBMCACHE */

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
/* #undef STATSORT */

/**********************************************************************/
/*             Enable site depend code
 */
/* #undef LINUX */

/*             Enable a small fix to slowdown submitm such that the parent
 *             could finish readring the pipe when submit exits. This is
 *             needed on RISC-machines like HP9000s700 an newer.
 */
/* #undef SUBMIT_TOO_FAST */

/*             Enable 8bit-clean mode when getting messages over smtp.
 */
/* #undef EIGHT_BIT_CLEAN */

/*             Enable tcp_wrapper implementation in smtpsrvr. With the 
 *             tcp_wrapper package you can monitor and filter incoming request
 *             to the smtp server. Over the ident-protocol (RFC931) you can 
 *             get the calling username.
 *    renamed to HAVE_LIBWRAP
 *    set by configure in config.h.in
 */
/* #undef HAVE_TCP_WRAPPER */

/*             Enable the yp_match lookup code for accessing tables over
 *             NIS.
 */
/* #undef HAVE_NIS */

#endif /* CONFIG_OLD_H */
