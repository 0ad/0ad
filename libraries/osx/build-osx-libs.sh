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
# --------------------------------------------------------------
# Library versions for ease of updating:
ZLIB_VERSION="zlib-1.2.8"
CURL_VERSION="curl-7.32.0"
ICONV_VERSION="libiconv-1.14"
XML2_VERSION="libxml2-2.9.1"
# * SDL 1.2.15+ required for Lion support
SDL_VERSION="SDL-1.2.15"
BOOST_VERSION="boost_1_52_0"
# * wxWidgets 2.9+ is necessary for 64-bit OS X build w/ OpenGL support
WXWIDGETS_VERSION="wxWidgets-3.0.1"
JPEG_VERSION="jpegsrc.v8d"
JPEG_DIR="jpeg-8d" # Must match directory name inside source tarball
# * libpng was included as part of X11 but that's removed from Mountain Lion
#   (also the Snow Leopard version was ancient 1.2)
PNG_VERSION="libpng-1.5.13"
OGG_VERSION="libogg-1.3.0"
VORBIS_VERSION="libvorbis-1.3.3"
# gloox is necessary for multiplayer lobby
GLOOX_VERSION="gloox-1.0.9"
# NSPR is necessary for threadsafe Spidermonkey
NSPR_VERSION="4.10.3"
# OS X only includes part of ICU, and only the dylib
ICU_VERSION="icu4c-52_1"
# --------------------------------------------------------------
# Bundled with the game:
# * SpiderMonkey 24
# * ENet 1.3.3
# * NVTT
# * FCollada
# * MiniUPnPc
# --------------------------------------------------------------
# Provided by OS X:
# * OpenAL
# * OpenGL
# --------------------------------------------------------------

# Force build architecture, as sometimes environment is broken.
# For a universal fat binary, the approach would be to build every
# dependency with both archs and combine them with lipo, then do the
# same thing with the game itself.
# Choices are "x86_64" or  "i386" (ppc and ppc64 not supported)
ARCH=${ARCH:="x86_64"}

# Define compiler as "gcc" (in case anything expects e.g. gcc-4.2)
# On newer OS X versions, this will be a symbolic link to LLVM GCC
# TODO: don't rely on that
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

# The various libs offer inconsistent configure options, some allow
# setting sysroot and OS X-specific options, others don't. Adding to
# the confusion, Apple moved /Developer/SDKs into the Xcode app bundle
# so the path can't be guessed by clever build tools (like Boost.Build).
# Sometimes configure gets it wrong anyway, especially on cross compiles.
# This is why we prefer using (OBJ)CFLAGS, (OBJ)CXXFLAGS, and LDFLAGS.

# Check if SYSROOT is set and not empty
if [[ $SYSROOT && ${SYSROOT-_} ]]; then
  C_FLAGS="-isysroot $SYSROOT"
  LDFLAGS="$LDFLAGS -Wl,-syslibroot,$SYSROOT"
fi
# Check if MIN_OSX_VERSION is set and not empty
if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
  C_FLAGS="$C_FLAGS -mmacosx-version-min=$MIN_OSX_VERSION"
  # clang and llvm-gcc look at mmacosx-version-min to determine link target
  # and CRT version, and use it to set the macosx_version_min linker flag
  LDFLAGS="$LDFLAGS -mmacosx-version-min=$MIN_OSX_VERSION"
fi
C_FLAGS="$C_FLAGS -arch $ARCH -fvisibility=hidden"
LDFLAGS="$LDFLAGS -arch $ARCH"

CFLAGS="$CFLAGS $C_FLAGS"
CXXFLAGS="$CXXFLAGS $C_FLAGS"
OBJCFLAGS="$OBJCFLAGS $C_FLAGS"
OBJCXXFLAGS="$OBJCXXFLAGS $C_FLAGS"

JOBS=${JOBS:="-j2"}

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

already_built()
{
  echo -e "Skipping - already built (use --force-rebuild to override)"
}

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
echo -e "Building zlib..."

LIB_VERSION="${ZLIB_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="http://zlib.net/"

mkdir -p zlib
pushd zlib > /dev/null

