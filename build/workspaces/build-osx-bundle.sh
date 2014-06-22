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
# 5. if building 32-bit 10.5 bundle, read the accompanying documentation
# 6. run this script
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
# TODO: we could get this from xcode-select but the user must set that up
#export SYSROOT=${SYSROOT="/Developer/SDKs/MacOSX10.5.sdk"}
export SYSROOT=${SYSROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk"}
export MIN_OSX_VERSION=${MIN_OSX_VERSION="10.8"}

# 0 A.D. release version, e.g. Alpha 12 is 0.0.12
BUNDLE_VERSION=${BUNDLE_VERSION:="0.0.0"}

# Define compiler as "gcc" (in case anything expects e.g. gcc-4.2)
# On newer OS X versions, this will be a symbolic link to LLVM GCC
# TODO: don't rely on that
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

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

BUNDLE_OUTPUT=${BUNDLE_OUTPUT:="$(pwd)/0ad.app"}
BUNDLE_CONTENTS=$BUNDLE_OUTPUT/Contents
BUNDLE_BIN=$BUNDLE_CONTENTS/MacOS
BUNDLE_RESOURCES=$BUNDLE_CONTENTS/Resources
BUNDLE_FRAMEWORKS=$BUNDLE_CONTENTS/Frameworks
BUNDLE_PLUGINS=$BUNDLE_CONTENTS/PlugIns
BUNDLE_SHAREDSUPPORT=$BUNDLE_CONTENTS/SharedSupport

# Unique identifier string for this bundle (reverse-DNS style)
BUNDLE_IDENTIFIER=${BUNDLE_IDENTIFIER:="com.wildfiregames.0ad"}
# Minimum version of OSX on which the bundle will run
BUNDLE_MIN_OSX_VERSION="${MIN_OSX_VERSION}.0"

JOBS=${JOBS:="-j5"}

# Parse command-line options:
for i in "$@"
do
  case $i in
    -j* ) JOBS=$i ;;
  esac
done

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

# Get SVN revision and update svn_revision.txt
SVNREV=`svnversion -n .`
echo L\"${SVNREV}-release\" > ../svn_revision/svn_revision.txt

pushd gcc
echo "\nBuilding game\n"
(make clean && CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" make ${JOBS}) || die "Game build failed!"
popd

# TODO: This is yucky - should we first export our working copy, like source\tools\dist\build.sh?
svn revert ../svn_revision/svn_revision.txt

# Create bundle structure
echo "\nCreating bundle directories\n"
rm -rf ${BUNDLE_OUTPUT}
mkdir -p ${BUNDLE_BIN}
mkdir -p ${BUNDLE_FRAMEWORKS}
mkdir -p ${BUNDLE_PLUGINS}
mkdir -p ${BUNDLE_RESOURCES}
mkdir -p ${BUNDLE_SHAREDSUPPORT}

pushd ../../binaries/system
# Run test to confirm all is OK
echo "\nRunning tests\n"
./test || die "Test(s) failed!"

# Build archive(s) - don't archive the _test.* mods
pushd ../data/mods
archives=""
for modname in [a-zA-Z0-9]*
do
  archives="${archives} ${modname}"
done
popd

for modname in $archives
do
  echo "\nBuilding archive for '${modname}'\n"
  ARCHIVEBUILD_INPUT="$(pwd)/../data/mods/${modname}"
  ARCHIVEBUILD_OUTPUT="${BUNDLE_RESOURCES}/data/mods/${modname}"

  # For some reason the output directory has to exist?
  mkdir -p ${ARCHIVEBUILD_OUTPUT}

  (./pyrogenesis -archivebuild=${ARCHIVEBUILD_INPUT} -archivebuild-output=${ARCHIVEBUILD_OUTPUT}/${modname}.zip) || die "Archive build for '${modname}' failed!"
done
popd

# Copy binaries
echo "\nCopying binaries\n"
# Only pyrogenesis for now, until we find a way to load
#   multiple binaries from one app bundle
# TODO: Would be nicer if we could set this path in premake
cp ../../binaries/system/pyrogenesis ${BUNDLE_BIN}

# Copy libs
echo "\nCopying libs\n"
# TODO: Should probably make it so these libs get built in place
cp -v ../../binaries/system/libAtlasUI.dylib ${BUNDLE_FRAMEWORKS}
cp -v ../../binaries/system/libCollada.dylib ${BUNDLE_FRAMEWORKS}

# Copy data
echo "\nCopying non-archived game data\n"
# Removing it now and restoring it later, cp has no exclusion switch
# and using find is a bit over-the-top
rm ../../binaries/data/config/dev.cfg
cp -v ../resources/0ad.icns ${BUNDLE_RESOURCES}
cp -R -v ../../binaries/data/config ${BUNDLE_RESOURCES}/data/
cp -R -v ../../binaries/data/tools ${BUNDLE_RESOURCES}/data/
svn revert ../../binaries/data/config/dev.cfg

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
PlistBuddy -c "Add :NSHumanReadableCopyright string Copyright © 2013 Wildfire Games" ${INFO_PLIST}

# TODO: Automatically create compressed DMG with hditutil?
# (this is a bit complicated so I do it manually for now)
# (also we need to have good icon placement, background image, etc)

echo "\nBundle complete! Located in ${BUNDLE_OUTPUT}"
