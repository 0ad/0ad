Important notice:
-----------------
This version of SpiderMonkey comes from
https://ftp.mozilla.org/pub/mozilla.org/js/mozjs-24.2.0.tar.bz2

The game must be compiled with precisely this version since SpiderMonkey 
does not guarantee API stability and may have behavioural changes that 
cause subtle bugs or network out-of-sync errors.
A standard system-provided version of the library may only be used if it's
exactly the same version or if it's another minor release that does not 
change the behaviour of the scripts executed by SpiderMonkey.


Building on Linux:
------------------
To build SpiderMonkey for use in 0 A.D. on Linux, you need libnspr4-dev, which
should be installed from the distribution's package management system.
As an alternative you can build nspr yourself, but we don't provide a guide for
that here. When you have nspr, just run build.sh.

NSPR ist available here: 
https://ftp.mozilla.org/pub/mozilla.org/nspr/releases/


Building on Mac OS X:
---------------------
Use the build-osx-libs.sh script in libraries/osx.


Building on Windows:
--------------------
We provide precompiled binaries for Windows.
If you still need to build on Windows, here's a short guide.

Setting up the build environment:
1. Get https://developer.mozilla.org/en/Windows_Build_Prerequisites#MozillaBuild
2. I had to adjust some paths to the correct SDK folders in start-msvc10.bat. 
   That depends a lot on your setup and the version of mozilla build, so you have 
   to figure out yourself how to get it working. 

Building NSPR:
1. Get nspr. We are using nspr-4.10.3 which is the newest version at the moment.
   Newer versions should probably work too. 
   Download link: https://ftp.mozilla.org/pub/mozilla.org/nspr/releases/
2. Run mozillabuild (start-msvc10.bat) as administrator
3. Extract nspr to libraries/source/spidermonkey
   tar -xzvf nspr-4.10.3.tar.gz
   cd nspr-4.10.3
   cd nspr
4. Build nspr by calling make

Building SpiderMonkey:
1. Adjust the absolute paths to nspr in the build.sh file to match your environment.
2. Run mozillabuild (start-msvc10.bat) as administrator and run ./build.sh.


