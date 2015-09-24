# Apply patches if needed
# This script gets called from build-osx-libs.sh and build.sh.
cd mozjs31

# Tracelogger patches (the Tracelogger is a tool for developers and these
# patches are only needed if the tracelogger is used). The first patch is
# a backport from newer SpiderMonkey versions and the second patch
# combines some fixes from newer versions with a change that makes it
# flush data to the disk after 100 MB. The developer of the tracelogger
# (h4writer) is informed about everything and an these patches shouldn't
# be needed anymore for the next version of SpiderMonkey.
patch -p1 -i ../FixTraceLoggerBuild.diff
patch -p1 -i ../FixTraceLoggerFlushing.diff

# A patch to fix a bug that prevents Ion compiling of for .. of loops.
# It makes quite a big difference for performance.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1046176
patch -p1 -i ../FixForOfBailouts.diff

# Fix build failures on GCC 5.1 and Clang 3.6
patch -p1 -i ../FixBug1021171.diff
patch -p1 -i ../FixBug1119228.diff

# Fix debug build failure on platforms with Ion disabled (eg AArch64)
patch -p1 -i ../FixBug1037470.diff
