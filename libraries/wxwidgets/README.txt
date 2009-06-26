Since the wxWidgets library is quite large (~70MB for the include and lib files), and is not needed unless you want to compile Atlas, it is not included in SVN and should instead be installed manually:

* Download the wxMSW zip from http://wxwidgets.org/downloads/
* Edit include/wx/msw/setup.h and set wxUSE_GLCANVAS to 1
* Open build/msw/wx.dsw in Visual Studio
* Agree to convert all projects
* Select the "Unicode Debug" configuration
* Build
* Select the "Unicode Release" configuration
* Build
* Copy lib/ and include/ into the game's libraries/wxwidgets/
