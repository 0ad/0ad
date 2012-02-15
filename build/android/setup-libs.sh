#!/bin/bash

set -e
set -o nounset

ANDROID=$HOME/android
NDK=$ANDROID/android-ndk-r7-crystax-4
SDK=$ANDROID/android-sdk-linux
TOOLCHAIN=$ANDROID/toolchain-0ad

build_toolchain=true

build_boost=true
build_curl=true
build_libpng=true
build_libjpeg=true
build_libxml2=true
build_enet=true
build_js185=true

JOBS=${JOBS:="-j4"}

SYSROOT=$TOOLCHAIN/sysroot
export PATH=$TOOLCHAIN/bin:$PATH
CFLAGS="-mandroid -mthumb -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp"

mkdir -p files
pushd files

if [ ! -e boost_1_45_0.tar.bz2 ]; then
  wget http://downloads.sourceforge.net/project/boost/boost/1.45.0/boost_1_45_0.tar.bz2
fi

if [ ! -e curl-7.24.0.tar.bz2 ]; then
  wget http://curl.haxx.se/download/curl-7.24.0.tar.bz2
fi

if [ ! -e MysticTreeGames-Boost-for-Android-70838fc.tar.gz ]; then
  wget https://github.com/MysticTreeGames/Boost-for-Android/tarball/70838fcfba930646cac724a77f5c626e930431f6 -O MysticTreeGames-Boost-for-Android-70838fc.tar.gz
fi

if [ ! -e enet-1.3.3.tar.gz ]; then
  wget http://enet.bespin.org/download/enet-1.3.3.tar.gz
fi

if [ ! -e js185-1.0.0.tar.gz ]; then
  cp ../../../libraries/spidermonkey/js185-1.0.0.tar.gz .
fi

if [ ! -e libjpeg-turbo-1.1.1.tar.gz ]; then
  wget http://downloads.sourceforge.net/project/libjpeg-turbo/1.1.1/libjpeg-turbo-1.1.1.tar.gz
fi

if [ ! -e libpng-1.5.8.tar.xz ]; then
  wget http://prdownloads.sourceforge.net/libpng/libpng-1.5.8.tar.xz
fi

if [ ! -e libxml2-2.7.8.tar.gz ]; then
  wget ftp://xmlsoft.org/libxml2/libxml2-2.7.8.tar.gz
fi

popd


if [ "$build_toolchain" = "true" ]; then

  rm -r $TOOLCHAIN || true
  $NDK/build/tools/make-standalone-toolchain.sh --platform=android-9 --toolchain=arm-linux-androideabi-4.4.3 --install-dir=$TOOLCHAIN

  mkdir -p $SYSROOT/usr/local
  
  # Set up some symlinks to make the SpiderMonkey build system happy
  ln -sfT ../platforms $NDK/build/platforms
  for f in $TOOLCHAIN/bin/arm-linux-androideabi-*; do
    ln -sf $f ${f/arm-linux-androideabi-/arm-eabi-}
  done

  # Set up some symlinks for the typical autoconf-based build systems
  for f in $TOOLCHAIN/bin/arm-linux-androideabi-*; do
    ln -sf $f ${f/arm-linux-androideabi-/arm-linux-eabi-}
  done

fi

mkdir -p temp

if [ "$build_boost" = "true" ]; then
  rm -rf temp/MysticTreeGames-Boost-for-Android-70838fc
  tar xvf files/MysticTreeGames-Boost-for-Android-70838fc.tar.gz -C temp/
  cp files/boost_1_45_0.tar.bz2 temp/MysticTreeGames-Boost-for-Android-70838fc/
  patch temp/MysticTreeGames-Boost-for-Android-70838fc/build-android.sh < boost-android-build.patch
  pushd temp/MysticTreeGames-Boost-for-Android-70838fc
  ./build-android.sh $TOOLCHAIN
  cp -rv build/{include,lib} $SYSROOT/usr/local/
  popd
fi

if [ "$build_curl" = "true" ]; then
  rm -rf temp/curl-7.24.0
  tar xvf files/curl-7.24.0.tar.bz2 -C temp/
  pushd temp/curl-7.24.0
  ./configure --host=arm-linux-androideabi --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local CFLAGS="$CFLAGS" --disable-shared
  make -j3
  make install
  popd
fi

if [ "$build_libpng" = "true" ]; then
  rm -rf temp/libpng-1.5.7
  tar xvf files/libpng-1.5.7.tar.xz -C temp/
  pushd temp/libpng-1.5.7
  ./configure --host=arm-linux-eabi --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local CFLAGS="$CFLAGS"
  make $JOBS
  make install
  popd
fi

if [ "$build_libjpeg" = "true" ]; then
  rm -rf temp/libjpeg-turbo-1.1.1
  tar xvf files/libjpeg-turbo-1.1.1.tar.gz -C temp/
  pushd temp/libjpeg-turbo-1.1.1
  ./configure --host=arm-linux-eabi --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local CFLAGS="$CFLAGS"
  make $JOBS
  make install
  popd
fi

if [ "$build_libxml2" = "true" ]; then
  rm -rf temp/libxml2-2.7.8
  tar xvf files/libxml2-2.7.8.tar.gz -C temp/
  patch temp/libxml2-2.7.8/Makefile.in < libxml2-android-build.patch
  pushd temp/libxml2-2.7.8
  ./configure --host=arm-linux-eabi --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local CFLAGS="$CFLAGS"
  make $JOBS
  make install
  popd
fi

if [ "$build_enet" = "true" ]; then
  rm -rf temp/enet-1.3.3
  tar xvf files/enet-1.3.3.tar.gz -C temp/
  pushd temp/enet-1.3.3
  ./configure --host=arm-linux-eabi --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local CFLAGS="$CFLAGS"
  make $JOBS
  make install
  popd
fi

if [ "$build_js185" = "true" ]; then
  rm -rf temp/js-1.8.5
  tar xvf files/js185-1.0.0.tar.gz -C temp/
  patch temp/js-1.8.5/js/src/assembler/wtf/Platform.h < js185-android-build.patch
  pushd temp/js-1.8.5/js/src
  CXXFLAGS="-I $TOOLCHAIN/arm-linux-androideabi/include/c++/4.4.3/arm-linux-androideabi" \
  HOST_CXXFLAGS="-DFORCE_LITTLE_ENDIAN" \
  ./configure \
    --prefix=$SYSROOT/usr/local \
    --target=arm-android-eabi \
    --with-android-ndk=$NDK \
    --with-android-sdk=$SDK \
    --with-android-toolchain=$TOOLCHAIN \
    --with-android-version=9 \
    --disable-tests \
    --disable-shared-js \
    --with-arm-kuser \
    CFLAGS="$CFLAGS"
  make $JOBS JS_DISABLE_SHELL=1
  make install JS_DISABLE_SHELL=1
  popd
fi
