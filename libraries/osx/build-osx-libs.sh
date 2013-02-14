#!/bin/bash
#
# Script for acquiring and building OS X dependencies for 0 A.D.
#
# The script checks whether a source tarball exists for each
# dependency, if not it will download the correct version from
# the project's website, then it removes previous build files,
# extracts the tarball, configures and builds the lib. The script
# should die on any errors to ease troubleshooting.
#
# make install is used to copy the compiled libs to each specific
# directory and also the config tools (e.g. sdl-config). Because
# of this, OS X developers must run this script at least once,
# to configure the correct lib directories. It must be run again
# if the libraries are moved.
#
# Building against an SDK is an option, though not required,
# as not all build environments contain the Developer SDKs
# (Xcode does, but the Command Line Tools package does not)
#

set -e

die()
{
  echo ERROR: $*
  exit 1
}

download_lib()
{
  local url=$1
  local filename=$2

  if [ ! -e $filename ]; then
    echo "Downloading $filename"
    curl -L -O ${url}${filename} || die "Download of $url$filename failed"
  fi
}

# --------------------------------------------------------------
# Library versions for ease of updating:
# * SDL 1.2.15+ required for Lion support
SDL_VERSION="SDL-1.2.15"
BOOST_VERSION="boost_1_52_0"
# * wxWidgets 2.9+ is necessary for 64-bit OS X build w/ OpenGL support
WXWIDGETS_VERSION="wxWidgets-2.9.4"
JPEG_VERSION="jpegsrc.v8d"
JPEG_DIR="jpeg-8d" # Must match directory name inside source tarball
# * libpng was included as part of X11 but that's removed from Mountain Lion
#   (also the Snow Leopard version was ancient 1.2)
PNG_VERSION="libpng-1.5.13"
OGG_VERSION="libogg-1.3.0"
VORBIS_VERSION="libvorbis-1.3.3"
# --------------------------------------------------------------
# Bundled with the game:
# * SpiderMonkey 1.8.5
# * ENet
# * NVTT
# * FCollada
# --------------------------------------------------------------
# Provided by OS X:
# * curl
# * libxml2
# * OpenAL
# * OpenGL
# * zlib
# --------------------------------------------------------------

# Force build architecture, as sometimes configure is broken.
# (Using multiple values would in theory produce a "universal"
#  or fat binary, but this is untested)
#
# Choices are: x86_64 i386 (ppc and ppc64 NOT supported)
ARCH=${ARCH:="x86_64"}

# Define compiler as simply gcc (if anything expects e.g. gcc-4.2)
# sometimes the OS X build environment will be messed up because
# Apple dropped GCC support and removed the symbolic links, but 
# after everyone complained they added them again. Now it is
# merely a GCC-like frontend to LLVM.
export CC=${CC:="gcc"} CXX=${CXX:="g++"}

# The various libs offer inconsistent configure options, some allow
# setting sysroot and OS X-specific options, others don't. Adding to
# the confusion, Apple moved /Developer/SDKs into the Xcode app bundle
# so the path can't be guessed by clever build tools (like Boost.Build).
# Sometimes configure gets it wrong anyway, especially on cross compiles.
# This is why we prefer using CFLAGS, CPPFLAGS, and LDFLAGS.

# Check if SYSROOT is set and not empty
if [[ $SYSROOT && ${SYSROOT-_} ]]; then
  CFLAGS="$CFLAGS -isysroot $SYSROOT"
  LDFLAGS="$LDFLAGS -Wl,-syslibroot,$SYSROOT"
fi
# Check if MIN_OSX_VERSION is set and not empty
if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
  CFLAGS="$CFLAGS -mmacosx-version-min=$MIN_OSX_VERSION"
  LDFLAGS="$LDFLAGS -mmacosx-version-min=$MIN_OSX_VERSION"
fi
CFLAGS="$CFLAGS -arch $ARCH"
CPPFLAGS="$CPPFLAGS $CFLAGS"
LDFLAGS="$LDFLAGS -arch $ARCH"

JOBS=${JOBS:="-j2"}

# Check that we're actually on OS X
if [ "`uname -s`" != "Darwin" ]; then
  die "This script is intended for OS X only"
fi

# Parse command-line options:
force_rebuild=false

for i in "$@"
do
  case $i in
    --force-rebuild ) force_rebuild=true;;
    -j* ) JOBS=$i ;;
  esac
done

cd "$(dirname $0)"
# Now in libraries/osx/ (where we assume this script resides)

# --------------------------------------------------------------

echo -e "\nBuilding SDL...\n"

LIB_VERSION="${SDL_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="http://www.libsdl.org/release/"

mkdir -p sdl
pushd sdl

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # Don't use X11 - we don't need it and Mountain Lion removed it
  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --without-x --enable-shared=no && make $JOBS && make install) || die "SDL build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding Boost...\n"

LIB_VERSION="${BOOST_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/boost/"

mkdir -p boost
pushd boost

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # Can't use macosx-version, see above comment.
  (./bootstrap.sh --with-libraries=filesystem,system,signals --prefix=$INSTALL_DIR && ./b2 cflags="$CFLAGS" cxxflags="$CPPFLAGS" linkflags="$LDFLAGS" ${JOBS} -d2 --layout=tagged --debug-configuration link=static threading=multi variant=release,debug install) || die "Boost build failed"

  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
# TODO: This build takes ages, anything we can exlude?
echo -e "\nBuilding wxWidgets...\n"

LIB_VERSION="${WXWIDGETS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/wxwindows/"

