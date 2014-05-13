#!/bin/bash

set -ev

# Compiled executable for archive-builder tool
EXE=/mnt/0ad/0ad/binaries/system/pyrogenesis

# Location of clean checkout
SVNWC=/mnt/0ad/0ad/

SVNREV=`svnversion -n ${SVNWC}`
PREFIX=0ad-0.0.XXX-alpha

XZOPTS="-9 -e"
BZ2OPTS="-9"
GZIPOPTS="-9"
GZIP7ZOPTS="-mx=9"

# Export files with appropriate line-endings
rm -rf export-unix
rm -rf export-win32
svn export ${SVNWC} export-unix
svn export --native-eol CRLF ${SVNWC} export-win32

# Only include translations for a subset of languages
find export-{unix,win32}/binaries/data/ -name "*.po" | grep -v '.*/\(ca\|cs\|de\|en_GB\|es\|fr\|gd\|gl\|it\|nl\|pt_PT\|pt_BR\)\.[A-Za-z0-9_.]\+\.po' | xargs rm

# Update the svn_revision, so these builds can be identified
echo L\"${SVNREV}-release\" > export-unix/build/svn_revision/svn_revision.txt
echo L\"${SVNREV}-release\" > export-win32/build/svn_revision/svn_revision.txt

# Package the mod data
# (The platforms differ only in line endings, so just do the Unix one instead of
# generating two needlessly inconsistent packages)
${EXE} -archivebuild=export-unix/binaries/data/mods/public -archivebuild-output=export-unix/binaries/data/mods/public/public.zip
cp export-unix/binaries/data/mods/public/public.zip export-win32/binaries/data/mods/public/public.zip

# Collect the relevant files
ln -Tsf export-unix ${PREFIX}
tar cf $PREFIX-unix-build.tar \
	--exclude='*.bat' --exclude='*.dll' --exclude='*.exe' --exclude='*.lib' \
	--exclude='libraries/source/fcollada/src/FCollada/FColladaTest' \
	--exclude='libraries/source/spidermonkey/include-win32-*' \
	${PREFIX}/{source,build,libraries/source,binaries/system/readme.txt,binaries/data/l10n,binaries/data/tests,binaries/data/mods/_test.*,*.txt}
tar cf $PREFIX-unix-data.tar \
	--exclude='binaries/data/config/dev.cfg' \
	 ${PREFIX}/binaries/data/{config,mods/public/public.zip,tools}
# TODO: ought to include generated docs in here, perhaps?

# Compress
xz -kv ${XZOPTS} $PREFIX-unix-build.tar
xz -kv ${XZOPTS} $PREFIX-unix-data.tar
7z a ${GZIP7ZOPTS} $PREFIX-unix-build.tar.gz $PREFIX-unix-build.tar
7z a ${GZIP7ZOPTS} $PREFIX-unix-data.tar.gz $PREFIX-unix-data.tar

# Create Windows installer
makensis -nocd -dcheckoutpath=export-win32 -drevision=${SVNREV} -dprefix=${PREFIX} export-win32/source/tools/dist/0ad.nsi

# Fix permissions
chmod -f 644 ${PREFIX}-{unix-{build,data}.tar.{xz,bz2,gz},win32.exe}

# Print digests for copying into wiki page
sha1sum ${PREFIX}-{unix-{build,data}.tar.{xz,bz2,gz},win32.exe}
