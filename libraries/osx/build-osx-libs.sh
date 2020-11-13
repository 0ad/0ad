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
CURL_VERSION="curl-7.59.0"
ICONV_VERSION="libiconv-1.15"
XML2_VERSION="libxml2-2.9.8"
SDL2_VERSION="SDL2-2.0.5"
# NOTE: remember to also update LIB_URL below when changing version
BOOST_VERSION="boost_1_64_0"
# NOTE: remember to also update LIB_URL below when changing version
WXWIDGETS_VERSION="wxWidgets-3.0.3.1"
# libpng was included as part of X11 but that's removed from Mountain Lion
# (also the Snow Leopard version was ancient 1.2)
PNG_VERSION="libpng-1.6.34"
OGG_VERSION="libogg-1.3.3"
VORBIS_VERSION="libvorbis-1.3.6"
# gloox requires GnuTLS, GnuTLS requires Nettle and GMP
GMP_VERSION="gmp-6.1.2"
NETTLE_VERSION="nettle-3.5.1"
# NOTE: remember to also update LIB_URL below when changing version
GNUTLS_VERSION="gnutls-3.6.13"
GLOOX_VERSION="gloox-1.0.22"
# OS X only includes part of ICU, and only the dylib
# NOTE: remember to also update LIB_URL below when changing version
ICU_VERSION="icu4c-59_2"
ENET_VERSION="enet-1.3.13"
MINIUPNPC_VERSION="miniupnpc-2.0.20180222"
SODIUM_VERSION="libsodium-1.0.18"
# --------------------------------------------------------------
# Bundled with the game:
# * SpiderMonkey 45
# * NVTT
# * FCollada
# --------------------------------------------------------------
# We use suffixes here in order to force rebuilding when patching these libs
SPIDERMONKEY_VERSION="mozjs-45.0.2+wildfiregames.2"
NVTT_VERSION="nvtt-2.1.1+wildfiregames.1"
FCOLLADA_VERSION="fcollada-3.05+wildfiregames.1"
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

# Define compiler as "clang", this is all Mavericks supports.
# gcc symlinks may still exist, but they are simply clang with
# slightly different config, which confuses build scripts.
# llvm-gcc and gcc 4.2 are no longer supported by SpiderMonkey.
export CC=${CC:="clang"} CXX=${CXX:="clang++"}
export MIN_OSX_VERSION=${MIN_OSX_VERSION:="10.9"}

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
# Force using libc++ since it has better C++11 support required by the game
# but pre-Mavericks still use libstdc++ by default
# Also enable c++0x for consistency with the game build
C_FLAGS="$C_FLAGS -arch $ARCH -fvisibility=hidden"
LDFLAGS="$LDFLAGS -arch $ARCH -stdlib=libc++"

CFLAGS="$CFLAGS $C_FLAGS"
CXXFLAGS="$CXXFLAGS $C_FLAGS -stdlib=libc++ -std=c++0x"
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

# --------------------------------------------------------------
echo -e "Building zlib..."

LIB_VERSION="${ZLIB_VERSION}"
LIB_ARCHIVE="$LIB_VERSION.tar.gz"
LIB_DIRECTORY=$LIB_VERSION
LIB_URL="http://zlib.net/"

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
LIB_URL="https://dl.bintray.com/boostorg/release/1.64.0/source/"

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
          --layout=tagged \
          --debug-configuration \
          link=static \
          threading=multi \
          variant=release,debug install \
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
LIB_URL="http://github.com/wxWidgets/wxWidgets/releases/download/v3.0.3.1/"

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
    --enable-macosx_arch=$ARCH
    --enable-unicode
    --with-cocoa
    --with-opengl
    --with-libiconv-prefix=${ICONV_DIR}
    --with-expat=builtin
    --with-libpng=builtin
    --without-libtiff
    --without-sdl
    --without-x
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
  (../configure CFLAGS="$CFLAGS" \
      CXXFLAGS="$CXXFLAGS" \
      CPPFLAGS="-stdlib=libc++ -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=1" \
      LDFLAGS="$LDFLAGS" $CONF_OPTS \
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

  (./configure CFLAGS="$CFLAGS" \
      LDFLAGS="$LDFLAGS" \
      --prefix=$INSTALL_DIR \
      --enable-shared=no \
    && make ${JOBS} && make install) || die "libpng build failed"
  popd
  echo "$LIB_VERSION" > .already-built
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
      --prefix="$INSTALL_DIR" \
      --enable-fat \
      --disable-shared \
      --with-pic \
    && make ${JOBS} && make install) || die "GMP build failed"
  popd
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
echo -e "Building Nettle..."

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

  # GnuTLS 3.6.8 added the TCP Fast Open feature, which requires connectx
  # but that's only available on OS X 10.11+ (GnuTLS doesn't support SDK based builds yet)
  # So we disable that functionality
  (patch -Np0 -i ../../patches/gnutls-disable-tcpfastopen.diff \
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
          --disable-nls \
    && make ${JOBS} LDFLAGS= install) || die "GnuTLS build failed"
  popd
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
LIB_URL="https://github.com/unicode-org/icu/releases/download/release-59-2/"

mkdir -p $LIB_DIRECTORY
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
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------------
# The following libraries are shared on different OSes and may
# be customized, so we build and install them from bundled sources
# --------------------------------------------------------------------
# SpiderMonkey - bundled, no download
echo -e "Building SpiderMonkey..."

