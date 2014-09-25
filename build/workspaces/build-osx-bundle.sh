#!/bin/sh
#
# This script will build an OSX app bundle for 0 A.D.
#
# App bundles are intended to be self-contained and portable.
# An SDK is required, usually included with Xcode. The SDK ensures
# that only those system libraries are used which are available on
# the chosen target and compatible systems.
#
# Steps to build a 0 A.D. bundle are:
# 1. confirm ARCH is set to desired target architecture
# 2. confirm SYSROOT points to the correct target SDK
# 3. confirm MIN_OSX_VERSION matches the target OS X version
# 4. update BUNDLE_VERSION to match current 0 A.D. version
# 5. run this script
#

# Force build architecture, as sometimes environment is broken.
# For a universal fat binary, the approach would be to build every
# dependency with both archs and combine them with lipo, then do the
# same thing with the game itself.
# Choices are "x86_64" or  "i386" (ppc and ppc64 not supported)
export ARCH=${ARCH:="x86_64"}

# Set SDK and mimimum required OSX version
# (As of Xcode 4.3, the SDKs are located directly in Xcode.app,
#  but previously they were in /Developer/SDKs)
#export SYSROOT=${SYSROOT="/Developer/SDKs/MacOSX10.5.sdk"}
export SYSROOT=${SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk"}
export MIN_OSX_VERSION=${MIN_OSX_VERSION="10.7"}

# 0 A.D. release version, e.g. Alpha 17 is 0.0.17
BUNDLE_VERSION=${BUNDLE_VERSION:="0.0.0"}

# Define compiler as "clang", this is all Mavericks supports.
# gcc symlinks may still exist, but they are simply clang with
# slightly different config, which confuses build scripts.
# llvm-gcc and gcc 4.2 are no longer supported by SpiderMonkey.
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

# Unique identifier string for this bundle (reverse-DNS style)
BUNDLE_IDENTIFIER=${BUNDLE_IDENTIFIER:="com.wildfiregames.0ad"}
# Minimum version of OSX on which the bundle will run
BUNDLE_MIN_OSX_VERSION="${MIN_OSX_VERSION}.0"

die()
{
  echo ERROR: $*
  exit 1
}

# Check that we're actually on OS X
if [ "`uname -s`" != "Darwin" ]; then
  die "This script is intended for OS X only"
fi
  
# Check SDK exists
if [ ! -d "$SYSROOT" ]; then
  die "$SYSROOT does not exist! You probably need to install Xcode"
fi

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

JOBS=${JOBS:="-j5"}
build_release=false
build_path="$HOME/0ad-export"

# Parse command-line options:
for i in "$@"
do
  case $i in
    -j* ) JOBS=$i ;;
    --release ) build_release=true ;;
  esac
done

# For release, export an SVN copy
if [ "$build_release" = "true" ]; then
  BUNDLE_OUTPUT="$build_path/0ad.app"
  SVN_REV=`svnversion -n ../..`
  rm -rf $build_path
  svn export ../.. $build_path || die "Error exporting SVN working directory"
  cd $build_path
  rm binaries/data/config/dev.cfg
  # Only include translations for a subset of languages
  find binaries/data -name "*.po" | grep -v '.*/\(ca\|cs\|de\|en_GB\|es\|fr\|gd\|gl\|it\|nl\|pt_PT\|pt_BR\)\.[A-Za-z0-9_.]\+\.po' | xargs rm
  echo L\"${SVNREV}-release\" > build/svn_revision/svn_revision.txt
  cd build/workspaces
fi

BUNDLE_OUTPUT=${BUNDLE_OUTPUT:="$(pwd)/0ad.app"}
BUNDLE_CONTENTS=$BUNDLE_OUTPUT/Contents
BUNDLE_BIN=$BUNDLE_CONTENTS/MacOS
BUNDLE_RESOURCES=$BUNDLE_CONTENTS/Resources
BUNDLE_FRAMEWORKS=$BUNDLE_CONTENTS/Frameworks
BUNDLE_PLUGINS=$BUNDLE_CONTENTS/PlugIns
BUNDLE_SHAREDSUPPORT=$BUNDLE_CONTENTS/SharedSupport

# TODO: Do we really want to regenerate everything? (consider if one task fails)

# Build libraries against SDK
echo "\nBuilding libaries\n"
pushd ../../libraries/osx
./build-osx-libs.sh $JOBS --force-rebuild || die "Libraries build script failed"
popd

# Clean and update workspaces
echo "\nGenerating workspaces\n"

