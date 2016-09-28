#!/bin/sh
# Apply patches if needed
# This script gets called from build-osx-libs.sh and build.sh.

# Fix the version specification code which used PYTHON before it was set.
# The second patch is required to not add autoconf as a dependency.
patch -p1 < ../FixVersionDetection.diff
patch -p1 < ../FixVersionDetectionConfigure.diff

# Fix the path to the moz.build file in the zlib module
patch -p1 < ../FixZLibMozBuild.diff

# === Fix the SM38 tracelogger ===
# This patch is a squashed version of several patches that were adapted
# to fix failing hunks.
#
# Applied in the following order, they are:
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1223767
#    Assertion failure: i < size_, at js/src/vm/TraceLoggingTypes.h:210
#    Also fix stop-information to make reduce.py work correctly.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1227914
#    Limit the memory tracelogger can take.
#    This causes tracelogger to flush data to the disk regularly and prevents out of
#    memory issues if a lot of data gets logged.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1155618
#    Fix tracelogger destructor that touches possibly uninitialised hash table.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1223636
#    Don't treat extraTextId as containing only extra ids.
#    This fixes an assertion failure: id == nextTextId at js/src/vm/TraceLoggingGraph.cpp
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1227028
#    Fix when to keep the payload of a TraceLogger event.
#    This fixes an assertion failure: textId < uint32_t(1 << 31) at js/src/vm/TraceLoggingGraph.h
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1266649
#    Handle failing to add to pointermap gracefully.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1280648
#    Don't cache based on pointers to movable GC things.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1224123
#    Fix the use of LastEntryId in tracelogger.h.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1231170
#    Use size in debugger instead of the current id to track last logged item.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1221844
#    Move TraceLogger_Invalidation to LOG_ITEM.
#    Add some debug checks to logTimestamp.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1255766
#    Also mark resizing of memory.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1259403
#    Only increase capacity by multiples of 2.
#    Always make sure there are 3 free slots for events.
# ===
patch -p1  < ../FixTracelogger.diff