ZLIB_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # patch zlib's configure script to use our CFLAGS and LDFLAGS
  (patch -p0 -i ../../patches/zlib_flags.diff && CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" ./configure --prefix="$ZLIB_DIR" --static && make ${JOBS} && make install) || die "zlib build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libcurl..."

LIB_VERSION="${CURL_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://curl.haxx.se/download/"

mkdir -p libcurl
pushd libcurl > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --enable-ipv6 --without-gnutls --without-gssapi --without-libmetalink --without-librtmp --without-libssh2 --without-nss --without-polarssl --without-spnego --without-ssl --disable-ares --disable-ldap --disable-ldaps --without-libidn --with-zlib="${ZLIB_DIR}" --enable-shared=no && make ${JOBS} && make install) || die "libcurl build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libiconv..."

LIB_VERSION="${ICONV_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://ftp.gnu.org/pub/gnu/libiconv/"

mkdir -p iconv
pushd iconv > /dev/null

ICONV_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix="$ICONV_DIR" --without-libiconv-prefix --without-libintl-prefix --disable-nls --enable-shared=no && make ${JOBS} && make install) || die "libiconv build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libxml2..."

LIB_VERSION="${XML2_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="ftp://xmlsoft.org/libxml2/"

mkdir -p libxml2
pushd libxml2 > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --without-lzma --without-python --with-iconv="${ICONV_DIR}" --with-zlib="${ZLIB_DIR}" --enable-shared=no && make ${JOBS} && make install) || die "libxml2 build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------

echo -e "Building SDL..."

LIB_VERSION="${SDL_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="http://www.libsdl.org/release/"

mkdir -p sdl
pushd sdl > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY


  # patch SDL to fix Mavericks build (fixed upstream, see https://bugzilla.libsdl.org/show_bug.cgi?id=2085 )
  # Don't use X11 - we don't need it and Mountain Lion removed it
  (patch -p0 -i ../../patches/sdl-mavericks-quartz-fix.diff && ./configure CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --disable-video-x11 --without-x --enable-shared=no && make $JOBS && make install) || die "SDL build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building Boost..."

LIB_VERSION="${BOOST_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/boost/"

mkdir -p boost
pushd boost > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # Can't use macosx-version, see above comment.
(./bootstrap.sh --with-libraries=filesystem,system,signals --prefix=$INSTALL_DIR && ./b2 cflags="$CFLAGS" toolset=clang cxxflags="$CXXFLAGS" linkflags="$LDFLAGS" ${JOBS} -d2 --layout=tagged --debug-configuration link=static threading=multi variant=release,debug install) || die "Boost build failed"

  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# TODO: This build takes ages, anything we can exlude?
echo -e "Building wxWidgets..."

LIB_VERSION="${WXWIDGETS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/wxwindows/"

mkdir -p wxwidgets
pushd wxwidgets > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  mkdir -p build-release
  pushd build-release

  # disable XML and richtext support, to avoid dependency on expat
  CONF_OPTS="--prefix=$INSTALL_DIR --disable-shared --enable-unicode --with-cocoa --with-opengl --with-libiconv-prefix=${ICONV_DIR} --disable-richtext --without-expat --without-sdl"
  # wxWidgets configure now defaults to targeting 10.5, if not specified,
  # but that conflicts with our flags
  if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
    CONF_OPTS="$CONF_OPTS --with-macosx-version-min=$MIN_OSX_VERSION"
  fi

  (../configure CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" $CONF_OPTS && make ${JOBS} && make install) || die "wxWidgets build failed"
  popd
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libjpg..."

LIB_VERSION="${JPEG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="${JPEG_DIR}"
LIB_URL="http://www.ijg.org/files/"

mkdir -p libjpg
pushd libjpg > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --enable-shared=no && make ${JOBS} && make install) || die "libjpg build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libpng..."

LIB_VERSION="${PNG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://download.sourceforge.net/libpng/"

mkdir -p libpng
pushd libpng > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix=$INSTALL_DIR --enable-shared=no && make ${JOBS} && make install) || die "libpng build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libogg..."

LIB_VERSION="${OGG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://downloads.xiph.org/releases/ogg/"

# Dependency of vorbis
# we can install them in the same directory for convenience
mkdir -p libogg
mkdir -p vorbis

pushd libogg > /dev/null
OGG_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix=$OGG_DIR --enable-shared=no && make ${JOBS} && make install) || die "libogg build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libvorbis..."

LIB_VERSION="${VORBIS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://downloads.xiph.org/releases/vorbis/"

