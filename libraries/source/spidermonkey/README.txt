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

The basic idea is to follow the instructions to build Firefox:
https://firefox-source-docs.mozilla.org/setup/windows_build.html#mozillabuild
And after running "mach boostrap", run ./build.sh

The customised option (which I used):
- Install mozilla-build per the instructions above
- Install rust (make sure to add it to your PATH)
- Open Powershell and run "rustup install i686-pc-windows-msvc" and "rustup install x86_64-pc-windows-msvc"
- Install LLVM 8 prebuilt binaries from https://releases.llvm.org somewhere (the script plans for C:/Program Files/LLVM)
- From powershell, run ". C:/mozilla-build/start-shell.bat", cd to 0ad/libraries/source/spidermonkey and then run "./build.sh"

At that point, everything should be setup and run correctly.
