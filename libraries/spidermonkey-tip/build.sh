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
# the LIBRARY_NAME for each build
sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1-ps-debug/' Makefile.in
mkdir -p build-debug
cd build-debug
../configure ${CONF_OPTS} --enable-debug --disable-optimize
make ${MAKE_OPTS}
cd ..

sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1-ps-release/' Makefile.in
mkdir -p build-release
cd build-release
../configure ${CONF_OPTS}
make ${MAKE_OPTS}
cd ..

# Remove the library suffixes to avoid spurious SVN diffs
sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1/' Makefile.in

cd ..

# Copy files into the necessary locations for building the game
mkdir -p include/debug/js
mkdir -p include/release/js
mkdir -p lib/
cp -uL src/build-debug/dist/include/* include/debug/js/
cp -uL src/build-release/dist/include/* include/release/js/
cp -L src/build-debug/dist/lib/libmozjs-ps-debug.so lib/
cp -L src/build-release/dist/lib/libmozjs-ps-release.so lib/

cd ../..

# Copy files into the system directory for running the game
cp libraries/spidermonkey-tip/lib/libmozjs-ps-debug.so binaries/system/
cp libraries/spidermonkey-tip/lib/libmozjs-ps-release.so binaries/system/

# Flag that it's already been built successfully so we can skip it next time
touch libraries/spidermonkey-tip/.already-built
