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
ZLIB_VERSION="zlib-1.2.11"
CURL_VERSION="curl-7.71.0"
ICONV_VERSION="libiconv-1.16"
XML2_VERSION="libxml2-2.9.10"
SDL2_VERSION="SDL2-2.0.18"
# NOTE: remember to also update LIB_URL below when changing version
BOOST_VERSION="boost_1_76_0"
# NOTE: remember to also update LIB_URL below when changing version
WXWIDGETS_VERSION="wxWidgets-3.1.4"
# libpng was included as part of X11 but that's removed from Mountain Lion
# (also the Snow Leopard version was ancient 1.2)
PNG_VERSION="libpng-1.6.36"
FREETYPE_VERSION="freetype-2.10.4"
OGG_VERSION="libogg-1.3.3"
VORBIS_VERSION="libvorbis-1.3.7"
# gloox requires GnuTLS, GnuTLS requires Nettle and GMP
GMP_VERSION="gmp-6.2.1"
NETTLE_VERSION="nettle-3.6"
# NOTE: remember to also update LIB_URL below when changing version
GLOOX_VERSION="gloox-1.0.24"
GNUTLS_VERSION="gnutls-3.6.15"
# OS X only includes part of ICU, and only the dylib
# NOTE: remember to also update LIB_URL below when changing version
ICU_VERSION="icu4c-69_1"
ENET_VERSION="enet-1.3.17"
MINIUPNPC_VERSION="miniupnpc-2.2.2"
SODIUM_VERSION="libsodium-1.0.18"
FMT_VERSION="7.1.3"
# --------------------------------------------------------------
# Bundled with the game:
# * SpiderMonkey
# * NVTT
# * FCollada

# Provided by OS X:
# * OpenAL
# * OpenGL
# --------------------------------------------------------------

export CC=${CC:="clang"} CXX=${CXX:="clang++"}
export MIN_OSX_VERSION=${MIN_OSX_VERSION:="10.12"}
export ARCH=${ARCH:=""}

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

CFLAGS="$CFLAGS $C_FLAGS -fvisibility=hidden"
CXXFLAGS="$CXXFLAGS $C_FLAGS -stdlib=libc++ -std=c++17"
OBJCFLAGS="$OBJCFLAGS $C_FLAGS"
OBJCXXFLAGS="$OBJCXXFLAGS $C_FLAGS"

# Force x86_64 architecture on MacOS for now.
# NB: annoyingly, this is rather unstandardised. Some libs expect -arch, others different things.
# Further: wxWidgets uses its own system and actually fails to compile with arch arguments.
ARCHLESS_CFLAGS=$CFLAGS
ARCHLESS_CXXFLAGS=$CXXFLAGS
ARCHLESS_LDFLAGS="$LDFLAGS -stdlib=libc++"

# If ARCH isn't set, select either x86_64 or arm64
if [ -z "${ARCH}" ]; then
  ARCH=`uname -m`
fi
if [ $ARCH == "arm64" ]; then
  # Some libs want this passed to configure for cross compilation.
  HOST_PLATFORM="--host=aarch64-apple-darwin"
else
  CXXFLAGS="$CXXFLAGS -msse4.1"
  # Some libs want this passed to configure for cross compilation.
  HOST_PLATFORM="--host=x86_64-apple-darwin"
fi

CFLAGS="$CFLAGS -arch $ARCH"
CXXFLAGS="$CXXFLAGS -arch $ARCH"

LDFLAGS="$LDFLAGS -arch $ARCH"

# CMake doesn't seem to pick up on architecture with CFLAGS only
CMAKE_FLAGS="-DCMAKE_OSX_ARCHITECTURES=$ARCH -DCMAKE_OSX_DEPLOYMENT_TARGET=$MIN_OSX_VERSION"

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
    curl -fLo ${filename} ${url}${filename} || die "Download of $url$filename failed"
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

# Create a location to create copies of dependencies' *.pc files, so they can be found by pkg-config
PC_PATH="$(pwd)/pkgconfig/"
if [[ "$force_rebuild" = "true" ]]; then
  rm -rf $PC_PATH
fi
mkdir -p $PC_PATH

# --------------------------------------------------------------
echo -e "Building zlib..."

LIB_VERSION="${ZLIB_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="https://zlib.net/fossils/"

mkdir -p zlib
pushd zlib > /dev/null

