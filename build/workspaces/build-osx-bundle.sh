#!/bin/sh
#
# This script will build an OS X app bundle for 0 A.D.
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
# library with both archs and combine them with lipo, then do the
# same thing with the game itself.
# Choices are "x86_64" or  "i386" (ppc and ppc64 not supported)
export ARCH=${ARCH:="x86_64"}

OSX_VERSION=`sw_vers -productVersion | grep -Eo "^\d+.\d+"`
# Set SDK and mimimum required OS X version
export SYSROOT=${SYSROOT:="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX$OSX_VERSION.sdk"}
export MIN_OSX_VERSION=${MIN_OSX_VERSION:="10.9"}

# 0 A.D. release version, e.g. Alpha 21 is 0.0.21
BUNDLE_VERSION=${BUNDLE_VERSION:="0.0.X"}

# Define compiler as "clang", this is all Mavericks supports.
# gcc symlinks may still exist, but they are simply clang with
# slightly different config, which confuses build scripts.
# llvm-gcc and gcc 4.2 are no longer supported by SpiderMonkey.
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

# Unique identifier string for this bundle (reverse-DNS style)
BUNDLE_IDENTIFIER=${BUNDLE_IDENTIFIER:="com.wildfiregames.0ad"}
# Minimum version of OS X on which the bundle will run
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
# TODO: proper logging of all output, but show errors in terminal too
build_log="$(pwd)/bundle-build.log"

# Parse command-line options:
for i in "$@"
do
  case $i in
    -j* ) JOBS=$i ;;
    --release ) build_release=true ;;
  esac
done

rm -f $build_log

# For release, export an SVN copy
if [ "$build_release" = "true" ]; then
  echo "\nExporting SVN and preparing release\n"

  BUNDLE_OUTPUT="$build_path/0ad.app"
  SVN_REV=`svnversion -n ../..`
  rm -rf $build_path
  svn export ../.. $build_path >> $build_log 2>&1 || die "Error exporting SVN working directory"
  cd $build_path
  rm -f binaries/data/config/dev.cfg
  # Only include translations for a subset of languages
  . source/tools/dist/remove-incomplete-translations.sh $build_path/binaries/data || die "Error excluding translations"
  echo L\"${SVN_REV}-release\" > build/svn_revision/svn_revision.txt
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
echo "\nBuilding libraries\n"
pushd ../../libraries/osx > /dev/null
./build-osx-libs.sh $JOBS --force-rebuild >> $build_log 2>&1 || die "Libraries build script failed"
popd > /dev/null

# Clean and update workspaces
echo "\nGenerating workspaces\n"

# Pass OS X options through to Premake
(./clean-workspaces.sh && SYSROOT="$SYSROOT" MIN_OSX_VERSION="$MIN_OSX_VERSION" ./update-workspaces.sh --macosx-bundle="$BUNDLE_IDENTIFIER" --sysroot="$SYSROOT" --macosx-version-min="$MIN_OSX_VERSION") >> $build_log 2>&1 || die "update-workspaces.sh failed!"

pushd gcc > /dev/null
echo "\nBuilding game\n"
(make clean && CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" make ${JOBS}) >> $build_log 2>&1 || die "Game build failed!"
popd > /dev/null

# Run test to confirm all is OK
# TODO: tests are currently broken on OS X (see http://trac.wildfiregames.com/ticket/2780)
#pushd ../../binaries/system > /dev/null
#echo "\nRunning tests\n"
#./test || die "Post-build testing failed!"
#popd > /dev/null

# Create bundle structure
echo "\nCreating bundle directories\n"
rm -rf ${BUNDLE_OUTPUT}
mkdir -p ${BUNDLE_BIN}
mkdir -p ${BUNDLE_FRAMEWORKS}
mkdir -p ${BUNDLE_PLUGINS}
mkdir -p ${BUNDLE_RESOURCES}
mkdir -p ${BUNDLE_SHAREDSUPPORT}

# Build archive(s) - don't archive the _test.* mods
pushd ../../binaries/data/mods > /dev/null
archives=""
for modname in [a-zA-Z0-9]*
do
  archives="${archives} ${modname}"
done
popd > /dev/null

pushd ../../binaries/system > /dev/null

for modname in $archives
do
  echo "\nBuilding archive for '${modname}'\n"
  ARCHIVEBUILD_INPUT="$(pwd)/../data/mods/${modname}"
  ARCHIVEBUILD_OUTPUT="${BUNDLE_RESOURCES}/data/mods/${modname}"

  # For some reason the output directory has to exist?
  mkdir -p ${ARCHIVEBUILD_OUTPUT}

  (./pyrogenesis -archivebuild=${ARCHIVEBUILD_INPUT} -archivebuild-output=${ARCHIVEBUILD_OUTPUT}/${modname}.zip) >> $build_log 2>&1 || die "Archive build for '${modname}' failed!"
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

popd > /dev/null

# Copy data
echo "\nCopying non-archived game data\n"
pushd ../../binaries/data > /dev/null
if [ "$build_release" = "false" ]; then
  mv config/dev.cfg config/dev.bak
fi
cp -R -v config ${BUNDLE_RESOURCES}/data/
cp -R -v l10n ${BUNDLE_RESOURCES}/data/
cp -R -v tools ${BUNDLE_RESOURCES}/data/
if [ "$build_release" = "false" ]; then
  mv config/dev.bak config/dev.cfg
fi
popd > /dev/null
cp -v ../resources/0ad.icns ${BUNDLE_RESOURCES}

# Copy license/readmes
# TODO: Also want copies in the DMG - decide on layout
echo "\nCopying readmes\n"
cp -v ../../*.txt ${BUNDLE_RESOURCES}
cp -v ../../libraries/LICENSE.txt ${BUNDLE_RESOURCES}/LIB_LICENSE.txt

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
PlistBuddy -c "Add :NSHumanReadableCopyright string Copyright © $(date +%Y) Wildfire Games" ${INFO_PLIST}

# TODO: Automatically create compressed DMG with hdiutil?
# (this is a bit complicated so I do it manually for now)
# (also we need to have good icon placement, background image, etc)

echo "\nBundle complete! Located in ${BUNDLE_OUTPUT}"
