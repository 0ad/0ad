Since the wxWidgets library is quite large (~250MB for the include and lib files), and is not needed unless you want to compile Atlas, it is not included in SVN and should instead be installed manually:

* Download the Windows source code from http://wxwidgets.org/downloads/
* Open build\msw\wx_vc12.sln in Visual Studio 2013 (same version as used for the game)
* Select the "Debug" configuration
* Build
* Select the "Release" configuration
* Build
* Copy lib\ and include\ into the game's libraries\win32\wxwidgets folder
