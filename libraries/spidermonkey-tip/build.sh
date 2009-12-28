#!/bin/sh

set -e

CONF_OPTS="--enable-valgrind --enable-threadsafe --with-system-nspr --disable-tests"

cd src

# autoconf-2.13   # this generates ./configure, which we've added to SVN instead

# We want separate debug/release versions of the library, so we have to change
# the LIBRARY_NAME for each build
sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1-debug/' Makefile.in
mkdir -p build-debug
cd build-debug
../configure ${CONF_OPTS} --enable-debug --disable-optimize
make -j2
cd ..

sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1-release/' Makefile.in
mkdir -p build-release
cd build-release
../configure ${CONF_OPTS}
make -j2
cd ..

# Remove the library suffixes to avoid spurious SVN diffs
sed -i -r 's/^(LIBRARY_NAME\s+= mozjs).*/\1/' Makefile.in

cd ..

mkdir -p include/{debug,release}/js
mkdir -p lib/
cp -uL src/build-debug/dist/include/* include/debug/js/
cp -uL src/build-release/dist/include/* include/release/js/
cp -L src/build-debug/dist/lib/libmozjs-debug.so lib/
cp -L src/build-release/dist/lib/libmozjs-release.so lib/

cd ../..

cp libraries/spidermonkey-tip/lib/libmozjs-{debug,release}.so binaries/system/
