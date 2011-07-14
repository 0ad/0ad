#!/bin/sh

set -e

JOBS=${JOBS:="-j2"}

echo "Building libenet..."
echo

cd src/

./configure

make ${JOBS}

cd ../

mkdir -p lib/

if [ "`uname -s`" = "Darwin" ]
then
  # Fix libtool's use of an absolute path
  install_name_tool -id @executable_path/libenet.1.dylib src/.libs/libenet.1.dylib
  cp src/.libs/libenet.dylib lib/
  cp src/.libs/libenet.1.dylib ../../binaries/system/
else
  cp src/.libs/libenet.so lib/
  cp src/.libs/libenet.so.1 ../../binaries/system/
fi
