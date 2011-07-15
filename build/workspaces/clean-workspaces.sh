#!/bin/sh

# (We don't attempt to clean up every last file here - output in
# binaries/system/ will still be there, etc. This is mostly just
# to quickly fix problems in the bundled dependencies.)

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

echo "Cleaning bundled third-party dependencies..."

(cd ../../libraries/fcollada/src && rm -rf ./output)
(cd ../../libraries/spidermonkey-tip && rm -f .already-built)
(cd ../../libraries/spidermonkey-tip/src && rm -rf ./build-debug)
(cd ../../libraries/spidermonkey-tip/src && rm -rf ./build-release)
(cd ../../libraries/nvtt/src && rm -rf ./build)

(cd ../premake/src && make clean)

echo "Cleaning build output..."

# Remove workspaces/gcc if present
rm -rf ./gcc
# Remove workspaces/xcode3 if present
rm -rf ./xcode3

echo
echo "Done. Try running update-workspaces.sh again now."