mkdir -p wxwidgets
pushd wxwidgets

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  mkdir -p build-release
  pushd build-release

  # Avoid linker warnings about compiling translation units with different visibility settings
  CPPFLAGS="$CPPFLAGS -fvisibility=hidden"
  (../configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --disable-shared --enable-unicode --with-cocoa --with-opengl && make ${JOBS} && make install) || die "wxWidgets build failed"
  popd
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding libjpg...\n"

LIB_VERSION="${JPEG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="${JPEG_DIR}"
LIB_URL="http://www.ijg.org/files/"

mkdir -p libjpg
pushd libjpg

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --enable-shared=no && make ${JOBS} && make install) || die "libjpg build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding libpng...\n"

LIB_VERSION="${PNG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/libpng/"

mkdir -p libpng
pushd libpng

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --enable-shared=no && make ${JOBS} && make install) || die "libpng build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding libogg...\n"

LIB_VERSION="${OGG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://downloads.xiph.org/releases/ogg/"

# Dependency of vorbis
# we can install them in the same directory for convenience
mkdir -p libogg
mkdir -p vorbis

INSTALL_DIR="$(pwd)/vorbis"
pushd libogg

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --enable-shared=no && make ${JOBS} && make install) || die "libogg build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding libvorbis...\n"

LIB_VERSION="${VORBIS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://downloads.xiph.org/releases/vorbis/"

pushd vorbis

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --enable-shared=no --with-ogg="$INSTALL_DIR" && make ${JOBS} && make install) || die "libvorbis build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------------
# The following libraries are shared on different OSes and may
# be customzied, so we build and install them from bundled sources
# --------------------------------------------------------------------
echo -e "\nBuilding Spidermonkey...\n"

LIB_VERSION="js185-1.0.0"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="js-1.8.5"

pushd ../source/spidermonkey/

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f lib/*.a
  rm -rf $LIB_DIRECTORY
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY/js/src

  # We want separate debug/release versions of the library, so change their install name in the Makefile
  sed -i.bak -e 's/\(STATIC_LIBRARY_NAME),mozjs185-\)\(\$(SRCREL_ABI_VERSION)\)\{0,1\}/\1ps-debug/' Makefile.in

  CONF_OPTS="--prefix=${INSTALL_DIR} --disable-tests --disable-shared-js"
  # Uncomment this line for 32-bit 10.5 cross compile:
  #CONF_OPTS="$CONF_OPTS --target=i386-apple-darwin9.0.0"
  if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
    CONF_OPTS="$CONF_OPTS --enable-macos-target=$MIN_OSX_VERSION"
  fi
  if [[ $SYSROOT && ${SYSROOT-_} ]]; then
    CONF_OPTS="$CONF_OPTS --with-macosx-sdk=$SYSROOT"
  fi

  mkdir -p build-debug
  pushd build-debug
  (CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" AR=ar CROSS_COMPILE=1 ../configure $CONF_OPTS --enable-debug --disable-optimize && make ${JOBS} && make install) || die "Spidermonkey build failed"
  popd
  mv Makefile.in.bak Makefile.in
  
  sed -i.bak -e 's/\(STATIC_LIBRARY_NAME),mozjs185-\)\(\$(SRCREL_ABI_VERSION)\)\{0,1\}/\1ps-release/' Makefile.in

  mkdir -p build-release
  pushd build-release
  (CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" AR=ar CROSS_COMPILE=1 ../configure $CONF_OPTS && make ${JOBS} && make install) || die "Spidermonkey build failed"
  popd
  mv Makefile.in.bak Makefile.in
  
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
echo -e "\nBuilding ENet...\n"

pushd ../source/enet/

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  INSTALL_DIR="$(pwd)"

  pushd src
  (./configure CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" LDFLAGS="$LDFLAGS" --prefix=${INSTALL_DIR} --enable-shared=no && make clean && make ${JOBS} && make install) || die "ENet build failed"
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
# NVTT - no install
echo -e "\nBuilding NVTT...\n"

pushd ../source/nvtt

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  pushd src
  rm -rf build
  mkdir -p build

  pushd build

  # Could use CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_SYSROOT
  # but they're not as flexible for cross-compiling
  # Disable optional libs that we don't need (avoids some conflicts with MacPorts)
  (cmake .. -DCMAKE_LINK_FLAGS="$LDFLAGS" -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CPPFLAGS" -DCMAKE_BUILD_TYPE=Release -DBINDIR=bin -DLIBDIR=lib -DGLUT=0 -DGLEW=0 -DCG=0 -DCUDA=0 -DOPENEXR=0 -DJPEG=0 -DPNG=0 -DTIFF=0 -G "Unix Makefiles" && make clean && make nvtt ${JOBS}) || die "NVTT build failed"
  popd

  mkdir -p ../lib
  cp build/src/nv*/libnv*.a ../lib/
  cp build/src/nvtt/squish/libsquish.a ../lib/
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd

# --------------------------------------------------------------
# FCollada - no install
echo -e "\nBuilding FCollada...\n"

pushd ../source/fcollada

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  pushd src
  rm -rf output
  mkdir -p ../lib

  # The Makefile refers to pkg-config for libxml2, but we
  # don't have that (replace with xml2-config instead)
  sed -i.bak -e 's/pkg-config libxml-2.0/xml2-config/' Makefile
  (make clean && CXXFLAGS=$CPPFLAGS make ${JOBS}) || die "FCollada build failed"
  # Undo Makefile change
  mv Makefile.bak Makefile
  popd
  touch .already-built
else
  echo -e "\nSkipping - already built\n"
fi
popd
