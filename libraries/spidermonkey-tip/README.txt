To build SpiderMonkey for use in 0 A.D. on Linux or OS X, run ./build.sh

To build on Windows (if you don't want to use the precompiled binaries in SVN),
get https://developer.mozilla.org/en/Windows_Build_Prerequisites#MozillaBuild
then run start-msvc8.bat and run ./build.sh here.

This version of SpiderMonkey comes from http://hg.mozilla.org/mozilla-central/
revision 5108c4c2c043
plus the patch from https://bugzilla.mozilla.org/show_bug.cgi?id=509857

The game must be compiled with precisely this version, and must not use a
standard system-provided version of the library, since SpiderMonkey does not
guarantee API stability and may have behavioural changes that cause subtle
bugs or network out-of-sync errors.

To save space, the src/ directory only contains files from Mozilla's js/src/
plus the following subdirectories:
    build
    config
    editline
    jsapi-tests
    lirasm
    nanojit
    ref-config
    shell
    vprof

Also, the output of autoconf-2.13 is included in src/
