#!/bin/sh
if test ! -r alias.local
then
	echo "Cannot read 'alias.local'"
	exit 1
elif test ! -r alias.global
then
	echo "Cannot read 'alias.global'"
	exit 1
fi

trap 'rm -f /tmp/$$.* newalias 2> /dev/null; echo --Aborted--; exit' 1 2 3 15
set -u
umask 077
LOCALHOST=`hostname`
echo "Building alias file for host '${LOCALHOST}'"

#
#  Put editor commands to convert YOUR hostname into the canonical form
#
cat > /tmp/$$.sed <<DONE
s/[ 	]*#.*//
/^$/d
s/@BRL-VGR/@brl-vgr/
s/@Brl-Vgr/@brl-vgr/
s/@VGR/@brl-vgr/
s/@Vgr/@brl-vgr/
s/@vgr/@brl-vgr/
s/@/ @/
DONE

cat > /tmp/$$.awk <<DONE
{
	if( \$3 == "" || \$3 == "@$LOCALHOST" ) {
		if( \$1 != \$2 )
			printf( "%s\t%s%s\n", \$1, \$2, \$3 )
	} else
		printf( "%s\t%s%s\n", \$1, \$2, \$3 )
}
DONE

echo -n "Processing alias.local"
sed -f /tmp/$$.sed alias.local | awk -f /tmp/$$.awk > newalias

echo -n " and alias.global"
sed -f /tmp/$$.sed alias.global | awk -f /tmp/$$.awk >> newalias

echo "."
rm /tmp/$$.*
chmod 644 newalias
mv -f aliases aliases.bak
mv -f newalias aliases
echo "New aliases file built"
