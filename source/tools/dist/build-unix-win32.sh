#!/bin/bash
set -ev

XZOPTS="-9 -e"
GZIP7ZOPTS="-mx=9"

BUNDLE_VERSION=${BUNDLE_VERSION:="0.0.xxx"}
PREFIX="0ad-${BUNDLE_VERSION}-alpha"

SVN_REV=$(svnversion -n .)
echo "${SVN_REV}-release" > build/svn_revision/svn_revision.txt

# Collect the relevant files
tar cf $PREFIX-unix-build.tar \
	--exclude='*.bat' --exclude='*.dll' --exclude='*.exe' --exclude='*.lib' \
	--exclude='libraries/source/fcollada/src/FCollada/FColladaTest' \
	--exclude='libraries/source/spidermonkey/include-*' \
	--exclude='libraries/source/spidermonkey/lib*' \
	{source,build,libraries/source,binaries/system/readme.txt,binaries/data/l10n,binaries/data/tests,binaries/data/mods/_test.*,*.txt}

tar cf $PREFIX-unix-data.tar \
	--exclude='binaries/data/config/dev.cfg' \
	 -s "|archives|binaries/data/mods|" \
	 binaries/data/{config,tools} archives/
# TODO: ought to include generated docs in here, perhaps?

# Compress
xz -kv ${XZOPTS} $PREFIX-unix-build.tar
xz -kv ${XZOPTS} $PREFIX-unix-data.tar
7z a ${GZIP7ZOPTS} $PREFIX-unix-build.tar.gz $PREFIX-unix-build.tar
7z a ${GZIP7ZOPTS} $PREFIX-unix-data.tar.gz $PREFIX-unix-data.tar

# Create Windows installer
# This needs nsisbi for files > 2GB
makensis -V4 -nocd \
	-dcheckoutpath="." \
	-drevision=${SVN_REV} \
	-dprefix=${PREFIX} \
	-darchive_path="archives/" \
	source/tools/dist/0ad.nsi

# Fix permissions
chmod -f 644 ${PREFIX}-{unix-{build,data}.tar.{xz,gz},win32.exe}

# Print digests for copying into wiki page
shasum -a 1 ${PREFIX}-{unix-{build,data}.tar.{xz,gz},win32.exe}