LIB_VERSION="${SPIDERMONKEY_VERSION}"
LIB_DIRECTORY="mozjs-45.0.2"
LIB_ARCHIVE="$LIB_DIRECTORY.tar.bz2"

pushd ../source/spidermonkey/ > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  INSTALL_DIR="$(pwd)"
  INCLUDE_DIR_DEBUG=$INSTALL_DIR/include-unix-debug
  INCLUDE_DIR_RELEASE=$INSTALL_DIR/include-unix-release

  rm -f .already-built
  rm -f lib/*.a
  rm -rf $LIB_DIRECTORY $INCLUDE_DIR_DEBUG $INCLUDE_DIR_RELEASE
  tar -xf $LIB_ARCHIVE

  # Apply patches
  pushd $LIB_DIRECTORY
  . ../patch.sh
  popd

  pushd $LIB_DIRECTORY/js/src

  CONF_OPTS="--target=$ARCH-apple-darwin
    --prefix=${INSTALL_DIR}
    --enable-posix-nspr-emulation
    --with-system-zlib=${ZLIB_DIR}
    --disable-tests
    --disable-shared-js
    --disable-jemalloc
    --without-intl-api"
  # Change the default location where the tracelogger should store its output, which is /tmp/ on OSX.
  TLCXXFLAGS='-DTRACE_LOG_DIR="\"../../source/tools/tracelogger/\""'
  if [[ $MIN_OSX_VERSION && ${MIN_OSX_VERSION-_} ]]; then
    CONF_OPTS="$CONF_OPTS --enable-macos-target=$MIN_OSX_VERSION"
  fi
  if [[ $SYSROOT && ${SYSROOT-_} ]]; then
    CONF_OPTS="$CONF_OPTS --with-macosx-sdk=$SYSROOT"
  fi

  # We want separate debug/release versions of the library, so change their install name in the Makefile
  perl -i.bak -pe 's/(^STATIC_LIBRARY_NAME\s+=).*/$1'\''mozjs45-ps-debug'\''/' moz.build
  mkdir -p build-debug
  pushd build-debug
  (CC="clang" CXX="clang++" CXXFLAGS="${TLCXXFLAGS}" AR=ar CROSS_COMPILE=1 \
    ../configure $CONF_OPTS \
        --enable-debug \
        --disable-optimize \
        --enable-js-diagnostics \
        --enable-gczeal \
    && make ${JOBS}) || die "SpiderMonkey build failed"
  # js-config.h is different for debug and release builds, so we need different include directories for both
  mkdir -p $INCLUDE_DIR_DEBUG
  cp -R -L dist/include/* $INCLUDE_DIR_DEBUG/
  cp dist/sdk/lib/*.a $INSTALL_DIR/lib
  cp js/src/*.a $INSTALL_DIR/lib
  popd
  mv moz.build.bak moz.build

  perl -i.bak -pe 's/(^STATIC_LIBRARY_NAME\s+=).*/$1'\''mozjs45-ps-release'\''/' moz.build
  mkdir -p build-release
  pushd build-release
  (CC="clang" CXX="clang++" CXXFLAGS="${TLCXXFLAGS}" AR=ar CROSS_COMPILE=1 \
    ../configure $CONF_OPTS \
        --enable-optimize \
    && make ${JOBS}) || die "SpiderMonkey build failed"
  # js-config.h is different for debug and release builds, so we need different include directories for both
  mkdir -p $INCLUDE_DIR_RELEASE
  cp -R -L dist/include/* $INCLUDE_DIR_RELEASE/
  cp dist/sdk/lib/*.a $INSTALL_DIR/lib
  cp js/src/*.a $INSTALL_DIR/lib
  popd
  mv moz.build.bak moz.build
  
  popd
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# NVTT - bundled, no download
echo -e "Building NVTT..."

LIB_VERSION="${NVTT_VERSION}"

pushd ../source/nvtt > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
then
  rm -f .already-built
  rm -f lib/*.a
  pushd src
  rm -rf build
  mkdir -p build

  pushd build

  # Could use CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_SYSROOT
  # but they're not as flexible for cross-compiling
  # Disable png support (avoids some conflicts with MacPorts)
  (cmake .. \
      -DCMAKE_LINK_FLAGS="$LDFLAGS" \
      -DCMAKE_C_FLAGS="$CFLAGS" \
      -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBINDIR=bin \
      -DLIBDIR=lib \
      -DPNG=0 \
      -G "Unix Makefiles" \
    && make clean && make nvtt ${JOBS}) || die "NVTT build failed"
  popd

  mkdir -p ../lib
  cp build/src/bc*/libbc*.a ../lib/
  cp build/src/nv*/libnv*.a ../lib/
  cp build/src/nvtt/squish/libsquish.a ../lib/
  popd
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null

# --------------------------------------------------------------
# FCollada - bundled, no download
echo -e "Building FCollada..."

LIB_VERSION="${FCOLLADA_VERSION}"

pushd ../source/fcollada > /dev/null

if [[ "$force_rebuild" = "true" ]] || [[ ! -e .already-built ]] || [[ "$(<.already-built)" != "$LIB_VERSION" ]]
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
  echo "$LIB_VERSION" > .already-built
else
  already_built
fi
popd > /dev/null
