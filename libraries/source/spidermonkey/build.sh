#!/bin/sh
# This script is called by update-workspaces.sh / build-osx-libraries.sh
set -e

# This should match the version in config/milestone.txt
FOLDER="mozjs-68.12.1"
# If same-version changes are needed, increment this.
LIB_VERSION="68.12.1+0"
LIB_NAME="mozjs68-ps"

# Since this script is called by update-workspaces.sh, we want to quickly
# avoid doing any work if SpiderMonkey is already built and up-to-date.
# Running SM's Makefile is a bit slow and noisy, so instead we'll make a
# special file and only rebuild if the build.sh version differs.
if [ -e .already-built ] && [ "$(cat .already-built)" = "${LIB_VERSION}" ]
then
    echo "SpiderMonkey is already up to date."
    exit
fi

echo "Building SpiderMonkey..."
echo

# Use Mozilla make on Windows
if [ "${OS}" = "Windows_NT" ]
then
  MAKE="mozmake"
else
  MAKE=${MAKE:="make"}
fi

MAKE_OPTS="${JOBS}"

# Standalone SpiderMonkey can not use jemalloc (see https://bugzilla.mozilla.org/show_bug.cgi?id=1465038)
CONF_OPTS="--disable-tests
           --disable-jemalloc
           --disable-js-shell
           --without-intl-api
           --enable-shared-js"

# NSPR is needed on Windows for POSIX emulation.
# If you want to build on Windows, check README.txt and edit the absolute paths
# to match your environment.
if [ "${OS}" = "Windows_NT" ]
then
  # On windows, you may either use the full firefox build process (via mach bootstrap)
  # or simply install LLVM to some convenient directory and hack the env.
  if [ ! -d "~/.mozbuild"];
  then
    export MOZBUILD_STATE_PATH="C:/Program Files/LLVM"
  fi
  CONF_OPTS="${CONF_OPTS} --enable-nspr-build --with-visual-studio-version=2017 --target=i686"
else
  CONF_OPTS="${CONF_OPTS} --enable-posix-nspr-emulation"
fi

if [ "`uname -s`" = "Darwin" ]
then
  # Link to custom-built zlib
  CONF_OPTS="${CONF_OPTS} --with-system-zlib=${ZLIB_DIR}"
  # Specify target versions and SDK
  if [ "${MIN_OSX_VERSION}" ] && [ "${MIN_OSX_VERSION-_}" ]; then
    CONF_OPTS="${CONF_OPTS} --enable-macos-target=$MIN_OSX_VERSION"
  fi
  if [ "${SYSROOT}" ] && [ "${SYSROOT-_}" ]; then
    CONF_OPTS="${CONF_OPTS} --with-macos-sdk=${SYSROOT}"
  fi
fi

# If Valgrind looks like it's installed, then set up SM to support it
# (else the JITs will interact poorly with it)
if [ -e /usr/include/valgrind/valgrind.h ]
then
  CONF_OPTS="${CONF_OPTS} --enable-valgrind"
fi

# Quick sanity check to print explicit error messages
if [ ! "$(command -v rustc)" ]
then
  echo "Error: rustc is not available. Install the rust toolchain before proceeding."
  exit 1
fi

# We need to be able to override CHOST in case it is 32bit userland on 64bit kernel
CONF_OPTS="${CONF_OPTS} \
  ${CBUILD:+--build=${CBUILD}} \
  ${CHOST:+--host=${CHOST}} \
  ${CTARGET:+--target=${CTARGET}}"

echo "SpiderMonkey build options: ${CONF_OPTS}"

# It can occasionally be useful to not rebuild everything, but don't do this by default.
REBUILD=true
if $REBUILD = true;
then
  # Delete the existing directory to avoid conflicts and extract the tarball
  rm -rf "$FOLDER"
  if [ ! -e "${FOLDER}.tar.bz2" ];
  then
    # The tarball is committed to svn, but it's useful to let jenkins download it (when testing upgrade scripts).
    download="$(command -v wget || echo "curl -L -o "${FOLDER}.tar.bz2"")"
    $download "https://github.com/wraitii/spidermonkey-tarballs/releases/download/v68.12.1/${FOLDER}.tar.bz2"
  fi
  tar xjf "${FOLDER}.tar.bz2"

  # Clean up header files that may be left over by earlier versions of SpiderMonkey
  rm -rf include-unix-debug
  rm -rf include-unix-release

  # Apply patches
  cd "$FOLDER"
  . ../patch.sh
  # Prevent complaining that configure is outdated.
  touch ./js/src/configure
