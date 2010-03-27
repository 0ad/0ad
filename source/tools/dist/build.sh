#!/bin/bash

set -ev

SVNWC=~/0ad/public-trunk/
SVNREV=`svnversion -n ${SVNWC}`
PREFIX=0ad-r0${SVNREV}-pre-alpha

SEVENZOPTS="-mmt -mx=9"
XZOPTS="-9"
BZ2OPTS="-9"

# Export files with appropriate line-endings
rm -rf export-unix
rm -rf export-win32
svn export ${SVNWC} export-unix
svn export --native-eol CRLF ${SVNWC} export-win32

# Update the svn_revision, so these builds can be identified
echo L\"${SVNREV}-release\" > export-unix/build/svn_revision/svn_revision.txt
echo L\"${SVNREV}-release\" > export-win32/build/svn_revision/svn_revision.txt

# Collect the relevant files
ln -Tsf export-unix ${PREFIX}
tar cf $PREFIX-unix-build-temp.tar ${PREFIX}/{source,build,libraries/{cxxtest,fcollada,spidermonkey-tip,valgrind},binaries/system/readme.txt,*.txt}
tar cf $PREFIX-unix-data-temp.tar ${PREFIX}/binaries/data
# TODO: ought to include generated docs in here, perhaps?

# Anonymise the tarballs a bit
tardy -gna wfg -gnu 1000 -una wfg -unu 1000 $PREFIX-unix-build-temp.tar $PREFIX-unix-build.tar
tardy -gna wfg -gnu 1000 -una wfg -unu 1000 $PREFIX-unix-data-temp.tar $PREFIX-unix-data.tar

# Compress
xz -kv ${XZOPTS} $PREFIX-unix-build.tar
xz -kv ${XZOPTS} $PREFIX-unix-data.tar
bzip2 -kp ${BZ2OPTS} $PREFIX-unix-build.tar
bzip2 -kp ${BZ2OPTS} $PREFIX-unix-data.tar

# Create Windows self-extracting .exe
ln -Tsf export-win32 ${PREFIX}
wine 'c:/program files/7-zip/7zG' a -sfx $PREFIX-win32.exe ${SEVENZOPTS} ${PREFIX}/{source,build,libraries,binaries/{data,system},*.*}

# Print digests for copying into wiki page
sha1sum ${PREFIX}-{unix-{build,data}.tar.{xz,bz2},win32.exe}
