#!/bin/sh
set -e
LIB_VERSION="fcollada-3.05+wildfiregames.8"
JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}
LDFLAGS=${LDFLAGS:=""}
CFLAGS=${CFLAGS:=""}
CXXFLAGS=${CXXFLAGS:=""}

if [ -e .already-built ] && [ "$(cat .already-built)" = "${LIB_VERSION}" ]
then
  echo "FCollada is already up to date."
  exit
fi

echo "Building FCollada..."
echo

if [ "$(uname -s)" = "Darwin" ]; then
  # The Makefile refers to pkg-config for libxml2, but we
  # don't have that (replace with xml2-config instead).
  sed -i.bak -e 's/pkg-config libxml-2.0/xml2-config/' src/Makefile
fi

rm -f .already-built
rm -f lib/*.a
mkdir -p lib
(cd src && rm -rf "output/" && "${MAKE}" clean && CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" && LDFLAGS="$LDFLAGS" "${MAKE}" "${JOBS}")

if [ "$(uname -s)" = "Darwin" ]; then
  # Undo Makefile change as we don't want to have it when creating patches.
  mv src/Makefile.bak src/Makefile
fi

echo "$LIB_VERSION" > .already-built
