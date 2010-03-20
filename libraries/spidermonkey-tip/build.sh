#!/bin/sh

set -e

# Since this script is called by update-workspaces.sh, we want to quickly
# avoid doing any work if SpiderMonkey is already built and up-to-date.
# Running SM's Makefile is a bit slow and noisy, so instead we'll make a
# special file and only rebuild if it's older than SVN.
# README.txt should be updated whenever we update SM, so use that as
# a time comparison.
if [ -e .already-built -a .already-built -nt README.txt ]
then
    echo "SpiderMonkey is already up to date"
    exit
fi

echo "Building SpiderMonkey..."
echo

MAKE_OPTS="-j2"

CONF_OPTS="--disable-tests"
# Potentially useful flags:
#  --enable-valgrind
#  --enable-callgrind
#  --enable-tracevis
#
# (We don't use --enable-threadsafe because we don't use a single runtime in
# multiple threads, so it is unnecessary complexity and performance overhead)

cd src

# autoconf-2.13   # this generates ./configure, which we've added to SVN instead

# We want separate debug/release versions of the library, so we have to change
# the LIBRARY_NAME for each build.
# (We use perl instead of sed so that it works with MozillaBuild on Windows,
# which has an ancient sed.)
perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs).*/$1-ps-debug/' Makefile.in
mkdir -p build-debug
cd build-debug
../configure ${CONF_OPTS} --enable-debug --disable-optimize
make ${MAKE_OPTS}
cd ..

perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs).*/$1-ps-release/' Makefile.in
mkdir -p build-release
cd build-release
../configure ${CONF_OPTS}
make ${MAKE_OPTS}
cd ..

# Remove the library suffixes to avoid spurious SVN diffs
perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs).*/$1/' Makefile.in

cd ..

if [ "${OS}" = "Windows_NT" ]
then
  INCLUDE_DIR=include-win32
  DLL_EXTN=dll
  LIB_EXTN=lib
  LIB_PREFIX=
elif [ "`uname -s`" = "Darwin" ]
then
  INCLUDE_DIR=include-unix
  DLL_EXTN=dylib
  LIB_EXTN=dylib
  LIB_PREFIX=lib
else
  INCLUDE_DIR=include-unix
  DLL_EXTN=so
  LIB_EXTN=so
  LIB_PREFIX=lib
fi

# Copy files into the necessary locations for building and running the game
mkdir -p ${INCLUDE_DIR}/debug/js
mkdir -p ${INCLUDE_DIR}/release/js
mkdir -p lib/
cp -L src/build-debug/dist/include/* ${INCLUDE_DIR}/debug/js/
cp -L src/build-release/dist/include/* ${INCLUDE_DIR}/release/js/
cp -L src/build-debug/dist/lib/${LIB_PREFIX}mozjs-ps-debug.${LIB_EXTN} lib/
cp -L src/build-release/dist/lib/${LIB_PREFIX}mozjs-ps-release.${LIB_EXTN} lib/
cp -L src/build-debug/dist/bin/${LIB_PREFIX}mozjs-ps-debug.${DLL_EXTN} ../../binaries/system/
cp -L src/build-release/dist/bin/${LIB_PREFIX}mozjs-ps-release.${DLL_EXTN} ../../binaries/system/

# Flag that it's already been built successfully so we can skip it next time
touch .already-built
