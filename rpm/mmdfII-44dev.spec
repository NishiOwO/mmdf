%define version_major II
%define version_minor 45
%define patch_level b7
%define name mmdf
%define release 20001007

Summary: MMDF
Name: %{name}
Version: %{version_major}.%{version_minor}.%{release}
Serial: %{release}
Group: Network/Mail
Source0: ftp://www.mathematik.uni-kl.de/pub/Source/mail+news/mmdf/mmdf-2.44dev-%{release}.tar.gz
Source1: setlogs
Source2: mmdf.init
# Source3: mmdfII-4.3-doc.tgz
Source4: mmdftailor
Source5: local-configure.sh
Release: 3
URL: http://www.mmdf.org/
#URL: http://www.mathematik.uni-kl.de/ftp/pub/Sources/mail+news/mmdf/
Copyright: BSD
Vendor: FB Mathematik, Uni Kaiserslautern
Packager: Kai Krueger, krueger@mathematik.uni-kl.de
BuildRoot: /var/tmp/Buildroot/%{name}-%{version_major}.%{version_minor}
Provides: sendmail
Provides: smtpdaemon

%description
The  Multi-channel  Memorandum   Distribution Facility (MMDF) is a powerful 
and flexible Message Handling System for the  UNIX operating 
system. It  is a message transport system designed to handle large numbers 
of messages in a robust and efficient manner. MMDF's modular design allows
flexible choice of User Interfaces, and direct connection to several 
different message transfer protocols.
Features: * Prepared for UUCP (uid=11).
          * 8bit clean
          * long header line (1024 bytes)
          * suppport baduser delivery to postmaster with notification to sender
          * Rewrite Received: field in header
	  * MTBL supports nis queries
          * compiled with tcp_wrapper 7.4 within smtpsrvr

%package clients
Summary: MMDF, header=1023, 8bit-clean, clients
Group: Network/Mail
#Requires: mmdf.II >=44
%description clients
The  Multi-channel  Memorandum   Distribution Facility (MMDF) is a powerful 
and flexible Message Handling System for the  UNIX operating 
system. It  is a message transport system designed to handle large numbers 
of messages in a robust and efficient manner. MMDF's modular design allows
flexible choice of User Interfaces, and direct connection to several 
different message transfer protocols.

%package devel
Summary: MMDF, header=1023, 8bit-clean, devel
Group: Network/Mail
%description devel
The  Multi-channel  Memorandum   Distribution Facility (MMDF) is a powerful 
and flexible Message Handling System for the  UNIX operating 
system. It  is a message transport system designed to handle large numbers 
of messages in a robust and efficient manner. MMDF's modular design allows
flexible choice of User Interfaces, and direct connection to several 
different message transfer protocols.


%prep
# NOTE:
#%setup -n v%{version_minor}%{patch_level}/devsrc
%setup -n devsrc
set
cp ${RPM_SOURCE_DIR}/local-configure.sh .
chmod 755 install.sh
#cvs -d :pserver:nobody@pingpong:/home/soft/cvsroot mmdf
#rm -rf CVS

./local-configure.sh

%build
echo $PATH
#make depend
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/bin
mkdir -p $RPM_BUILD_ROOT/usr/man/man1
mkdir -p $RPM_BUILD_ROOT/usr/man/man3
mkdir -p $RPM_BUILD_ROOT/usr/man/man5
mkdir -p $RPM_BUILD_ROOT/usr/man/man7
mkdir -p $RPM_BUILD_ROOT/usr/man/man8
mkdir -p $RPM_BUILD_ROOT/usr/include/mmdf
make install_prefix=$RPM_BUILD_ROOT dirs
make install_prefix=$RPM_BUILD_ROOT MMDFLOGIN=root MMDFGROUP=root install
rmdir $RPM_BUILD_ROOT/var/spool/mmdf/home

