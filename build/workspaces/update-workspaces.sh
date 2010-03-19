#!/bin/sh

die()
{
	echo ERROR: $*
	exit 1
}

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

# Build/update bundled external libraries
(cd ../../libraries/fcollada/src && make) || die "FCollada build failed"
(cd ../../libraries/spidermonkey-tip && ./build.sh) || die "SpiderMonkey build failed"

# Make sure workspaces/gcc exists.
mkdir -p gcc

# Now build premake and run it to create the makefiles
cd ../premake
make -C src || die "Premake build failed"
src/bin/premake --outpath ../workspaces/gcc --atlas --collada "$@" --target gnu || die "Premake failed"
