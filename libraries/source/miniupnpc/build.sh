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
  "Darwin" )
    # Fix libtool's use of an absolute path
    install_name_tool -id @executable_path/libminiupnpc.1.dylib src/libminiupnpc.1.dylib
    cp src/libminiupnpc.dylib lib/
    cp src/libminiupnpc.dylib ../../../binaries/system/
    ;;
  "OpenBSD" )
    cp src/libminiupnpc.so lib/
    cp src/libminiupnpc.so ../../../binaries/system/libminiupnpc.so.9
    ;;
  * )
    cp src/libminiupnpc.so lib/
    cp src/libminiupnpc.so ../../../binaries/system/libminiupnpc.so.9
    ;;
esac
