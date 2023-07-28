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
Using different settings for compiling SpiderMonkey and 0 A.D.
causes incompatibilities on the ABI (binary) level and can lead to
crashes at runtime!


Building on Linux:
------------------
To build SpiderMonkey for use in 0 A.D. on Linux, just run build.sh.


Building on Mac OS X:
---------------------
Use the build-osx-libs.sh script in libraries/osx.


Building on Windows:
--------------------
We provide precompiled binaries for Windows.
If you still need to build on Windows, here's a short guide.

In the Visual Studio Installer:
- Install Clang, Clang-CL

Install Rust from the official website.

Download & install Mozilla Build from [here](https://ftp.mozilla.org/pub/mozilla/libraries/win32/MozillaBuildSetup-Latest.exe).
In Mozilla build run:
rustup target add i686-pc-windows-msvc
rustup target add x86_64-pc-windows-msvc

From powershell, run the following commands.
They will start a mozbuild shell, setup LLVM_LOCATION (may vary),
then run the build.
. C:/mozilla-build/start-shell.bat
LLVM_LOCATION="/c/PROGRA~1/MICROS~1/2017/Community/VC/Tools/Llvm/x64/bin/"
export PATH="$PATH:$LLVM_LOCATION"
cd "/libraries/source/spidermonkey" &&".\build.sh"
