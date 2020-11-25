Important notice:
-----------------
The game must be compiled with precisely this version since SpiderMonkey
does not guarantee API stability and may have behavioural changes that
cause subtle bugs or network out-of-sync errors.
A standard system-provided version of the library may only be used if it's
exactly the same version or if it's another minor release that does not
change the behaviour of the scripts executed by SpiderMonkey. Also it's
crucial that "--enable-gcgenerational" was used for building the system
provided libraries and that exact stack rooting was not disabled.
Using different settings for compiling SpiderMonkey and 0 A.D.
causes incompatibilities on the ABI (binary) level and can lead to
crashes at runtime!


Building on Linux:
------------------
To build SpiderMonkey for use in 0 A.D. on Linux, just run build.sh.


Building on Mac OS X:
---------------------
Use the build-osx-libs.sh script in libraries/osx.


Building on Windows:
--------------------
We provide precompiled binaries for Windows.
If you still need to build on Windows, here's a short guide.

Setting up the build environment:
1. Go to https://firefox-source-docs.mozilla.org/setup/windows_build.html#mozillabuild
2. Download the latest mozbuild package and install it to C:/mozilla-build (default).
3. Run MozillaBuild 3.x (start-shell.bat)
4. cd to the build.sh directory and run ./build.sh
