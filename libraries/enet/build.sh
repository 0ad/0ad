#!/bin/sh

set -e

JOBS=${JOBS:="-j2"}

echo "Building libenet..."
echo

cd src/

./configure

make ${JOBS}

cd ../

if [ "`uname -s`" = "Darwin" ]
then
  # TODO: this is untested
  cp src/.libs/libenet.dylib lib/
  cp src/.libs/libenet.dylib ../../binaries/system/
else
  cp src/.libs/libenet.so lib/
  cp src/.libs/libenet.so.1 ../../binaries/system/
fi
