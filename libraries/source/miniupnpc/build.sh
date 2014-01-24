#!/bin/sh

set -e

JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}

echo "Building libminiupnpc..."
echo

cd src/

${MAKE} ${JOBS}

cd ../

mkdir -p lib/

case "`uname -s`" in
  * )
    cp src/libminiupnpc.so lib/
    cp src/libminiupnpc.so ../../../binaries/system/libminiupnpc.so.9
    ;;
esac
