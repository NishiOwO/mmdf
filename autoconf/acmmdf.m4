dnl lllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll
dnl AC_DEFUN(AC_INIT_PARSE_MMDF_ARGS,
dnl [ 
dnl ac_option=$1
dnl ac_optarg=$2
dnl ac_prev=$3


  case "$ac_option" in

  -mlogdir | --mlogdir | --mlogdi | --mlogd | --mlog)
    ac_prev=mlogdir ;;
  -mlogdir=* | --mlogdir=* | --mlogdi=* | --mlogd=* | --mlog=*)
    mlogdir="$ac_optarg"; used_mlogdir=1 ;;

  -mmdf-citation | --mmdf-citation | --mmdf-citatio | --mmdf-citati \
  | --mmdf-citat | --mmdf-cita | --mmdf-cit | --mmdf-ci | --mmdf-c )
    ac_prev=mmdfcitation ;;
  -mmdf-citation=* | --mmdf-citation=* | --mmdf-citatio=* | --mmdf-citati=* \
  | --mmdf-citat=* | --mmdf-cita=* | --mmdf-cit=* | --mmdf-ci=* \
  | --mmdf-c=* )
    mmdfcitation="$ac_optarg" ;;

  -mmdf-dbglog | --mmdf-dbglog | --mmdf-dbglo | --mmdf-dbgl \
  | --mmdf-dbg | --mmdf-db )
    if test "$mmdfdbglog" = 0; then
	mmdfdbglog=1 ;
    else
	mmdfdbglog=0 ;
    fi;;

  -mmdf-debug | --mmdf-debug | --mmdf-debu | --mmdf-deb | --mmdf-de )
    ac_prev=mmdfdebug ;;
  -mmdf-debug=* | --mmdf-debug=* | --mmdf-debu=* | --mmdf-deb=* | --mmdf-de=* )
    mmdfdebug="$ac_optarg" ;;

  -mmdf-d_calllog | --mmdf-d_calllog | --mmdf-d_calllo | --mmdf-d_calll \
  | --mmdf-d_call)
    ac_prev=d_calllog ;;
  -mmdf-d_calllog=* | --mmdf-d_calllog=* | --mmdf-d_calllo=* | --mmdf-d_calll=* \
  | --mmdf-d_call=*)
    d_calllog="$ac_optarg"; used_d_calllog=1 ;;

  -mmdf-dlog | --mmdf-dlog | --mmdf-dlo | --mmdf-dl )
    if test "$mmdfdlog" = 0; then
        mmdfdlog=1 ;
    else
	mmdfdlog=0 ;
    fi;;

  -mmdf-leftdots | --mmdf-leftdots | --mmdf-leftdot | --mmdf-leftdo \
  | --mmdf-leftd | --mmdf-left | --mmdf-lef | --mmdf-le )
    if test "$mmdfleftdots" = 0; then
	mmdfleftdots=1
    else
	mmdfleftdots=0
    fi ;;

  -mmdf-nodomlit | --mmdf-nodomlit | --mmdf-nodomli | --mmdf-nodoml \
  | --mmdf-nodom | --mmdf-nodo | --mmdf-nod | --mmdf-no | --mmdf-n )
    if test "$mmdfnodomlit" = 0; then
	mmdfnodomlit=1
    else
	mmdfnodomlit=0
    fi ;;

  -mmdf-prefix | --mmdf-prefix | --mmdf-prefi | --mmdf-pref | --mmdf-pre \
  | --mmdf-pr )
    ac_prev=mmdfprefix ;;
  -mmdf-prefix=* | --mmdf-prefix=* | --mmdf-prefi=* | --mmdf-pref=* \
  | --mmdf-pre=* | --mmdf-pr=* )
    mmdfprefix="$ac_optarg"; used_mmdfprefix=1 ;;

  -mmdf-statsort | --mmdf-statsort | --mmdf-statsor | --mmdf-statso \
  | --mmdf-stats | --mmdf-stat | --mmdf-sta | --mmdf-st )
    if test "$mmdfstatsort" = "0"; then
	mmdfstatsort=1
    else
	mmdfstatsort=0
    fi ;;

  -mcmddir | --mcmddir | --mcmddi | --mcmdd | --mcmd)
    ac_prev=mcmddir ;;
  -mcmddir=* | --mcmddir=* | --mcmddi=* | --mcmdd=* | --mcmd=*)
    mcmddir="$ac_optarg"; used_mcmddir=1 ;;

  -mchndir | --mchndir | --mchndi | --mchnd | --mchn)
    ac_prev=mchndir ;;
  -mchndir=* | --mchndir=* | --mchndi=* | --mchnd=* | --mchn=*)
    mchndir="$ac_optarg"; used_mchndir=1 ;;

  -mtbldir | --mtbldir | --mtbldi | --mtbld | --mtbl)
    ac_prev=mtbldir ;;
  -mtbldir=* | --mtbldir=* | --mtbldi=* | --mtbld=* | --mtbl=*)
    mtbldir="$ac_optarg"; used_mtbldir=1 ;;

  -mphsdir | --mphsdir | --mphsdi | --mphsd | --mphs)
    ac_prev=mphsdir ;;
  -mphsdir=* | --mphsdir=* | --mphsdi=* | --mphsd=* | --mphs=*)
    mphsdir="$ac_optarg"; used_mphsdir=1 ;;

  -mquedir | --mquedir | --mquedi | --mqued | --mque)
    ac_prev=mquedir ;;
  -mquedir=* | --mquedir=* | --mquedi=* | --mqued=* | --mque=*)
    mquedir="$ac_optarg"; used_mquedir=1 ;;

