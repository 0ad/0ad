#!/bin/sh
set -e
LIB_VERSION="nvtt-2.1.1+wildfiregames.4"
JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}
LDFLAGS=${LDFLAGS:=""}
CFLAGS=${CFLAGS:=""}
CXXFLAGS=${CXXFLAGS:=""}
CMAKE_FLAGS=${CMAKE_FLAGS:=""}

if [ -e .already-built ] && [ "$(cat .already-built)" = "${LIB_VERSION}" ]
then
  echo "NVTT is already up to date."
  exit
fi

echo "Building NVTT..."
echo

rm -f .already-built
rm -f lib/*.a
rm -rf src/build/
mkdir -p src/build/
cd src/build/

if [ "$(uname -s)" = "Darwin" ]; then
  # Could use CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_SYSROOT
  # but they're not as flexible for cross-compiling
  # Disable png support (avoids some conflicts with MacPorts)
  cmake .. \
    -DCMAKE_LINK_FLAGS="$LDFLAGS" \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_BUILD_TYPE=Release \
    $CMAKE_FLAGS \
    -DBINDIR=bin \
    -DLIBDIR=lib \
    -DPNG=0 \
    -G "Unix Makefiles"
else
  cmake .. \
    -DCMAKE_LINK_FLAGS="$LDFLAGS" \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    $CMAKE_FLAGS \
    -DNVTT_SHARED=1 \
    -DOpenGL_GL_PREFERENCE=GLVND \
    -DBINDIR=bin \
    -DLIBDIR=lib \
    -G "Unix Makefiles"
fi

("${MAKE}" clean && "${MAKE}" nvtt "${JOBS}") || die "NVTT build failed"
cd ../../
mkdir -p lib/
LIB_PREFIX=lib

if [ "$(uname -s)" = "Darwin" ]; then
  LIB_EXTN=a
  cp src/build/src/bc*/"${LIB_PREFIX}"bc*."${LIB_EXTN}" lib/
  cp src/build/src/nvtt/squish/"${LIB_PREFIX}"squish."${LIB_EXTN}" lib/
else
  LIB_EXTN=so
  cp src/build/src/nv*/"${LIB_PREFIX}"nv*."${LIB_EXTN}" ../../../binaries/system/
fi

cp src/build/src/nv*/"${LIB_PREFIX}"nv*."${LIB_EXTN}" lib/

echo "$LIB_VERSION" > .already-built
