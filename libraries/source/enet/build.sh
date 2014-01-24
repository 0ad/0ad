#!/bin/sh

set -e

JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}

echo "Building libenet..."
echo

cd src/

./configure SET_MAKE=${MAKE}

${MAKE} ${JOBS}

cd ../

mkdir -p lib/

case "`uname -s`" in
  "OpenBSD" )
    cp src/.libs/libenet.so.1.* lib/
    cp src/.libs/libenet.so.1.* ../../../binaries/system/
    ;;
  * )
    cp src/.libs/libenet.so lib/
    cp src/.libs/libenet.so.1 ../../../binaries/system/
    ;;
esac
