#! /bin/sh 

OPT=""
OPT=$OPT" --create-target"
OPT=$OPT" --mmdf-debug=1"
OPT=$OPT" --mmdf-dlog"
OPT=$OPT" --mmdf-dbglog"

### Features
OPT=$OPT" --enable-wrap --enable-nameserver"
# OPT=$OPT" --enable-nis --enable-nn"
OPT=$OPT" --enable-nosrcroute --enable-8bit"
OPT=$OPT" --enable-mgt_addid --enable-mgt_addipaddr --enable-mgt_addipname"

### Channel programs
OPT=$OPT" --disable-ean --enable-list --disable-niftp --disable-phone"
OPT=$OPT" --disable-bboards --disable-pop"
OPT=$OPT" --enable-pobox "
OPT=$OPT" --enable-prog"
OPT=$OPT" --enable-dial"

###
OPT=$OPT" --enable-fhs"
OPT=$OPT" --enable-esmtp --enable-esmtp_dsn --enable-esmtp_8bitmime"
OPT=$OPT" --enable-filelist"
OPT=$OPT" --enable-rbl"

#OPT=$OPT" --enable-pobox"
#OPT=$OPT" --enable-blockaddr"
#OPT=$OPT" --enable-blockaddr"
#OPT=$OPT" --enable-phone"
#OPT=$OPT" --enable-smphone"
#OPT=$OPT" --enable-"
OPT=$OPT" --enable-ucbmail"


#OPT=$OPT" --enable-esmtp"
#OPT=$OPT" --enable-esmtp_dsn"
#OPT=$OPT" --enable-esmtp_8bitmime"
##OPT=$OPT" --enable-wildcard"
#OPT=$OPT" --enable-rbl"

./configure $OPT $*
