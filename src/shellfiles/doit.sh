#! /bin/sh
PATH=.:/bin:/usr/bin
export PATH
umask 002		# vital! -Mike
# to do everything, run this file.
echo "Processing new MMDF tables for `hostname`"
/bin/sh makearpa
/bin/sh makemailids
/bin/sh makealias
dbmbuild -Onv
