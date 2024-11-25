#
#
#

M4	= m4
srcdir	= ..
SRC1	= acgeneral.m4 acoldnames.m4 acspecific.m4 aclocal.m4
SRC2	= acconfig.h

#all: autoconf.m4f autoheader.m4f configure
all: autoconf.m4f configure


autoconf.m4f: autoconf.m4 ${SRC1}
	$(M4) -F $@ $<

autoheader.m4f: autoheader.m4 ${SRC2}
	$(M4) -F $@ $<

configure: ${srcdir}/configure autoconf.m4f
${srcdir}/configure:    ${srcdir}/configure.ac autoconf.m4f
	(cd ${srcdir} && autoconf -I autoconf)


clean:
	rm -f autoconf.m4f autoheader.m4f

autoconf.m4: ${SRC}
