To build SpiderMonkey for use in 0 A.D. on Linux or OS X, run ./build.sh

To build on Windows (if you don't want to use the precompiled binaries in SVN),
get https://developer.mozilla.org/en/Windows_Build_Prerequisites#MozillaBuild
then run start-msvc8.bat and run ./build.sh here.

This version of SpiderMonkey comes from http://hg.mozilla.org/tracemonkey/
revision a4ed852d402a
plus the patch https://bug610228.bugzilla.mozilla.org/attachment.cgi?id=489277
plus the patch https://bug613089.bugzilla.mozilla.org/attachment.cgi?id=491381
plus the patch https://bug617596.bugzilla.mozilla.org/attachment.cgi?id=496149
plus the patch https://bug609024.bugzilla.mozilla.org/attachment.cgi?id=491526
plus a trivial patch to build with python2.7

The game must be compiled with precisely this version, and must not use a
standard system-provided version of the library, since SpiderMonkey does not
guarantee API stability and may have behavioural changes that cause subtle
bugs or network out-of-sync errors.

To save space, the src/ directory only contains files from Mozilla's js/src/
plus the following subdirectories:
    assembler
    build
    config
    editline
    jsapi-tests
    lirasm
    methodjit
    nanojit
    perf
    ref-config
    shell
    tracejit
    v8-dtoa
    vprof
    yarr

Also, the output of autoconf-2.13 is included in src/
