#!/bin/sh

LOG=World.log
#TARGET="sparc-sun-solaris2.5.1"

OPT="--prefix=/usr"
#OPT=$OPT" --libdir=/opt/mmdf/SRC/lib"
#OPT=$OPT" --includedir=/opt/mmdf/SRC/include"
#OPT=$OPT" --bindir=/opt/local/bin"
#OPT=$OPT" --libexecdir=/opt/mmdf/lib"

#OPT=$OPT" --mmdf-prefix=/opt/mmdf"
#OPT=$OPT" --mlogdir=/var/spool/mmdf/log"
#OPT=$OPT" --mphsdir=/var/spool/mmdf/phase"
#OPT=$OPT" --mquedir=/var/spool/mmdf/lock/home"
# OPT=$OPT" --mchndir="
# OPT=$OPT" --mtbldir="

OPT=$OPT" --mmdf-debug=1"      # 0,1,2
# OPT=$OPT" --mmdf-dlog"
# OPT=$OPT" --mmdf-nodomlit=0"
OPT=$OPT" --enable-leftdots"
# OPT=$OPT" --mmdf-statsort=0"
# OPT=$OPT" --mmdf-citation=1"

OPT=$OPT" --with-mmdflogin=mmdf"
OPT=$OPT" --with-mmdfgroup=mmdf"
# OPT=$OPT" --with-rootlogin="

OPT=$OPT" --with-ldap"

# OPT=$OPT" --disable-nameserver"
# OPT=$OPT" --disable-dbm"
# OPT=$OPT" --disable-wrap"
# OPT=$OPT" --enable-nn"

# OPT=$OPT" --disable-nosrcroute"
# OPT=$OPT" --disable-8bit"
# OPT=$OPT" --enable-lock_files"
# OPT=$OPT" --enable-dial"
#OPT=$OPT" --disable-gcc"
# OPT=$OPT" --disable-fhs"
# OPT=$OPT" --disable-elf"

OPT=$OPT" --enable-esmtp"
OPT=$OPT" --enable-esmtp_dsn"
OPT=$OPT" --enable-esmtp_8bitmime"
# OPT=$OPT" --enable-wildcard"
OPT=$OPT" --enable-rbl"
OPT=$OPT" --enable-filelist"

# OPT=$OPT" --disable-badusers"
# OPT=$OPT" --enable-bboards"   # need MH or NMH
OPT=$OPT" --enable-blockaddr"
OPT=$OPT" --enable-delay"
# OPT=$OPT" --enable-ean"
# OPT=$OPT" --disable-list"
OPT=$OPT" --enable-niftp"
OPT=$OPT" --enable-phone"
OPT=$OPT" --enable-pobox"
# OPT=$OPT" --disable-pop"      # need MH or NMH
OPT=$OPT" --enable-prog"
OPT=$OPT" --enable-smphone"
# OPT=$OPT" --disable-smtp"
# OPT=$OPT" --disable-uucp"

# OPT=$OPT" --disable-msg"
# OPT=$OPT" --disable-other"
# OPT=$OPT" --disable-send"
# OPT=$OPT" --disable-snd"
# OPT=$OPT" --disable-ucbmail"
# OPT=$OPT" --enable-unsupported"
OPT=$OPT" --enable-virtual-domains"

echo ./configure $OPT --target=$TARGET $*
#./configure $OPT --target=$TARGET $*
rm -f $LOG
./configure $OPT $* | tee -a $LOG
echo ========================= make ===================================>>$LOG
make 2>&1 | tee -a $LOG
