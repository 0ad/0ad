#!/bin/sh

die()
{
  echo ERROR: $*
  exit 1
}

# Check for whitespace in absolute path; this will cause problems in the
# SpiderMonkey build (https://bugzilla.mozilla.org/show_bug.cgi?id=459089)
# and maybe elsewhere, so we just forbid it
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=`dirname "$SCRIPT"`
case "$SCRIPTPATH" in
  *\ * )
    die "Absolute path contains whitespace, which will break the build - move the game to a path without spaces" ;;
esac

JOBS=${JOBS:="-j2"}

# Some of our makefiles depend on GNU make, so we set some sane defaults if MAKE
# is not set.
case "`uname -s`" in
  "FreeBSD" | "OpenBSD" )
    MAKE=${MAKE:="gmake"}
    ;;
  * )
    MAKE=${MAKE:="make"}
    ;;
esac

# Parse command-line options:

premake_args=""

with_system_nvtt=false
with_system_enet=false
with_system_mozjs185=false
enable_atlas=true

for i in "$@"
do
  case $i in
    --with-system-nvtt ) with_system_nvtt=true; premake_args="${premake_args} --with-system-nvtt" ;;
    --with-system-enet ) with_system_enet=true; premake_args="${premake_args} --with-system-enet" ;;
    --with-system-mozjs185 ) with_system_mozjs185=true; premake_args="${premake_args} --with-system-mozjs185" ;;
    --enable-atlas ) enable_atlas=true ;;
    --disable-atlas ) enable_atlas=false ;;
    -j* ) JOBS=$i ;;
    # Assume any other --options are for Premake
    --* ) premake_args="${premake_args} $i" ;;
  esac
done

premake_args="${premake_args} --collada"
if [ "$enable_atlas" = "true" ]; then
  premake_args="${premake_args} --atlas"
fi


cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

echo "Updating bundled third-party dependencies..."
echo

# Build/update bundled external libraries
(cd ../../libraries/fcollada/src && ${MAKE} ${JOBS}) || die "FCollada build failed"
echo
if [ "$with_system_mozjs185" = "false" ]; then
  (cd ../../libraries/spidermonkey && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "SpiderMonkey build failed"
fi
echo
if [ "$with_system_nvtt" = "false" ]; then
  (cd ../../libraries/nvtt && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "NVTT build failed"
fi
echo
if [ "$with_system_enet" = "false" ]; then
  (cd ../../libraries/enet && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "ENet build failed"
fi
echo

# Now build premake and run it to create the makefiles
cd ../premake/premake4
# Fix the premake makefile to work on BSDs
case "`uname -s`" in
  "GNU/kFreeBSD" )
    # This needs -ldl as we have a GNU userland and libc
    ;;
  *"BSD" )
    # BSDs don't need to link with dl so modify the makefile
    # Only GNU and FreeBSD sed have the -i option (and redirecting 
    # to the same file results in an empty file and starting a subshell
    # isn't as obvious as redirecting to a new file and replacing the old)
    mv build/gmake.unix/Premake4.make build/gmake.unix/Premake4.make.bak
    sed -e 's/ -ldl/ /g' build/gmake.unix/Premake4.make.bak > build/gmake.unix/Premake4.make
   ;;
  "Darwin" )
    # Remove the obsolete -s and the unused -rdynamic parameter and
    # link with the CoreServices framework.
    sed -e 's/^\([ ]*LDFLAGS[ ]*+=[ ]*\)\(\( [^ ]*\)*\) -s\(\( [^ ]*\)*\)$/\1\2\4/' \
        -e 's/^\([ ]*LDFLAGS[ ]*+=[ ]*\)\(\( [^ ]*\)*\) -rdynamic\(\( [^ ]*\)*\)$/\1\2\4/' \
        -e 's/^\([ ]*LIBS[ ]*+=[ ]*\)\(\( [^ ]*\)*\)$/\1\2 -framework CoreServices /' \
        -i.bak build/gmake.unix/Premake4.make
    ;;
esac
${MAKE} -C build/gmake.unix ${JOBS} || die "Premake build failed"

echo

cd ..

# If we're in bash then make HOSTTYPE available to Premake, for primitive arch-detection
export HOSTTYPE="$HOSTTYPE"

premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/gcc/" ${premake_args} gmake || die "Premake failed"
premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/codeblocks/" ${premake_args} codeblocks || die "Premake failed"

# Also generate xcode3 workspaces if on OS X
if [ "`uname -s`" = "Darwin" ]
then
  premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/xcode3" ${premake_args} xcode3 || die "Premake failed"
fi
