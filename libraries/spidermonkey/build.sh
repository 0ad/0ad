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

JOBS=${JOBS:="-j2"}

MAKE_OPTS="${JOBS}"

CONF_OPTS="--disable-tests"
# (We don't use --enable-threadsafe because we don't use a single runtime in
# multiple threads, so it is unnecessary complexity and performance overhead)

# If Valgrind looks like it's installed, then set up SM to support it
# (else the JITs will interact poorly with it)
if [ -e /usr/include/valgrind/valgrind.h ]
then
  CONF_OPTS="${CONF_OPTS} --enable-valgrind"
fi

#CONF_OPTS="${CONF_OPTS} --enable-threadsafe --with-system-nspr"
#CONF_OPTS="${CONF_OPTS} --enable-trace-jscalls"

# We need to be able to override CHOST in case it is 32bit userland on 64bit kernel
CONF_OPTS="${CONF_OPTS} \
  ${CBUILD:+--build=${CBUILD}} \
  ${CHOST:+--host=${CHOST}} \
  ${CTARGET:+--target=${CTARGET}}"

echo "SpiderMonkey build options: ${CONF_OPTS}"

# Extract the tarball
tar xzf js185-1.0.0.tar.gz

cd js-1.8.5/js/src

# We want separate debug/release versions of the library, so we have to change
# the LIBRARY_NAME for each build.
# (We use perl instead of sed so that it works with MozillaBuild on Windows,
# which has an ancient sed.)
perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs185)(-ps-debug|-ps-release)?/$1-ps-debug/' Makefile.in
mkdir -p build-debug
cd build-debug
../configure ${CONF_OPTS} --enable-debug --disable-optimize
make ${MAKE_OPTS}
cd ..

perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs185)(-ps-debug|-ps-release)?/$1-ps-release/' Makefile.in
mkdir -p build-release
cd build-release
../configure ${CONF_OPTS} # --enable-gczeal --enable-debug-symbols
make ${MAKE_OPTS}
cd ..

# Remove the library suffixes to avoid spurious SVN diffs
perl -i.bak -pe 's/^(LIBRARY_NAME\s+= mozjs185)(-ps-debug|-ps-release)?/$1/' Makefile.in

cd ../../..

if [ "${OS}" = "Windows_NT" ]
then
  INCLUDE_DIR=include-win32
  DLL_SRC_SUFFIX=-1.0.dll
  DLL_DST_SUFFIX=-1.0.dll
  LIB_PREFIX=
  LIB_SRC_SUFFIX=-1.0.lib
  LIB_DST_SUFFIX=.lib
elif [ "`uname -s`" = "Darwin" ]
then
  INCLUDE_DIR=include-unix
  DLL_SRC_SUFFIX=.dylib
  DLL_DST_SUFFIX=.1.0.dylib
  LIB_PREFIX=lib
  LIB_SRC_SUFFIX=.dylib
  LIB_DST_SUFFIX=.dylib
else
  INCLUDE_DIR=include-unix
  DLL_SRC_SUFFIX=.so
  DLL_DST_SUFFIX=.so.1.0
  LIB_PREFIX=lib
  LIB_SRC_SUFFIX=.so
  LIB_DST_SUFFIX=.so
fi

# Copy files into the necessary locations for building and running the game

# js-config.h is the same for both debug and release builds, so we only need to copy one
mkdir -p ${INCLUDE_DIR}/js
cp -L js-1.8.5/js/src/build-release/dist/include/* ${INCLUDE_DIR}/js/

mkdir -p lib/
cp -L js-1.8.5/js/src/build-debug/dist/lib/${LIB_PREFIX}mozjs185-ps-debug${LIB_SRC_SUFFIX} lib/${LIB_PREFIX}mozjs185-ps-debug${LIB_DST_SUFFIX}
cp -L js-1.8.5/js/src/build-release/dist/lib/${LIB_PREFIX}mozjs185-ps-release${LIB_SRC_SUFFIX} lib/${LIB_PREFIX}mozjs185-ps-release${LIB_DST_SUFFIX}
cp -L js-1.8.5/js/src/build-debug/dist/bin/${LIB_PREFIX}mozjs185-ps-debug${DLL_SRC_SUFFIX} ../../binaries/system/${LIB_PREFIX}mozjs185-ps-debug${DLL_DST_SUFFIX}
cp -L js-1.8.5/js/src/build-release/dist/bin/${LIB_PREFIX}mozjs185-ps-release${DLL_SRC_SUFFIX} ../../binaries/system/${LIB_PREFIX}mozjs185-ps-release${DLL_DST_SUFFIX}

# Flag that it's already been built successfully so we can skip it next time
touch .already-built
