dnl
dnl $Id: aclocal.m4,v 1.6 1999/08/29 12:27:23 krueger Exp $
dnl
dnl
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl  AC_GET_BOOL(<Variable>, <Text>, <default> [,<d=allways define>])
AC_DEFUN(AC_GET_BOOL,
[
  /bin/echo "Enable $2 (y/n) ("$3") ? : \c"
  read ans
  if test "$ans" = ""; then
    ans=$3
  fi
  if test "$ans" = "y" || test "$ans" = "yes"; then
dnl    ac_def_$1="yes"
    $1="yes"
    AC_DEFINE_UNQUOTED($1, 1)
  else
dnl    ac_def_$1="no"
    $1="no"
dnl    if test "$4" = "d" then
dnl      AC_DEFINE_UNQUOTED($1, 0)
dnl    fi
  fi
])
AC_PROVIDE(AC_GET_BOOL)

dnl
dnl AC_GET_STRING(<Variable>, <Text>, <default>)
AC_DEFUN(AC_GET_STRING,
[
  /bin/echo $2 "("$3") : \c"
  read ans
  if test "$ans" = ""; then
    ans=$3
  fi
  AC_DEFINE_UNQUOTED($1, "$ans")
  ac_string_$1=$ans
])
AC_PROVIDE(AC_GET_STRING)

dnl  AC_CHECK_FILE(<Variable>, <file>)
AC_DEFUN(AC_CHECK_FILE,
[
  AC_MSG_CHECKING(for file $2)

  if test -f "$2"; then
    AC_MSG_RESULT(yes)
    AC_DEFINE_UNQUOTED(HAVE_$1, 1)
  else
    AC_MSG_RESULT(no)
    dnl AC_DEFINE_UNQUOTED(HAVE_$1, 0)
  fi
])
AC_PROVIDE(AC_CHECK_FILE)

dnl  AC_ENABLE_DEFAULT(<Variable>, <default: yes/no>, <subdirs>)
dnl
AC_DEFUN(AC_ENABLE_DEFAULT,
[
  AC_MSG_CHECKING(whether compiling $1)
  if test "$enable_$1" != "yes" && test "$enable_$1" != "no"; then
    AC_CACHE_VAL(ac_cv_enable_$1, ac_cv_enable_$1="$2")
    enable_$1=$ac_cv_enable_$1
  else
    ac_cv_enable_$1=$enable_$1
  fi
  
  if test "$enable_$1" != yes; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    $3="$$3 $1"
  fi
])
AC_PROVIDE(AC_ENABLE_DEFAULT)