dnl old options
  -mmdf-logdir | --mmdf-logdir | --mmdf-logdi | --mmdf-logd | --mmdf-log \
  | --mmdf-lo )
    ac_prev=mlogdir ;;
  -mmdf-logdir=* | --mmdf-logdir=* | --mmdf-logdi=* | --mmdf-logd=* | --mmdf-log=* \
  | --mmdf-lo=* )
    mlogdir="$ac_optarg"; used_mlogdir=1 ;;

  -mmdf-phasedir | --mmdf-phasedir | --mmdf-phasedi | --mmdf-phased | --mmdf-phase | --mmdf-phas \
  | --mmdf-pha | --mmdf-ph)
    ac_prev=mphsdir ;;
  -mmdf-phasedir=* | --mmdf-phasedir=* | --mmdf-phasedi=* | --mmdf-phased=* | --mmdf-phase=* \
  | --mmdf-phas=* | --mmdf-pha=* | --mmdf-ph=*)
    mphsdir="$ac_optarg"; used_mphsdir=1 ;;

  -mmdf-spooldir | --mmdf-spooldir | --mmdf-spooldi | --mmdf-spoold | --mmdf-spool | --mmdf-spoo \
  | --mmdf-spo | --mmdf-sp)
    ac_prev=mquedir ;;
  -mmdf-spooldir=* | --mmdf-spooldir=* | --mmdf-spooldi=* | --mmdf-spoold=* | --mmdf-spool=* \
  | --mmdf-spoo=* | --mmdf-spo=* | --mmdf-sp=*)
    mquedir="$ac_optarg"; used_mquedir=1 ;;
  -mmdf-diallog | --mmdf-diallog | --mmdf-diallo | --mmdf-diall | --mmdf-dial | --mmdf-dia)
    ac_prev=mmdfdiallog ;;
  -mmdf-diallog=* | --mmdf-diallog=* | --mmdf-diallo=* | --mmdf-diall=* | --mmdf-dial=* | --mmdf-dia=*)
    mmdfdiallog="$ac_optarg"; used_mmdfdiallog=1 ;;

  -*) AC_MSG_ERROR([$ac_option: invalid option; use --help to show usage])
    ;;
  esac
dnl
dnl ])
dnl AC_PROVIDE(AC_INIT_PARSE_MMDF_ARGS)

