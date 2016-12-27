#!/bin/sh

# Run "update-workspaces.sh --coverage" (in the appropriate directory) then
# do "make test Collada config=debug", before running this script.
# Also make sure you don't build via ccache. Also you might need a patch like
# http://ltp.cvs.sourceforge.net/viewvc/ltp/utils/analysis/lcov/bin/lcov?r1=1.33&r2=1.34

rm -f app.info

for APPDIR in ../workspaces/gcc/obj/*; do
  lcov -d "$APPDIR" --zerocounters
  lcov -d "$APPDIR" -b ../workspaces/gcc --capture --initial -o temp.info
  if [ -e app.info ]; then
    lcov -a app.info -a temp.info -o app.info
  else
    mv temp.info app.info
  fi
done

(cd ../../binaries/system/; ./test_dbg)

for APPDIR in ../workspaces/gcc/obj/*; do
  lcov -d "$APPDIR" -b ../workspaces/gcc --capture -o temp.info &&
    lcov -a app.info -a temp.info -o app.info
done

lcov -r app.info '/usr/*' -o app.info
lcov -r app.info '*/libraries/*' -o app.info
lcov -r app.info '*/third_party/*' -o app.info

rm -rf output
mkdir output
(cd output; genhtml ../app.info)