dnl  AC_ENABLE_DEFAULT_CODE(<Variable>, <default: yes/no>, <pref>, <string>)
dnl
AC_DEFUN(AC_ENABLE_DEFAULT_CODE,
[
  AC_MSG_CHECKING(whether using $4)
  if test "$enable_$1" != "yes" && test "$enable_$1" != "no"; then
    AC_CACHE_VAL(ac_cv_enable_$3_$1, ac_cv_enable_$3_$1="$2")
    enable_$1=$ac_cv_enable_$3_$1
  else
    ac_cv_enable_$3_$1=$enable_$1
  fi
  if test "$enable_$1" != yes; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    if test "$3" != "lib"; then
      var=`echo $1 | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
      AC_DEFINE_UNQUOTED(HAVE_$var, 1)
    else
      if test "$ac_cv_enable_$3_$1" != "yes"; then
        AC_MSG_WARN("cannot enable $4: library $3$1 not found. disabling");
        enable_$1="no"
        ac_cv_enable_$3_$1=no
      fi
    fi
  fi
])
AC_PROVIDE(AC_ENABLE_DEFAULT_CODE)

AC_DEFUN(AC_GET_UNAME,
[
  UNAME_MACHINE=`(uname -m) 2>/dev/null` || UNAME_MACHINE=unknown
  UNAME_NODE=`(uname -n) 2>/dev/null` || UNAME_NODE=unknown
  UNAME_RELEASE=`(uname -r) 2>/dev/null` || UNAME_RELEASE=unknown
  UNAME_SYSTEM=`(uname -s) 2>/dev/null` || UNAME_SYSTEM=unknown
  UNAME_VERSION=`(uname -v) 2>/dev/null` || UNAME_VERSION=unknown
])
AC_PROVIDE(AC_GET_UNAME)

AC_DEFUN(AC_LOAD_LOCAL_CONFIG,
[
  AC_MSG_CHECKING(for local config '$1')
  if test ! -x "conf"; then
     mkdir conf
  fi
  if test ! -x "conf/sites"; then
     mkdir conf/sites
  fi
  if test -r conf/sites/$1; then
    AC_MSG_RESULT(yes)
    echo "loading local configuration '"$1"'"
    . conf/sites/$1
    for i in `set | grep CONFIG_SYS_`
    do
dnl      var=`echo $i | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'| sed s/config_sys_/enable_/`
dnl      var=`echo $i | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'| sed s/CONFIG_SYS_//`
      var=`echo $i | sed s/CONFIG_SYS_//`
      eval D_$var
    done
    for i in `set | grep CONFIG_CODE_`
    do
      var=`echo $i | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'| sed s/config_/enable_/`
      eval ac_cv_$var
    done
    for i in `set | grep CONFIG_LIB_`
    do
      var=`echo $i | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'| sed s/config_lib_/enable_/`
      eval ac_cv_$var
    done
    for i in `set | grep CONFIG_SRC_`
    do
      var=`echo $i | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'| sed s/config_src_/enable_/`
      eval ac_cv_$var
    done
    for i in `set | grep CONFIG_UIP_`
    do
      var=`echo $i | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' 'abcdefghijklmnopqrstuvwxyz'| sed s/config_uip_/enable_/`
      eval ac_cv_$var
    done
    ac_cv_enable_lib_dbm=$ac_cv_enable_code_dbm
    ac_cv_enable_lib_wrap=$ac_cv_enable_code_wrap
  else
    AC_MSG_RESULT(no)
  fi
])
AC_PROVIDE(AC_LOAD_LOCAL_CONFIG)