else
  cd "$FOLDER"
fi

mkdir -p build-debug
cd build-debug
# SM configure checks for autoconf and llvm-objdump, but neither are actually used for building.
# To avoid a dependency, pass something arbitrary (it does need to be an actual program).
CXXFLAGS="${CXXFLAGS}" ../js/src/configure AUTOCONF="ls" LLVM_OBJDUMP="ls" ${CONF_OPTS} \
  --enable-debug \
  --disable-optimize \
  --enable-gczeal
${MAKE} ${MAKE_OPTS}
cd ..

mkdir -p build-release
cd build-release
CXXFLAGS="${CXXFLAGS}" ../js/src/configure AUTOCONF="ls" LLVM_OBJDUMP="ls" ${CONF_OPTS} \
  --enable-optimize
${MAKE} ${MAKE_OPTS}
cd ..

cd ..

if [ "${OS}" = "Windows_NT" ]
then
  INCLUDE_DIR_DEBUG=include-win32-debug
  INCLUDE_DIR_RELEASE=include-win32-release
  LIB_PREFIX=
  LIB_SUFFIX=.dll
else
  INCLUDE_DIR_DEBUG=include-unix-debug
  INCLUDE_DIR_RELEASE=include-unix-release
  LIB_PREFIX=lib
  LIB_SUFFIX=.so
  if [ "`uname -s`" = "OpenBSD" ];
  then
    LIB_SUFFIX=.so.1.0
  elif [ "`uname -s`" = "Darwin" ];
  then
    LIB_SUFFIX=.a
  fi
fi

if [ "${OS}" = "Windows_NT" ]
then
  # Bug #776126
  # SpiderMonkey uses a tweaked zlib when building, and it wrongly copies its own files to include dirs
  # afterwards, so we have to remove them to not have them conflicting with the regular zlib
  pushd "${FOLDER}/build-release/dist/include"
  rm -f mozzconf.h zconf.h zlib.h
  popd
  pushd "${FOLDER}/build-debug/dist/include"
  rm -f mozzconf.h zconf.h zlib.h
  popd
fi

# Copy files into the necessary locations for building and running the game

# js-config.h is different for debug and release builds, so we need different include directories for both
mkdir -p "${INCLUDE_DIR_DEBUG}"
mkdir -p "${INCLUDE_DIR_RELEASE}"
cp -R -L "${FOLDER}"/build-release/dist/include/* "${INCLUDE_DIR_RELEASE}/"
cp -R -L "${FOLDER}"/build-debug/dist/include/* "${INCLUDE_DIR_DEBUG}/"

# These align the ligns below, making it easier to check for mistakes.
DEB="debug"
REL="release"

mkdir -p lib/
if [ "`uname -s`" = "Darwin" ]
then
  # On MacOS, copy the static libraries only.
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}js_static${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}js_static${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
elif [ "${OS}" = "Windows_NT" ]
then
  # Windows needs DLLs to binaries/, static stubs to lib/ and debug symbols
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}.lib" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}.lib"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}.lib" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}.lib"
  # Copy debug symbols as well.
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}.pdb" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}.pdb"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}.pdb" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}.pdb"
  # Windows need some additional libraries for posix emulation.
  cp -L "${FOLDER}/build-release/dist/bin/${LIB_PREFIX}nspr4.dll" "../../../binaries/system/${LIB_PREFIX}nspr4.dll"
  cp -L "${FOLDER}/build-release/dist/bin/${LIB_PREFIX}plc4.dll" "../../../binaries/system/${LIB_PREFIX}plc4.dll"
  cp -L "${FOLDER}/build-release/dist/bin/${LIB_PREFIX}plds4.dll" "../../../binaries/system/${LIB_PREFIX}plds4.dll"
else
  # Copy shared libs to both lib/ and binaries/ so the compiler and executable (resp.) can find them.
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
fi

# Flag that it's already been built successfully so we can skip it next time
echo "${LIB_VERSION}" > .already-built