ZLIB_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # patch zlib's configure script to use our CFLAGS and LDFLAGS
  (patch -Np0 -i ../../patches/zlib_flags.diff \
    && CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
      ./configure --prefix="$ZLIB_DIR" \
         --static \
    && make ${JOBS} && make install) || die "zlib build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
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

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix="$INSTALL_DIR" \
      --enable-ipv6 \
      --with-darwinssl \
      --without-gssapi \
      --without-libmetalink \
      --without-libpsl \
      --without-librtmp \
      --without-libssh2 \
      --without-nghttp2 \
      --without-nss \
      --without-polarssl \
      --without-ssl \
      --without-gnutls \
      --without-brotli \
      --without-cyassl \
      --without-winssl \
      --without-mbedtls \
      --without-wolfssl \
      --without-spnego \
      --disable-ares \
      --disable-ldap \
      --disable-ldaps \
      --without-libidn2 \
      --with-zlib="${ZLIB_DIR}" \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libcurl build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
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

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix="$ICONV_DIR" \
      --without-libiconv-prefix \
      --without-libintl-prefix \
      --disable-nls \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libiconv build failed"
  popd
  echo "$LIB_VERSION" > .already-built
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

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix="$INSTALL_DIR" \
      --without-lzma \
      --without-python \
      --with-iconv="${ICONV_DIR}" \
      --with-zlib="${ZLIB_DIR}" \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libxml2 build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------

echo -e "Building SDL2..."

LIB_VERSION="${SDL2_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="https://libsdl.org/release/"