AC_DEFUN(AC_LOAD_INTERACTIVE_CONFIG,
[
  AC_MSG_CHECKING(for '$1')
  if test -r $1; then
    AC_MSG_RESULT(yes)
    echo "loading local configuration '"$1"'"
    . $1
  else
    AC_MSG_RESULT(no)
  fi
])
AC_PROVIDE(AC_LOAD_INTERACTIVE_CONFIG)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl  AC_CREATE_TARGET(<Variable>)
AC_DEFUN(AC_CREATE_TARGET,
[
  /bin/echo "creating setup for '$1'"
  cd conf
  mkdir -p $1
  cp sample/* $1/
  cd ..
])
AC_PROVIDE(AC_CREATE_TARGET)

dnl  AC_ENABLE_DEFAULT(<Variable>, <default: yes/no>)
dnl
AC_DEFUN(AC_SET_DEFINE,
[
  AC_MSG_CHECKING(whether define $1)
  if test "$2" != "0"; then
    AC_DEFINE_UNQUOTED($1, $2)
    AC_MSG_RESULT(yes)
  else
    dnl AC_MSG_RESULT(undefined)
    AC_MSG_RESULT(no)
  fi
])
AC_PROVIDE(AC_SET_DEFINE)

dnl  AC_CHECK_DEFINE(<var>, <define>)
dnl
AC_DEFUN(AC_CHECK_DEFINE,
[
  AC_MSG_CHECKING(whether $2 should be defined '$1')
  if test "$1" = "yes"; then
     ac_cv_$1=$1
     AC_DEFINE($2)
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi
])
AC_PROVIDE(AC_CHECK_DEFINE)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl AC_SET_FHS_PATHNAME
AC_DEFUN(AC_SET_FHS_PATHNAME,
[
if test "x$prefix" = "xNONE"; then
   prefix=$ac_default_prefix
fi
if test "x$exec_prefix" = "xNONE"; then
   exec_prefix="\${prefix}"
fi
if test "$libexecdir" = "\${exec_prefix}/libexec"; then
   libexecdir="\${exec_prefix}/lib/mh"
fi
if test "$mtbldir" = "\${mmdfprefix}/table"; then
   mtbldir="\${sysconfdir}"
fi
if test "$datadir" = "\${prefix}/share"; then
   datadir="\${prefix}/share/mmdf"
fi
if test "$sysconfdir" = "\${prefix}/etc"; then
   sysconfdir="/etc/mmdf"
fi
mmtailor="\${sysconfdir}/mmdftailor"
tbldbm="\${mtbldir}/mmdfdbm"
lckdfldir="\${uucplock}/mmdf"
authfile="\${datadir}/warning"
uucplock="\${varprefix}/lock"
resendprog="\${bindir}/resend"
sendprog="\${bindir}/send"
])
AC_PROVIDE(AC_SET_FHS_PATHNAME)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl AC_SET_OLD_PATHNAME
AC_DEFUN(AC_SET_OLD_PATHNAME,
[
if test "x$prefix" = "xNONE"; then
   prefix=$ac_default_prefix
fi
if test "$mmdfprefix" = "\${prefix}/lib/mmdf"; then
   mmdfprefix="\${prefix}/mmdf"
fi
if test "x$exec_prefix" = "xNONE"; then
   exec_prefix="\${prefix}/local"
fi
if test "$bindir" = "\${exec_prefix}/bin"; then
   bindir="\${exec_prefix}/bin"
fi
if test "$sbindir" = "\${exec_prefix}/sbin"; then
   sbindir="\${exec_prefix}/sbin"
fi
if test "$libexecdir" = "\${exec_prefix}/libexec"; then
   libexecdir="\${exec_prefix}/lib/mh"
fi
if test "$datadir" = "\${prefix}/share"; then
   datadir="\${mmdfprefix}/table"
fi
if test "$varprefix" = "/var"; then
   varprefix="/var/spool"
fi
if test "$mquedir" = "\${varprefix}/spool/mmdf/home"; then
   mquedir="\${varprefix}/mmdf/home"
fi
if test "$mphsdir" = "\${varprefix}/state/mmdf"; then
   mphsdir="\${varprefix}/mmdf/log/phase"
fi
if test "$mlogdir" = "\${varprefix}/log/mmdf"; then
   mlogdir="\${varprefix}/mmdf/log"
fi
if test "$sharedstatedir" != "\${prefix}/com"; then
   dnl sharedstatedir="\${prefix}/spool/mmdf/home"
   echo "used old syntax --sharedstatedir="
   if test "$used_mquedir" = 0; then
      mquedir=$sharedstatedir
   else
      AC_MSG_WARN("Ignoring option 'sharedstatedir='")
   fi
fi
if test "$localstatedir" != "\${prefix}/var"; then
   dnl localstatedir="\${prefix}/spool/mmdf/log"
   echo "used old syntax --localstatedir="
   if test "$used_mlogdir" = 0 && test "$used_mphsdir" = 0 && \
	test "$used_d_calllog" = 0 ; then
      mlogdir=$localstatedir
      mphsdir=$localstatedir/phase
      d_calllog=$localstatedir/dial_log
   else
      AC_MSG_WARN("Ignoring option 'localstatedir'")
   fi
fi
if test "$sysconfdir" = "\${prefix}/etc"; then
   sysconfdir="\${mmdfprefix}"
fi
mmtailor="\${sysconfdir}/mmdftailor"
tbldbm="\${mtbldir}/mmdfdbm"
lckdfldir="/tmp/mmdf"
authfile="\${sysconfdir}/warning"
uucplock="/var/spool/locks"
resendprog="\${bindir}/resend"
sendprog="\${bindir}/send"
])
AC_PROVIDE(AC_SET_OLD_PATHNAME)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl
AC_DEFUN(AC_C_SUBST,[
  eval "dummy1=\$"$1
  while true
  do
    if `echo "$dummy1" | grep >/dev/null 2>&1 "{"`; then
      eval "dummy2=$dummy1"
      dummy1=$dummy2
    else
      break
    fi
    if test "$dummy1" = ""; then
      break;
    fi
  done
dnl  eval $2=$dummy1
dnl  AC_SUBST($2)
  c_$1=$dummy1
  AC_SUBST_FILE(c_$1)
])
AC_PROVIDE(AC_C_SUBST)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
