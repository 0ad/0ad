#!/bin/sh

# Function that copies a file if it differs from the already "installed" file.
maybecp()
{
	if ! diff -q $1 $2 >/dev/null 2>&1; then
		echo "$2 out of date, copying from $1"
		cp $1 $2
	else
		echo "$2 already up-to-date"
	fi
}

# We want spidermonkey to install its files in dist/*, and the libraries in dist/lib regardless of architecture (by default it makes a lib64 on amd64)
dist="$(pwd)/dist"
lib=lib

# libjs can't be included in a dylib if it has common symbols on Mac OS. This
# breaks linkage for Atlas.
if [ "`uname -s`" == "Darwin" ]; then
	no_common=-fno-common
fi

JS_THREADSAFE=1 \
OTHER_LIBS=`nspr-config --libs` \
CFLAGS="`nspr-config --cflags` $no_common" \
JS_DIST="$dist" \
JS_LIBDIR=$lib \
make -C js/src -f Makefile.ref all export &&

# Now copy the libraries and one auto-generated configuration header into the
# lib- and include-directories expected by pyrogenesis.
outlib=../lib
outinc=../include
maybecp "$dist"/$lib/libjs.a $outlib/libjs.a
maybecp "$dist"/include/jsautocfg.h $outinc/js/jsautocfg.h
