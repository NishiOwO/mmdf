dnl
dnl $Id: acsite.m4,v 1.1 2001/10/06 21:27:17 krueger Exp $
dnl
dnl
dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
AC_DEFUN(AC_INIT_MMDFVARS,
[
libexecdir='${exec_prefix}/lib/mh'

mmdfprefix='${prefix}/lib/mmdf'
mcmddir='${mmdfprefix}/lib'
mchndir='${mmdfprefix}/chans'
mtbldir='${mmdfprefix}/table'
# mquedir='${varprefix}/mmdf/home'
# mphsdir='${varprefix}/mmdf/log/phase'
# mlogdir='${varprefix}/mmdf/log'
mquedir='${varprefix}/spool/mmdf/home'
mphsdir='${varprefix}/state/mmdf'
mlogdir='${varprefix}/log/mmdf'
d_calllog='${mlogdir}/dial_log'
if test "$mmdfdebug"  = ""; then mmdfdebug=0;  fi
if test "$mmdfdlog"   = ""; then mmdfdlog=0;   fi
if test "$mmdfdbglog" = ""; then mmdfdbglog=0; fi
if test "$mmdfnodomlit" = ""; then mmdfnodomlit=0; fi
if test "$mmdfleftdots" = ""; then mmdfleftdots=0; fi
if test "$mmdfstatsort" = ""; then mmdfstatsort=0; fi
if test "$mmdfcitation" = ""; then mmdfcitation=0; fi
used_mcmddir=0
used_mchndir=0
used_mtbldir=0
used_mphsdir=0
used_mlogdir=0
used_d_calllog=0
used_mquedir=0
])
AC_PROVIDE(AC_INIT_MMDFVARS)

dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
AC_DEFUN(AC_MMDFARGS_HELP,
[
    cat << EOF
  --mmdf-debug=[0,1,2]    enable more logging information      [default: $mmdfdebug]
  --mmdf-dlog             enable debug-option of dial packages [default: $mmdfdlog]
  --mmdf-dbglog           enable more debug-option of dial packages [default: $mmdfdbglog]
  --mmdf-nodomlit         toggle define NODOMLIT [default: $mmdfnodomlit]
  --mmdf-leftdots         toggle define LEFTDOTS [default: $mmdfleftdots]
  --mmdf-statsort         toggle define STATSORT [default: $mmdfstatsort]
  --mmdf-citation         toggle define CITATION [default: $mmdfcitation]

  --mmdf-prefix=DIR       default home of mmdf in DIR [PREFIX/mmdf]

  --mmdf-mlogdir=DIR      x in DIR [VARPREFIX/mmdf/log]
  --mmdf-mphsdir=DIR      x in DIR [VARPREFIX/mmdf/log/phase]
  --mmdf-mcmddir=DIR      x in DIR [MMDFPREFIX/lib]
  --mmdf-mchndir=DIR      x in DIR [MMDFPREFIX/chans]
  --mmdf-mtbldir=DIR      x in DIR [MMDFPREFIX/table]
  --mmdf-d_calllog=FILE   x in FILE [MLOGDIR/dial_log]

  --mmdf-spooldir=DIR     obselete --mmdf-mquedir
  --mmdf-logdir=DIR       obselete --mmdf-mlogdir
  --mmdf-phasedir=DIR     obselete --mmdf-mphsdir
  --mmdf-diallog=FILE     obselete --mmdf-d_calllog
EOF
])
AC_PROVIDE(AC_MMDFARGS_HELP)
