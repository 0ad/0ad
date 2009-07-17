#!/bin/sh

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

# Make sure workspaces/gcc exists.
mkdir -p gcc

# Now build premake and run it to create the makefiles
cd ../premake
make -C src
src/bin/premake --outpath ../workspaces/gcc --atlas --collada "$@" --target gnu
