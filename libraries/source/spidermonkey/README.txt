To build SpiderMonkey for use in 0 A.D. on Linux or OS X, run ./build.sh

To build on Windows (if you don't want to use the precompiled binaries in SVN),
get https://developer.mozilla.org/en/Windows_Build_Prerequisites#MozillaBuild
then run start-msvc8.bat and run ./build.sh here.

This version of SpiderMonkey comes from
http://ftp.mozilla.org/pub/mozilla.org/js/js185-1.0.0.tar.gz
(see also https://developer.mozilla.org/en/SpiderMonkey/1.8.5)

The game must be compiled with precisely this version, and must not use a
standard system-provided version of the library, since SpiderMonkey does not
guarantee API stability and may have behavioural changes that cause subtle
bugs or network out-of-sync errors.
