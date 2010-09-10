#!/bin/sh

set -e

echo "Building NVTT..."
echo

mkdir -p src/build/
cd src/build/

cmake .. -DNVTT_SHARED=1 -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"

make nvtt

cd ../../

if [ "`uname -s`" = "Darwin" ]
then
  DLL_EXTN=dylib
  LIB_EXTN=dylib
  LIB_PREFIX=lib
else
  DLL_EXTN=so
  LIB_EXTN=so
  LIB_PREFIX=lib
fi

cp src/build/src/nv*/${LIB_PREFIX}nv*.${LIB_EXTN} lib/
cp src/build/src/nv*/${LIB_PREFIX}nv*.${DLL_EXTN} ../../binaries/system/