(cd src/uucp; install -s -c xrmail $RPM_BUILD_ROOT/usr/bin/rmail)
#%ifarch i386
#(cd uip/other; install -s -c -m4755 -ommdf -g mail xsendmail $RPM_BUILD_ROOT/usr/lib/sendmail)
#ln -s /usr/lib/sendmail $RPM_BUILD_ROOT/usr/sbin/sendmail
#ln -s /usr/sbin/sendmail $RPM_BUILD_ROOT/usr/lib/sendmail
#%endif

strip $RPM_BUILD_ROOT/usr/lib/mmdf/lib/*
strip $RPM_BUILD_ROOT/usr/lib/mmdf/chans/*
strip $RPM_BUILD_ROOT/usr/sbin/sendmail
install -c $RPM_SOURCE_DIR/setlogs $RPM_BUILD_ROOT/usr/lib/mmdf/lib/setlogs
install -c $RPM_SOURCE_DIR/mmdftailor $RPM_BUILD_ROOT/etc/mmdf/mmdftailor
install -c -m 444 lib/libmmdf.a $RPM_BUILD_ROOT/usr/lib/libmmdf.a

cd h
for i in pathnames.h config.h conf.h util.h mmdf.h
do
    install -c -m444 -oroot -groot $i $RPM_BUILD_ROOT/usr/include/mmdf/
done

%ifarch i386
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc0.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc1.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc2.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc3.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc5.d
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/rc6.d
install -m755 -g root -o root $RPM_SOURCE_DIR/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/init.d
cd $RPM_BUILD_ROOT/etc/rc.d/init.d
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K30mmdf
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc1.d/K30mmdf
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc2.d/S60mmdf
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S80mmdf
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S80mmdf
ln -sf ../init.d/mmdf.init $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K30mmdf
%endif

%pre
if [ `grep mmdf /etc/passwd | wc -l` = 0 ]; then
   /usr/sbin/useradd -M -r -d /usr/lib/mmdf -s /bin/bash \
                     -c "MMDF - Mail Transport Agent" mmdf || :
fi
if [ `grep mmdf /etc/group | wc -l` = 0 ]; then
   /usr/sbin/groupadd -r mmdf || :
fi
if [ -f /var/lock/subsys/mmdf ]; then
   /etc/rc.d/init.d/mmdf.init stop
fi

%post
if [ `grep smtpsrvr /etc/inetd.conf | wc -l` = 0 ]; then
   echo "Adding smtpsrvr to /etc/inetd.conf"
   echo "#smtp    stream  tcp     nowait  mmdf    /usr/lib/mmdf/chans/smtpd  smtpd -i /usr/lib/mmdf/chans/smtpsrvr smtp" >> /etc/inetd.conf
fi
echo "creating crontab for MMDF"
cat | su - mmdf -c "crontab -" <<  EOF
00 01 * * 0 /usr/lib/mmdf/lib/setlogs
30 00 * * * /usr/lib/mmdf/lib/cleanque
EOF
su - mmdf -c "crontab -l"


%preun
su - mmdf -c "crontab -r"
echo remove smtpsrvr from /etc/inetd.conf

%postun

%files
%attr(0755,mmdf,mmdf) %dir /etc/mmdf
%attr(0755,mmdf,mmdf) %dir /usr/lib/mmdf
%attr(0700,mmdf,mmdf) %dir /var/spool/mmdf
%attr(0711,mmdf,mmdf) %dir /var/state/mmdf
%attr(0711,mmdf,mmdf) %dir /var/log/mmdf
%attr(0700,mmdf,mmdf) %dir /usr/lib/mmdf/chans
%attr(0755,mmdf,mmdf) %dir /usr/lib/mmdf/lib
%attr(0644,mmdf,mmdf) %config /etc/mmdf/mmdftailor
%attr(0550,mmdf,mmdf) %config /usr/lib/mmdf/lib/setlogs
%doc doc/*
%doc samples
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/blockaddr
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/delay
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/list
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/pobox
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/recvprog
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/sendprog
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/smtp
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/smtpsrvr
%attr(-,mmdf,mmdf) /usr/lib/mmdf/chans/uucp
/usr/lib/mmdf/chans/badusers
/usr/lib/mmdf/chans/local
/usr/lib/mmdf/chans/smtpd
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/amp
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/checkaddr
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/checkup
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/checkque
%attr(-,root,mmdf) /usr/lib/mmdf/lib/cleanque
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/dbmbuild
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/dbmedit
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/dbmlist
%attr(-,root,mmdf) /usr/lib/mmdf/lib/deliver
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/fixtai
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/mailid
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/malias
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/newssend
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/nictable
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/process-uucp
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/rem_chans
#/usr/lib/mmdf/lib/ean2mmdf
#/usr/lib/mmdf/lib/ni_niftp
#/usr/lib/mmdf/lib/slave
#/usr/lib/mmdf/lib/smslave
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/setup
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/submit
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/tablelist
%attr(-,mmdf,mmdf) /usr/lib/mmdf/lib/getvar
#
#
%attr(4755,mmdf,mmdf) /usr/bin/rmail
%attr(-,mmdf,mmdf) /bin/rmail
%attr(-,mmdf,mmdf) /usr/bin/v6mail
%attr(-,mmdf,mmdf) /usr/lib/sendmail
%attr(-,mmdf,mmdf) /usr/sbin/sendmail
#
/usr/man/man1/malias.1
/usr/man/man1/v6mail.1
/usr/man/man3
/usr/man/man5
/usr/man/man7
/usr/man/man8
%ifarch i386
%config /etc/rc.d/init.d/mmdf.init
%config /etc/rc.d/rc0.d/K30mmdf
%config /etc/rc.d/rc1.d/K30mmdf
%config /etc/rc.d/rc2.d/S60mmdf
%config /etc/rc.d/rc3.d/S80mmdf
%config /etc/rc.d/rc5.d/S80mmdf
%config /etc/rc.d/rc6.d/K30mmdf
%endif


%files clients
%attr(-,mmdf,mmdf) /usr/bin/checkmail
%attr(-,mmdf,mmdf) /usr/bin/emactovi
%attr(-,mmdf,mmdf) /usr/bin/mlist
%attr(-,mmdf,mmdf) /usr/bin/msg
#%attr(-,mmdf,mmdf) /usr/bin/newmail
%attr(-,mmdf,mmdf) /usr/bin/resend
%attr(-,mmdf,mmdf) /usr/bin/send
%attr(-,mmdf,mmdf) /usr/bin/snd
%attr(-,mmdf,mmdf) /usr/lib/mh/rcvalert
%attr(-,mmdf,mmdf) /usr/lib/mh/rcvfile
%attr(-,mmdf,mmdf) /usr/lib/mh/rcvprint
%attr(-,mmdf,mmdf) /usr/lib/mh/rcvtrip
/usr/man/man1/msg.1
/usr/man/man1/send.1
/usr/man/man1/snd.1
%ifarch i386
/usr/man/man1/checkmail.1
/usr/man/man1/emactovi.1
/usr/man/man1/rcvalert.1
%endif
/usr/man/man1/rcvfile.1
/usr/man/man1/rcvprint.1
/usr/man/man1/rcvtrip.1
/usr/man/man1/resend.1
/usr/man/man1/mlist.1
#/usr/man/man1/newmail.1

%files devel
%attr(0444,-,-) /usr/lib/libmmdf.a
%attr(0755,-,-) %dir /usr/include/mmdf
%attr(0444,-,-) /usr/include/mmdf/conf.h
%attr(0444,-,-) /usr/include/mmdf/config.h
%attr(0444,-,-) /usr/include/mmdf/mmdf.h
%attr(0444,-,-) /usr/include/mmdf/pathnames.h
%attr(0444,-,-) /usr/include/mmdf/util.h
