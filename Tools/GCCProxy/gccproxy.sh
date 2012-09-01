#!/bin/bash

# Get invocation path (which could be a symlink in $PATH)
fullpath=`which "$0"`
if [[ !$? ]]; then
	fullpath="$0"
fi

# Resolve symlink
fullpath=`readlink -f "$fullpath"`

# Get base directory
BASEDIR=`dirname "$fullpath"`

cfgfile=`mktemp`
make --no-print-directory -f $BASEDIR/getconfig.mk ARCH=x86 > $cfgfile
#echo $cfgfile
#cat $cfgfile
. $cfgfile
rm $cfgfile

_miscargs=""
_compile=0

while [[ $# -gt 0 ]]; do
	case "$1" in
	-c)
		_compile=1
		;;
	-o)
		shift
		_outfile="-o $1"
		;;
	-I)
		shift
		_cflags=$_cflags" -I$1"
		;;
	-I*|-D*|-O*)
		_cflags=$_cflags" $1"
		;;
	-Wl,*)
		arg=$1
		arg=${arg#-Wl}
		arg=${arg/,/ }
		_ldflags=$_ldflags" ${arg}"
		;;
	-l)
		shift
		_libs=$_libs" -l$1"
		;;
	-l*)
		_libs=$_libs" $1"
		;;
	*)
		_miscargs=$_miscargs" $1"
		;;
	esac
	shift
done

if [[ $_compile -eq 1 ]]; then
#	echo $_CC $CFLAGS $*
	$_CC $CFLAGS $_miscargs -c $_outfile
else if echo " $_miscargs" | grep '\.c' >/dev/null; then
	tmpout=`mktemp acess_gccproxy.XXXXXXXXXX.o --tmpdir`
	$_CC $CFLAGS $_miscargs -c -o $tmpout
	$_LD $LDFLAGS $_ldflags $_libs $tmpout $_outfile
	rm $tmpout
else
	$_LD $LDFLAGS $_ldflags $_miscargs $_outfile $LIBGCC_PATH
fi; fi

