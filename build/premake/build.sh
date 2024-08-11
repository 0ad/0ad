#!/bin/sh
set -e

LIB_VERSION="premake-5-alpha-14+wildfiregames.1"

if [ -e .already-built ] && [ "$(cat .already-built)" = "${LIB_VERSION}" ]
then
  echo "Premake 5 is already up to date."
  exit
fi

JOBS=${JOBS:="-j2"}
MAKE=${MAKE:="make"}
CFLAGS="${CFLAGS:=""} -Wno-error=implicit-function-declaration"
OS="${OS:=$(uname -s)}"

echo "Building Premake 5..."
echo

PREMAKE_BUILD_DIR=premake5/build/gmake2.unix
# BSD and OS X need different Makefiles
case "$OS" in
"GNU/kFreeBSD" )
    # use default gmake2.unix (needs -ldl as we have a GNU userland and libc)
    ;;
*"BSD" )
    PREMAKE_BUILD_DIR=premake5/build/gmake2.bsd
    ;;
"Darwin" )
    PREMAKE_BUILD_DIR=premake5/build/gmake2.macosx
    ;;
esac
${MAKE} -C "${PREMAKE_BUILD_DIR}" "${JOBS}" CFLAGS="$CFLAGS" config=release

echo "${LIB_VERSION}" > .already-built