pushd vorbis > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --enable-shared=no --with-ogg="$OGG_DIR" && make ${JOBS} && make install) || die "libvorbis build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building gloox..."

LIB_VERSION="${GLOOX_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://camaya.net/download/"

mkdir -p gloox
pushd gloox > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # patch gloox to fix build error (fixed upstream in r4529)
  # TODO: pulls in libresolv dependency from /usr/lib
  # TODO: if we ever use SSL/TLS, that will add yet another dependency...
  (patch -p0 -i ../../patches/gloox-r4529.diff && ./configure CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" --prefix="$INSTALL_DIR" --enable-shared=no --with-zlib="${ZLIB_DIR}" --without-libidn --without-gnutls --without-openssl --without-tests --without-examples && make ${JOBS} && make install) || die "gloox build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building NSPR..."

LIB_VERSION="${NSPR_VERSION}"
LIB_ARCHIVE="nspr-$LIB_VERSION.tar.gz"
LIB_DIRECTORY="nspr-$LIB_VERSION"
LIB_URL="https://ftp.mozilla.org/pub/mozilla.org/nspr/releases/v$LIB_VERSION/src/"

mkdir -p nspr
pushd nspr > /dev/null

NSPR_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY/nspr

  (CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" ./configure --prefix="$NSPR_DIR" && make ${JOBS} && make install) || die "NSPR build failed"
  popd
  # TODO: how can we not build the dylibs?
  rm -f lib/*.dylib
    touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building ICU..."

LIB_VERSION="${ICU_VERSION}"
LIB_ARCHIVE="$LIB_VERSION-src.tgz"
LIB_DIRECTORY="icu"
LIB_URL="http://download.icu-project.org/files/icu4c/52.1/"

mkdir -p icu
pushd icu > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib sbin share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  mkdir -p source/build
  pushd source/build

(CXX="clang" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS -stdlib=libstdc++" LDFLAGS="$LDFLAGS -lstdc++" ../runConfigureICU MacOSX --prefix=$INSTALL_DIR --disable-shared --enable-static --disable-samples --enable-extras --enable-icuio --enable-layout --enable-tools && make ${JOBS} && make install) || die "ICU build failed"
  popd
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------------
# The following libraries are shared on different OSes and may
# be customzied, so we build and install them from bundled sources
# --------------------------------------------------------------------
echo -e "Building Spidermonkey..."

LIB_VERSION="mozjs-24.2.0"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="mozjs24"

pushd ../source/spidermonkey/ > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ .already-built -ot $LIB_DIRECTORY ]]
then
  INSTALL_DIR="$(pwd)"
  INCLUDE_DIR_DEBUG=$INSTALL_DIR/include-unix-debug
  INCLUDE_DIR_RELEASE=$INSTALL_DIR/include-unix-release

  rm -f .already-built
  rm -f lib/*.a
  rm -rf $LIB_DIRECTORY $INCLUDE_DIR_DEBUG $INCLUDE_DIR_RELEASE
  tar -xf $LIB_ARCHIVE
  # rename the extracted directory to something shorter
  mv $LIB_VERSION $LIB_DIRECTORY
  pushd $LIB_DIRECTORY/js/src

  # We want separate debug/release versions of the library, so change their install name in the Makefile
  perl -i.bak -pe 's/(^STATIC_LIBRARY_NAME\s+=).*/$1mozjs24-ps-debug/' Makefile.in
  perl -i.bak -pe 's/js_static/mozjs24-ps-debug/g' shell/Makefile.in

  CONF_OPTS="--target=$ARCH-apple-darwin --prefix=${INSTALL_DIR} --with-system-nspr --with-nspr-prefix=${NSPR_DIR} --with-system-zlib=${ZLIB_DIR} --enable-threadsafe --disable-tests --disable-shared-js" # --enable-trace-logging"
  # Uncomment this line for 32-bit 10.5 cross compile:
  #CONF_OPTS="$CONF_OPTS --target=i386-apple-darwin9.0.0"
  if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
    CONF_OPTS="$CONF_OPTS --enable-macos-target=$MIN_OSX_VERSION"
  fi
  if [[ $SYSROOT && ${SYSROOT-_} ]]; then
    CONF_OPTS="$CONF_OPTS --with-macosx-sdk=$SYSROOT"
  fi

  # apply patch to fix clang build on Mavericks (see https://bugzilla.mozilla.org/show_bug.cgi?id=917526)
  patch -p0 -d ../../ -i ../../../osx/patches/sm-mavericks-clang-fix.diff || die "Spidermonkey build failed"

  mkdir -p build-debug
  pushd build-debug
  (CC="clang" CXX="clang++" AR=ar CROSS_COMPILE=1 ../configure $CONF_OPTS --enable-debug --disable-optimize --enable-js-diagnostics --enable-gczeal && make ${JOBS}) || die "Spidermonkey build failed"
  # js-config.h is different for debug and release builds, so we need different include directories for both
  mkdir -p $INCLUDE_DIR_DEBUG
  cp -R -L dist/include/* $INCLUDE_DIR_DEBUG/
  cp *.a $INSTALL_DIR/lib
  popd
  mv Makefile.in.bak Makefile.in
  mv shell/Makefile.in.bak shell/Makefile.in

  perl -i.bak -pe 's/(^STATIC_LIBRARY_NAME\s+=).*/$1mozjs24-ps-release/' Makefile.in
  perl -i.bak -pe 's/js_static/mozjs24-ps-release/g' shell/Makefile.in
  mkdir -p build-release
  pushd build-release
  (CC="clang" CXX="clang++" AR=ar CROSS_COMPILE=1 ../configure $CONF_OPTS --enable-optimize && make ${JOBS}) || die "Spidermonkey build failed"
  # js-config.h is different for debug and release builds, so we need different include directories for both
  mkdir -p $INCLUDE_DIR_RELEASE
  cp -R -L dist/include/* $INCLUDE_DIR_RELEASE/
  cp *.a $INSTALL_DIR/lib
  popd
  mv Makefile.in.bak Makefile.in
  mv shell/Makefile.in.bak shell/Makefile.in
  
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building ENet..."

pushd ../source/enet/ > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  INSTALL_DIR="$(pwd)"
  rm -f .already-built

  pushd src
  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" --prefix=${INSTALL_DIR} --enable-shared=no && make clean && make ${JOBS} && make install) || die "ENet build failed"
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# NVTT - no install
echo -e "Building NVTT..."

pushd ../source/nvtt > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  rm -f .already-built
  rm -f lib/*.a
  pushd src
  rm -rf build
  mkdir -p build

  pushd build

  # Could use CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_SYSROOT
  # but they're not as flexible for cross-compiling
  # Disable optional libs that we don't need (avoids some conflicts with MacPorts)
  (cmake .. -DCMAKE_LINK_FLAGS="$LDFLAGS" -DCMAKE_C_FLAGS="$CFLAGS" -DCMAKE_CXX_FLAGS="$CXXFLAGS" -DCMAKE_BUILD_TYPE=Release -DBINDIR=bin -DLIBDIR=lib -DGLUT=0 -DGLEW=0 -DCG=0 -DCUDA=0 -DOPENEXR=0 -DJPEG=0 -DPNG=0 -DTIFF=0 -G "Unix Makefiles" && make clean && make nvtt ${JOBS}) || die "NVTT build failed"
  popd

  mkdir -p ../lib
  cp build/src/nv*/libnv*.a ../lib/
  cp build/src/nvtt/squish/libsquish.a ../lib/
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# FCollada - no install
echo -e "Building FCollada..."

pushd ../source/fcollada > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  rm -f .already-built
  rm -f lib/*.a
  pushd src
  rm -rf output
  mkdir -p ../lib

  # The Makefile refers to pkg-config for libxml2, but we
  # don't have that (replace with xml2-config instead)
  sed -i.bak -e 's/pkg-config libxml-2.0/xml2-config/' Makefile
  (make clean && CXXFLAGS=$CXXFLAGS make ${JOBS}) || die "FCollada build failed"
  # Undo Makefile change
  mv Makefile.bak Makefile
  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# MiniUPnPc - no install
echo -e "Building MiniUPnPc..."

pushd ../source/miniupnpc > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]]
then
  rm -f .already-built
  rm -f lib/*.a
  pushd src
  rm -rf output
  mkdir -p ../lib

  (make clean && CFLAGS=$CFLAGS LDFLAGS=$LDFLAGS make ${JOBS}) || die "MiniUPnPc build failed"

  cp libminiupnpc.a ../lib/

  popd
  touch .already-built
else
  already_built
fi
popd > /dev/null
