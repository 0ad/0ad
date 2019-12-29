#!/bin/sh

set -e

JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}

echo "Building NVTT..."
echo

mkdir -p src/build/
cd src/build/

cmake .. -DNVTT_SHARED=1 -DCMAKE_BUILD_TYPE=Release -DBINDIR=bin -DLIBDIR=lib -G "Unix Makefiles"

${MAKE} nvtt ${JOBS}

cd ../../

DLL_EXTN=so
LIB_EXTN=so
LIB_PREFIX=lib

cp src/build/src/nv*/${LIB_PREFIX}nv*.${LIB_EXTN} lib/
cp src/build/src/nv*/${LIB_PREFIX}nv*.${DLL_EXTN} ../../../binaries/system/
