#!/bin/sh
# This script is called by update-workspaces.sh / build-osx-libraries.sh
set -e

# This should match the version in config/milestone.txt
FOLDER="mozjs-91.13.1"
# If same-version changes are needed, increment this.
LIB_VERSION="91.13.1+0"
LIB_NAME="mozjs91-ps"

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

OS="${OS:=$(uname -s)}"

# Use Mozilla make on Windows
if [ "${OS}" = "Windows_NT" ]
then
  MAKE="mozmake"
else
  MAKE=${MAKE:="make"}
fi

MAKE_OPTS="${JOBS}"

# Standalone SpiderMonkey can not use jemalloc (see https://bugzilla.mozilla.org/show_bug.cgi?id=1465038)
# Jitspew doesn't compile on VS17 in the zydis disassembler - since we don't use it, deactivate it.
# Trace-logging doesn't compile for now.
CONF_OPTS="--disable-tests
           --disable-jemalloc
           --disable-js-shell
           --without-intl-api
           --enable-shared-js
           --disable-jitspew"

if [ -n "$PROFILE" ]
then
  CONF_OPTS="$CONF_OPTS --enable-profiling
                        --enable-perf
                        --enable-instruments
                        --enable-jitspew
                        --with-jitreport-granularity=3"
fi

if [ "${OS}" = "Windows_NT" ]
then
  CONF_OPTS="${CONF_OPTS} --target=i686"
elif [ "${OS}" = "Darwin" ]
  then
  # Unless we are forcing an architecture, switch between ARM / x86 based on platform.
  if [ -z "${ARCH}" ]; then
    if [ "$(uname -m)" == "arm64" ]
    then
      ARCH="aarch64"
    else
      ARCH="x86_64"
    fi
  fi
  CONF_OPTS="${CONF_OPTS} --target=$ARCH-apple-darwin"

  # Link to custom-built zlib
  export PKG_CONFIG_PATH="=${ZLIB_DIR}:${PKG_CONFIG_PATH}"
  CONF_OPTS="${CONF_OPTS} --with-system-zlib"
  # Specify target versions and SDK
  if [ "${MIN_OSX_VERSION}" ] && [ "${MIN_OSX_VERSION-_}" ]; then
    CONF_OPTS="${CONF_OPTS} --enable-macos-target=$MIN_OSX_VERSION"
  fi
  if [ "${SYSROOT}" ] && [ "${SYSROOT-_}" ]; then
    CONF_OPTS="${CONF_OPTS} --with-macos-sdk=${SYSROOT}"
  fi
fi

LLVM_OBJDUMP=${LLVM_OBJDUMP:=$(command -v llvm-objdump || command -v objdump)}

# Quick sanity check to print explicit error messages
# (Don't run this on windows as it would likely fail spuriously)
if [ "${OS}" != "Windows_NT" ]
then
  [ -n "$(command -v rustc)" ] || (echo "Error: rustc is not available. Install the rust toolchain (rust + cargo) before proceeding." && exit 1)
  [ -n "${LLVM_OBJDUMP}" ] || (echo "Error: LLVM objdump is not available. Install it (likely via LLVM-clang) before proceeding." && exit 1)
fi

# If Valgrind looks like it's installed, then set up SM to support it
# (else the JITs will interact poorly with it)
if [ -e /usr/include/valgrind/valgrind.h ]
then
  CONF_OPTS="${CONF_OPTS} --enable-valgrind"
fi

# We need to be able to override CHOST in case it is 32bit userland on 64bit kernel
CONF_OPTS="${CONF_OPTS} \
  ${CBUILD:+--build=${CBUILD}} \
  ${CHOST:+--host=${CHOST}} \
  ${CTARGET:+--target=${CTARGET}}"

echo "SpiderMonkey build options: ${CONF_OPTS}"

