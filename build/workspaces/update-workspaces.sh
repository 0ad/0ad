#!/bin/sh

die()
{
  echo ERROR: $*
  exit 1
}

with_system_nvtt=false
for i in "$@"
do
  case $i in
    --with-system-nvtt ) with_system_nvtt=true ;;
  esac
done

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

echo "Updating bundled third-party dependencies..."
echo

# Build/update bundled external libraries
(cd ../../libraries/fcollada/src && make) || die "FCollada build failed"
echo
(cd ../../libraries/spidermonkey-tip && ./build.sh) || die "SpiderMonkey build failed"
echo
if [ "$with_system_nvtt" = "false" ]; then
  (cd ../../libraries/nvtt && ./build.sh) || die "NVTT build failed"
fi
echo

# Make sure workspaces/gcc exists.
mkdir -p gcc

# Now build premake and run it to create the makefiles
cd ../premake
make -C src || die "Premake build failed"

echo

# If we're in bash then make HOSTTYPE available to Premake, for primitive arch-detection
export HOSTTYPE="$HOSTTYPE"

src/bin/premake --outpath ../workspaces/gcc --atlas --collada "$@" --target gnu || die "Premake failed"
