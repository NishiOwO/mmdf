#! /bin/sh
#
#	'@(#)installit	2.4	10/15/87'
#	From install.sh	4.8	(Berkeley)	3/6/86
#	on the 4.3 BSD distribution (with permission)
#
PATH=/bin:/etc:/usr/bin:/usr/ucb
export PATH
cmd=""
stripbefore=""
stripafter=""
chmod="chmod 755"
chown=":"
chgrp=":"
while true ; do
	case $1 in
		-s )	if test $cmd 
			then	stripafter="strip"
			else	stripbefore="strip"
			fi
			shift
			;;
		-c )	if test $cmd 
			then	echo "install: multiple specifications of -c"
				exit 1
			fi
			cmd="cp"
			stripafter=$stripbefore
			stripbefore=""
			shift
			;;
		-m )	chmod="chmod $2"
			shift
			shift
			;;
		-o )	chown="chown $2"
			shift
			shift
			;;
		-g )	chgrp="chgrp $2"
			shift
			shift
			;;
		* )	break
			;;
	esac
done
if test $cmd 
then true
else cmd="mv"
fi

if test ! ${2-""} 
then	echo "install: no destination specified"
	exit 1
fi
if test ${3-""} 
then	echo "install: too many files specified -> $*"
	exit 1
fi
if test $1 = $2 -o $2 = . 
then	echo "install: can't move $1 onto itself"
	exit 1
fi
if test '!' -f $1 
then	echo "install: can't open $1"
	exit 1
fi
if test -d $2 
then	file=$2/`basename $1`
else	file=$2
fi
/bin/rm -f $file
if test $stripbefore 
then	$stripbefore $1
fi
$cmd $1 $file
if test $stripafter 
then	$stripafter $file
fi
$chown $file
$chgrp $file
$chmod $file
exit 0
