#! /bin/sh
#
#		Written by Miquel van Smoorenburg <miquels@cistron.nl>.
#		Modified for Debian 
#		by Ian Murdock <imurdock@gnu.ai.mit.edu>.
#
# Version:	@(#)skeleton  1.9  26-Feb-2001  miquels@cistron.nl
#

PATH=/usr/lib/mmdf/lib:/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DAEMON=/usr/lib/mmdf/lib/deliver
NAME=deliver
DESC=deliver

test -x $DAEMON || exit 0

set -e

get_channels() {
    MMDFTAILOR="/etc/mmdf/mmdftailor"

    MCHN=`expand $MMDFTAILOR | grep "^MCHN " | awk '{print $2}' | sed 's/,//g'`
    CHANNEL=`echo $MCHN | sed 's/ /,/g'`
    echo $CHANNEL
}

case "$1" in
  start)
	echo -n "Starting $DESC: "
	MCHN=`get_channels`
	start-stop-daemon --start --quiet --pidfile /var/run/$NAME.pid \
		--exec "$DAEMON" -- -b -c$MCHN
	echo "$NAME."
	;;
  stop)
	echo -n "Stopping $DESC: "
	start-stop-daemon --stop --quiet --oknodo --pidfile /var/run/$NAME.pid
	echo "$NAME."
	;;
  #reload)
	#
	#	If the daemon can reload its config files on the fly
	#	for example by sending it SIGHUP, do it here.
	#
	#	If the daemon responds to changes in its config file
	#	directly anyway, make this a do-nothing entry.
	#
	# echo "Reloading $DESC configuration files."
	# start-stop-daemon --stop --signal 1 --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
  #;;
  #restart|force-reload)
	#
	#	If the "reload" option is implemented, move the "force-reload"
	#	option to the "reload" entry above. If not, "force-reload" is
	#	just the same as "restart".
	#
	#echo -n "Restarting $DESC: "
	#start-stop-daemon --stop --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
	#sleep 1
	#start-stop-daemon --start --quiet --pidfile \
	#	/var/run/$NAME.pid --exec $DAEMON
	#echo "$NAME."
	#;;
  *)
	N=/etc/init.d/$NAME
	# echo "Usage: $N {start|stop|restart|reload|force-reload}" >&2
	#echo "Usage: $N {start|stop|restart|force-reload}" >&2
	echo "Usage: $N {start|stop}" >&2
	exit 1
	;;
esac

exit 0