mkdir -p sdl2
pushd sdl2 > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # We don't want SDL2 to pull in system iconv, force it to detect ours with flags.
  # Don't use X11 - we don't need it and Mountain Lion removed it
  (./configure CPPFLAGS="-I${ICONV_DIR}/include" \
      CFLAGS="$CFLAGS" \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS -L${ICONV_DIR}/lib" \
      --prefix="$INSTALL_DIR" \
      --disable-video-x11 \
      --without-x \
      --enable-video-cocoa \
      --enable-shared=no \
    && make $JOBS && make install) || die "SDL2 build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building Boost..."

LIB_VERSION="${BOOST_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/"

mkdir -p boost
pushd boost > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # Can't use macosx-version, see above comment.
  (./bootstrap.sh --with-libraries=filesystem,system \
      --prefix=$INSTALL_DIR \
    && ./b2 cflags="$CFLAGS" \
          toolset=clang \
          cxxflags="$CXXFLAGS" \
          linkflags="$LDFLAGS" ${JOBS} \
          -d2 \
          --layout=system \
          --debug-configuration \
          link=static \
          threading=multi \
          variant=release install \
    ) || die "Boost build failed"

  popd
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# TODO: This build takes ages, anything we can exclude?
echo -e "Building wxWidgets..."

LIB_VERSION="${WXWIDGETS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/"

mkdir -p wxwidgets
pushd wxwidgets > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  mkdir -p build-release
  pushd build-release

  CONF_OPTS="--prefix=$INSTALL_DIR
    --disable-shared
    --enable-unicode
    --with-cocoa
    --with-opengl
    --with-libiconv-prefix=${ICONV_DIR}
    --enable-universal-binary=${ARCH}
    --with-expat=builtin
    --with-libpng=builtin
    --without-libtiff
    --without-sdl
    --without-x
    --disable-stc
    --disable-webview
    --disable-webkit
    --disable-webviewwebkit
    --disable-webviewie
    --without-libjpeg"
  # wxWidgets configure now defaults to targeting 10.5, if not specified,
  # but that conflicts with our flags
  if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
    CONF_OPTS="$CONF_OPTS --with-macosx-version-min=$MIN_OSX_VERSION"
  fi
  (../configure CFLAGS="$ARCHLESS_CFLAGS" \
      CXXFLAGS="$ARCHLESS_CXXFLAGS" \
      CPPFLAGS="-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=1" \
      LDFLAGS="$ARCHLESS_LDFLAGS" $CONF_OPTS \
    && make ${JOBS} && make install) || die "wxWidgets build failed"
  popd
  popd
  echo "$LIB_VERSION" > .already-built
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

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # libpng has no flags for zlib but the 10.12 version is too old, so link our own.
  (./configure CFLAGS="$CFLAGS" CPPFLAGS=" -I $ZLIB_DIR/include "\
      LDFLAGS="$LDFLAGS -L$ZLIB_DIR/lib" \
      --prefix=$INSTALL_DIR \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libpng build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building freetype..."

LIB_VERSION="${FREETYPE_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://download.savannah.gnu.org/releases/freetype/"

mkdir -p freetype
pushd freetype > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" \
      --prefix=$INSTALL_DIR \
      "$HOST_PLATFORM" \
      --enable-shared=no \
       --with-harfbuzz=no \
       --with-bzip2=no \
       --with-brotli=no \
    && make ${JOBS} && make install) || die "freetype build failed"
  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# Dependency of vorbis
echo -e "Building libogg..."

LIB_VERSION="${OGG_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://downloads.xiph.org/releases/ogg/"

mkdir -p libogg
pushd libogg > /dev/null
OGG_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix=$OGG_DIR \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libogg build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
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

mkdir -p vorbis
pushd vorbis > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix="$INSTALL_DIR" \
      --enable-shared=no \
      --with-ogg="$OGG_DIR" \
    && make ${JOBS} && make install) || die "libvorbis build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building GMP..."

LIB_VERSION="${GMP_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.bz2"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://gmplib.org/download/gmp/"

mkdir -p gmp
pushd gmp > /dev/null

GMP_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # NOTE: enable-fat in this case allows building and running on different CPUS.
  # Otherwise CPU-specific instructions will be used with no fallback for older CPUs.
  (./configure CFLAGS="$CFLAGS" \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS" \
      "$HOST_PLATFORM" \
      --prefix="$INSTALL_DIR" \
      --enable-fat \
      --disable-shared \
      --with-pic \
    && make ${JOBS} && make install) || die "GMP build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building Nettle..."
# Also builds hogweed

LIB_VERSION="${NETTLE_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://ftp.gnu.org/gnu/nettle/"

mkdir -p nettle
pushd nettle > /dev/null

NETTLE_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # NOTE: enable-fat in this case allows building and running on different CPUS.
  # Otherwise CPU-specific instructions will be used with no fallback for older CPUs.
  (./configure CFLAGS="$CFLAGS" \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --with-include-path="${GMP_DIR}/include" \
      --with-lib-path="${GMP_DIR}/lib" \
      --prefix="$INSTALL_DIR" \
      --enable-fat \
      --disable-shared \
      --disable-documentation \
      --disable-openssl \
      --disable-assembler \
    && make ${JOBS} && make install) || die "Nettle build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building GnuTLS..."

LIB_VERSION="${GNUTLS_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.xz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://www.gnupg.org/ftp/gcrypt/gnutls/v3.6/"

mkdir -p gnutls
pushd gnutls > /dev/null

GNUTLS_DIR="$(pwd)"

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # Patch GNUTLS for a linking issue with isdigit
  # Patch by Ross Nicholson: https://gitlab.com/gnutls/gnutls/-/issues/1033#note_379529145
  (patch -Np1 -i ../../patches/03-undo-libtasn1-cisdigit.patch \
    && ./configure CFLAGS="$CFLAGS" \
          CXXFLAGS="$CXXFLAGS" \
          LDFLAGS="$LDFLAGS" \
          LIBS="-L${GMP_DIR}/lib -lgmp" \
          NETTLE_CFLAGS="-I${NETTLE_DIR}/include" \
          NETTLE_LIBS="-L${NETTLE_DIR}/lib -lnettle" \
          HOGWEED_CFLAGS="-I${NETTLE_DIR}/include" \
          HOGWEED_LIBS="-L${NETTLE_DIR}/lib -lhogweed" \
          GMP_CFLAGS="-I${GMP_DIR}/include" \
          GMP_LIBS="-L${GMP_DIR}/lib -lgmp" \
          --prefix="$INSTALL_DIR" \
          --enable-shared=no \
          --without-idn \
          --with-included-unistring \
          --with-included-libtasn1 \
          --without-p11-kit \
          --disable-tests \
          --disable-guile \
          --disable-doc \
          --disable-tools \
          --disable-nls \
    && make ${JOBS} LDFLAGS= install) || die "GnuTLS build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
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

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # TODO: pulls in libresolv dependency from /usr/lib
  (./configure CFLAGS="$CFLAGS" \
      CXXFLAGS="$CXXFLAGS" \
      LDFLAGS="$LDFLAGS" \
      "$HOST_PLATFORM" \
      --prefix="$INSTALL_DIR" \
      GNUTLS_CFLAGS="-I${GNUTLS_DIR}/include" \
      GNUTLS_LIBS="-L${GNUTLS_DIR}/lib -lgnutls" \
      --enable-shared=no \
      --with-zlib="${ZLIB_DIR}" \
      --without-libidn \
      --with-gnutls="yes" \
      --without-openssl \
      --without-tests \
      --without-examples \
      --disable-getaddrinfo \
    && make ${JOBS} && make install) || die "gloox build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building ICU..."

LIB_VERSION="${ICU_VERSION}"
LIB_ARCHIVE="$LIB_VERSION-src.tgz"
LIB_DIRECTORY="icu"
LIB_URL="https://github.com/unicode-org/icu/releases/download/release-69-1/"

mkdir -p icu
pushd icu > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib sbin share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  mkdir -p source/build
  pushd source/build

  (CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" \
    ../runConfigureICU MacOSX \
        "$HOST_PLATFORM" \
        --prefix=$INSTALL_DIR \
        --disable-shared \
        --enable-static \
        --disable-samples \
        --enable-extras \
        --enable-icuio \
        --enable-tools \
    && make ${JOBS} && make install) || die "ICU build failed"

  popd
  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building ENet..."

LIB_VERSION="${ENET_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://enet.bespin.org/download/"

mkdir -p enet
pushd enet > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib sbin share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix=${INSTALL_DIR} \
      --enable-shared=no \
    && make clean && make ${JOBS} && make install) || die "ENet build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building MiniUPnPc..."

LIB_VERSION="${MINIUPNPC_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="http://miniupnp.tuxfamily.org/files/download.php?file="

mkdir -p miniupnpc
pushd miniupnpc > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY bin include lib share
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (make clean \
    && CFLAGS=$CFLAGS LDFLAGS=$LDFLAGS make ${JOBS} \
    && INSTALLPREFIX="$INSTALL_DIR" make install \
  ) || die "MiniUPnPc build failed"

  popd
  # TODO: how can we not build the dylibs?
  rm -f lib/*.dylib
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building libsodium..."

LIB_VERSION="${SODIUM_VERSION}"
LIB_ARCHIVE="$SODIUM_VERSION.tar.gz"
LIB_DIRECTORY="$LIB_VERSION"
LIB_URL="https://download.libsodium.org/libsodium/releases/"

mkdir -p libsodium
pushd libsodium > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"

  rm -f .already-built
  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix=${INSTALL_DIR} \
      --enable-shared=no \
    && make clean \
    && CFLAGS=$CFLAGS LDFLAGS=$LDFLAGS make ${JOBS} \
    && make check \
    && INSTALLPREFIX="$INSTALL_DIR" make install \
  ) || die "libsodium build failed"

  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building fmt..."

LIB_DIRECTORY="fmt-$FMT_VERSION"
LIB_ARCHIVE="$FMT_VERSION.tar.gz"
LIB_URL="https://github.com/fmtlib/fmt/archive/"

mkdir -p fmt
pushd fmt > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$FMT_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"
  rm -f .already-built

  download_lib $LIB_URL $LIB_ARCHIVE

  rm -rf $LIB_DIRECTORY include lib
  tar -xf $LIB_ARCHIVE
  pushd $LIB_DIRECTORY

  # It appears that older versions of Clang require constexpr statements to have a user-set constructor.
  patch -Np1 -i ../../patches/fmt_constexpr.diff

  mkdir -p build
  pushd build

  (cmake .. \
      -DFMT_TEST=False \
      -DFMT_DOC=False \
      -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
      $CMAKE_FLAGS \
    && make fmt ${JOBS} && make install) || die "fmt build failed"

  popd
  popd
  cp -f lib/pkgconfig/* $PC_PATH
  echo "$FMT_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------------
# The following libraries are shared on different OSes and may
# be customized, so we build and install them from bundled sources
# --------------------------------------------------------------------
# SpiderMonkey - bundled, no download
pushd ../source/spidermonkey/ > /dev/null

if [[ "$force_rebuild" = "true" ]]
then
  rm -f .already-built
fi

# Use the regular build script for SM.
JOBS="$JOBS" ZLIB_DIR="$ZLIB_DIR" ARCH="$ARCH" ./build.sh || die "Error building spidermonkey"

popd > /dev/null

# --------------------------------------------------------------
# NVTT - bundled, no download
pushd ../source/nvtt > /dev/null

if [[ "$force_rebuild" = "true" ]]
then
  rm -f .already-built
fi

CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" CMAKE_FLAGS=$CMAKE_FLAGS JOBS="$JOBS" ./build.sh || die "Error building NVTT"

popd > /dev/null

# --------------------------------------------------------------
# FCollada - bundled, no download
pushd ../source/fcollada/ > /dev/null

if [[ "$force_rebuild" = "true" ]]
then
  rm -f .already-built
fi

CXXFLAGS="$CXXFLAGS" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS" JOBS="$JOBS" ./build.sh || die "Error building FCollada"

popd > /dev/null
