Important notice:
-----------------
This version of SpiderMonkey comes from
https://people.mozilla.org/~sstangl/mozjs-38.2.1.rc0.tar.bz2

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
To build SpiderMonkey for use in 0 A.D. on Linux, you need libnspr4-dev, which
should be installed from the distribution's package management system.
As an alternative you can build nspr yourself, but we don't provide a guide for
that here. When you have nspr, just run build.sh.

NSPR is available here:
https://ftp.mozilla.org/pub/mozilla.org/nspr/releases/


Building on Mac OS X:
---------------------
Use the build-osx-libs.sh script in libraries/osx.


Building on Windows:
--------------------
We provide precompiled binaries for Windows.
If you still need to build on Windows, here's a short guide.

Setting up the build environment:
1. Get https://developer.mozilla.org/en-US/docs/Mozilla/Developer_guide/Build_Instructions/Windows_Prerequisites#MozillaBuild

Building NSPR:
1. Get nspr. We are using nspr-4.12 which is the newest version at the moment.
   Newer versions should probably work too.
   Download link: https://ftp.mozilla.org/pub/mozilla.org/nspr/releases/
2. Run mozillabuild (start-shell-msvc2013.bat) as administrator
3. Extract nspr to libraries/source/spidermonkey
   tar -xzvf nspr-4.12.tar.gz
   cd nspr-4.12
   cd nspr
4. Patch nspr with https://bugzilla.mozilla.org/show_bug.cgi?id=1238154#c7
5. Call configure. I've used this command:
   ./configure --disable-debug --enable-optimize --enable-win32-target=WIN95
6. Call make

Building SpiderMonkey:
1. Adjust the absolute paths to nspr in the build.sh file to match your environment.
2. Run mozillabuild (start-shell-msvc2013.bat) as administrator and run ./build.sh.


