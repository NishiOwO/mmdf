#! /bin/sh
#
# Create domain and channel tables from the pathalias database
#
# Mark Vasoll
# Computing and Information Sciences   Internet:  vasoll@a.cs.okstate.edu
# Oklahoma State University            UUCP:  {cbosgd, ihnp4, rutgers, seismo,
# Stillwater, Oklahoma                         uiucdcs}!okstate!vasoll
#
PATHS=/tmp/t1$$
DOMAINS=/tmp/t2$$
NODOM=/tmp/t3$$
SUMMARY=/tmp/t4$$
#
MAPFILE=../../Sys/okstate.map
VALIDDOMS="edu gov mil com org net arpa au ch de fi fr il jp kr nl nz se uk us"
#
#  create pathalias output, separate into domain and non-domain entries
#
pathalias -v $MAPFILE d.* u.* >$PATHS
egrep '[^ 	]*\.[^ 	]*	' <$PATHS | sed 's/^\.//' | sort -fu >$DOMAINS
egrep -v '[^ 	]*\.[^ 	]*	' <$PATHS >$NODOM
#
#  generate some interesting stats
#
awk '{split($2, arr, "!"); print arr[1];}' <$PATHS >$SUMMARY
echo count neighbor
sort <$SUMMARY | uniq -c
#
#  clobber old output files
#
rm mmdf.topdom mmdf.uucpdom mmdf.uucpchan /tmp/wk$$ 2>/dev/null
#
#  use only those entries that contain valid top-levels
#
for i in $VALIDDOMS
do
	egrep -i "\.$i[ 	]" $DOMAINS >>/tmp/wk$$
done
#
#  generate a "fake" .UUCP domain table for "non-domain" UUCP sites
#  to belong to.  also make a suplimental "top-domain" table that
#  includes the domains from pathalias.  also make the UUCP channel
#  table.
awk '{printf "%s:%s.UUCP\n", $1, $1;}' $NODOM >mmdf.uucpdom
awk '{printf "%s.UUCP:%s\n", $1, $2;}' $NODOM >mmdf.uucpchan
awk '{printf "%s:%s\n", $1, $1;}' /tmp/wk$$ >mmdf.topdom
awk '{printf "%s:%s\n", $1, $2;}' /tmp/wk$$ >>mmdf.uucpchan
rm $PATHS $DOMAINS $NODOM $SUMMARY /tmp/wk$$
