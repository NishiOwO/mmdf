#!/bin/sh

TARGET="sparc-sun-solaris2.5.1"

OPT="--prefix=/opt"
OPT=$OPT" --libdir=/opt/mmdf/SRC/lib"
OPT=$OPT" --includedir=/opt/mmdf/SRC/include"
OPT=$OPT" --bindir=/opt/local/bin"
OPT=$OPT" --libexecdir=/opt/mmdf/lib"

OPT=$OPT" --mmdf-prefix=/opt/mmdf"
OPT=$OPT" --mlogdir=/var/spool/mmdf/log"
OPT=$OPT" --mphsdir=/var/spool/mmdf/phase"
OPT=$OPT" --mquedir=/var/spool/mmdf/lock/home"
# OPT=$OPT" --mchndir="
# OPT=$OPT" --mtbldir="

OPT=$OPT" --mmdf-debug=1"      # 0,1,2
# OPT=$OPT" --mmdf-dlog"
# OPT=$OPT" --mmdf-nodomlit=0"
# OPT=$OPT" --mmdf-leftdots=1"
# OPT=$OPT" --mmdf-statsort=0"
# OPT=$OPT" --mmdf-citation=1"

# OPT=$OPT" --with-mmdflogin="
# OPT=$OPT" --with-mmdfgroup="
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
OPT=$OPT" --disable-gcc"
OPT=$OPT" --disable-fhs"
# OPT=$OPT" --disable-elf"

OPT=$OPT" --enable-esmtp"
# OPT=$OPT" --enable-esmtp_dsn"
# OPT=$OPT" --enable-esmtp_8bitmime"
# OPT=$OPT" --enable-wildcard"
OPT=$OPT" --enable-rbl"
# OPT=$OPT" --enable-filelist"

# OPT=$OPT" --disable-badusers"
# OPT=$OPT" --enable-bboards"
# OPT=$OPT" --enable-blockaddr"
# OPT=$OPT" --enable-delay"
# OPT=$OPT" --enable-ean"
# OPT=$OPT" --disable-list"
# OPT=$OPT" --enable-niftp"
# OPT=$OPT" --enable-phone"
# OPT=$OPT" --enable-pobox"
# OPT=$OPT" --disable-pop"
# OPT=$OPT" --enable-prog"
# OPT=$OPT" --enable-smphone"
# OPT=$OPT" --disable-smtp"
# OPT=$OPT" --disable-uucp"

# OPT=$OPT" --disable-msg"
# OPT=$OPT" --disable-other"
# OPT=$OPT" --disable-send"
# OPT=$OPT" --disable-snd"
# OPT=$OPT" --disable-ucbmail"
# OPT=$OPT" --enable-unsupported"

./configure $OPT --target=$TARGET $*