# It can occasionally be useful to not rebuild everything, but don't do this by default.
REBUILD=${REBUILD:=true}
if [ $REBUILD = true ]
then
  # Delete the existing directory to avoid conflicts and extract the tarball
  rm -rf "$FOLDER"
  if [ ! -e "${FOLDER}.tar.xz" ]
  then
    # The tarball is committed to svn, but it's useful to let jenkins download it (when testing upgrade scripts).
    download="$(command -v wget || echo "curl -L -o "${FOLDER}.tar.xz"")"
    $download "https://github.com/wraitii/spidermonkey-tarballs/releases/download/${FOLDER}/${FOLDER}.tar.xz"
  fi
  tar xfJ "${FOLDER}.tar.xz"

  # Clean up header files that may be left over by earlier versions of SpiderMonkey
  rm -rf include-unix-debug
  rm -rf include-unix-release

  cd "$FOLDER"

  # Prevent complaining that configure is outdated.
  touch ./js/src/configure

  # Apply patches
  . ../patch.sh
else
  cd "$FOLDER"
fi

# Debug version of SM is broken on FreeBSD.
if [ "${OS}" != "FreeBSD" ]
then
  mkdir -p build-debug
  cd build-debug
  # llvm-objdump is searched for with the complete name, not simply 'objdump', account for that.
  CXXFLAGS="${CXXFLAGS}" ../js/src/configure \
    LLVM_OBJDUMP="${LLVM_OBJDUMP}" \
    ${CONF_OPTS} \
    --enable-debug \
    --disable-optimize \
    --enable-gczeal
  ${MAKE} ${MAKE_OPTS}
  cd ..
fi

mkdir -p build-release
cd build-release
CXXFLAGS="${CXXFLAGS}" ../js/src/configure \
  LLVM_OBJDUMP="${LLVM_OBJDUMP}" \
  ${CONF_OPTS} \
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
  STATIC_LIB_SUFFIX=.lib
else
  INCLUDE_DIR_DEBUG=include-unix-debug
  INCLUDE_DIR_RELEASE=include-unix-release
  LIB_PREFIX=lib
  LIB_SUFFIX=.so
  STATIC_LIB_SUFFIX=.a
  if [ "${OS}" = "OpenBSD" ];
  then
    LIB_SUFFIX=.so.1.0
  elif [ "${OS}" = "Darwin" ];
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
mkdir -p "${INCLUDE_DIR_RELEASE}"
cp -R -L "${FOLDER}"/build-release/dist/include/* "${INCLUDE_DIR_RELEASE}/"

if [ "${OS}" != "FreeBSD" ]; then
  mkdir -p "${INCLUDE_DIR_DEBUG}"
  cp -R -L "${FOLDER}"/build-debug/dist/include/* "${INCLUDE_DIR_DEBUG}/"
fi

# These align the ligns below, making it easier to check for mistakes.
DEB="debug"
REL="release"

mkdir -p lib/

# Fetch the jsrust static library. Path is grepped from the build file as it varies by rust toolset.
rust_path=$(grep jsrust < "${FOLDER}/build-release/js/src/build/backend.mk" | cut -d = -f 2 | cut -c2-)
cp -L "${rust_path}" "lib/${LIB_PREFIX}${LIB_NAME}-rust${STATIC_LIB_SUFFIX}"

if [ "${OS}" = "Darwin" ]
then
  # On MacOS, copy the static libraries only.
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}js_static${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}js_static${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
elif [ "${OS}" = "Windows_NT" ]
then
  # Windows needs DLLs to binaries/, static stubs to lib/ and debug symbols
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${STATIC_LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}${STATIC_LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${STATIC_LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}${STATIC_LIB_SUFFIX}"
  # Copy debug symbols as well.
  cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}.pdb" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}.pdb"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}.pdb" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}.pdb"
  # Copy the debug jsrust library.
  rust_path=$(grep jsrust < "${FOLDER}/build-debug/js/src/build/backend.mk" | cut -d = -f 2 | cut -c2-)
  cp -L "${rust_path}" "lib/${LIB_PREFIX}${LIB_NAME}-rust-debug${STATIC_LIB_SUFFIX}"
else
  # Copy shared libs to both lib/ and binaries/ so the compiler and executable (resp.) can find them.
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
  cp -L "${FOLDER}/build-${REL}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${REL}${LIB_SUFFIX}"
  if [ "${OS}" != "FreeBSD" ]; then
    cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "../../../binaries/system/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
    cp -L "${FOLDER}/build-${DEB}/js/src/build/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}" "lib/${LIB_PREFIX}${LIB_NAME}-${DEB}${LIB_SUFFIX}"
  fi
fi

# Flag that it's already been built successfully so we can skip it next time
echo "${LIB_VERSION}" > .already-built