# Pass OSX options through to Premake
(./clean-workspaces.sh && SYSROOT="$SYSROOT" MIN_OSX_VERSION="$MIN_OSX_VERSION" ./update-workspaces.sh --macosx-bundle="$BUNDLE_IDENTIFIER" --sysroot="$SYSROOT" --macosx-version-min="$MIN_OSX_VERSION") || die "update-workspaces.sh failed!"

pushd gcc
echo "\nBuilding game\n"
(make clean && CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" make ${JOBS}) || die "Game build failed!"
popd

# Run test to confirm all is OK
pushd ../../binaries/system
echo "\nRunning tests\n"
./test || die "Post-build testing failed!"
popd

# Create bundle structure
echo "\nCreating bundle directories\n"
rm -rf ${BUNDLE_OUTPUT}
mkdir -p ${BUNDLE_BIN}
mkdir -p ${BUNDLE_FRAMEWORKS}
mkdir -p ${BUNDLE_PLUGINS}
mkdir -p ${BUNDLE_RESOURCES}
mkdir -p ${BUNDLE_SHAREDSUPPORT}

# Build archive(s) - don't archive the _test.* mods
pushd ../../binaries/data/mods
archives=""
for modname in [a-zA-Z0-9]*
do
  archives="${archives} ${modname}"
done
popd

pushd ../../binaries/system

for modname in $archives
do
  echo "\nBuilding archive for '${modname}'\n"
  ARCHIVEBUILD_INPUT="$(pwd)/../data/mods/${modname}"
  ARCHIVEBUILD_OUTPUT="${BUNDLE_RESOURCES}/data/mods/${modname}"

  # For some reason the output directory has to exist?
  mkdir -p ${ARCHIVEBUILD_OUTPUT}

  (./pyrogenesis -archivebuild=${ARCHIVEBUILD_INPUT} -archivebuild-output=${ARCHIVEBUILD_OUTPUT}/${modname}.zip) || die "Archive build for '${modname}' failed!"
done

# Copy binaries
echo "\nCopying binaries\n"
# Only pyrogenesis for now, until we find a way to load
#   multiple binaries from one app bundle
# TODO: Would be nicer if we could set this path in premake
cp pyrogenesis ${BUNDLE_BIN}

# Copy libs
echo "\nCopying libs\n"
# TODO: Would be nicer if we could set this path in premake
cp -v libAtlasUI.dylib ${BUNDLE_FRAMEWORKS}
cp -v libCollada.dylib ${BUNDLE_FRAMEWORKS}

popd

# Copy data
echo "\nCopying non-archived game data\n"
pushd ../../binaries/data
if [ "$build_release" = "false"]; then
  mv config/dev.cfg config/dev.bak
fi
cp -R -v config ${BUNDLE_RESOURCES}/data/
cp -R -v tools ${BUNDLE_RESOURCES}/data/
if [ "$build_release" = "false"]; then
  mv config/dev.bak config/dev.cfg
fi
popd
cp -v ../resources/0ad.icns ${BUNDLE_RESOURCES}

# Copy license/readmes
# TODO: Also want copies in the DMG - decide on layout
echo "\nCopying readmes\n"
cp -v ../../*.txt ${BUNDLE_RESOURCES}

# Create Info.plist
echo "\nCreating Info.plist\n"
alias PlistBuddy=/usr/libexec/PlistBuddy
INFO_PLIST="${BUNDLE_CONTENTS}/Info.plist"

PlistBuddy -c "Add :CFBundleName string 0 A.D." ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleIdentifier string ${BUNDLE_IDENTIFIER}" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleVersion string ${BUNDLE_VERSION}" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundlePackageType string APPL" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleSignature string none" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleExecutable string pyrogenesis" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleShortVersionString string ${BUNDLE_VERSION}" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleDevelopmentRegion string English" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleInfoDictionaryVersion string 6.0" ${INFO_PLIST}
PlistBuddy -c "Add :CFBundleIconFile string 0ad" ${INFO_PLIST}
PlistBuddy -c "Add :LSMinimumSystemVersion string ${BUNDLE_MIN_OSX_VERSION}" ${INFO_PLIST}
PlistBuddy -c "Add :NSHumanReadableCopyright string Copyright © 2014 Wildfire Games" ${INFO_PLIST}

# TODO: Automatically create compressed DMG with hdiutil?
# (this is a bit complicated so I do it manually for now)
# (also we need to have good icon placement, background image, etc)

echo "\nBundle complete! Located in ${BUNDLE_OUTPUT}"
