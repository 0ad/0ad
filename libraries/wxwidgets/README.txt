Since the wxWidgets library is quite large (~70MB for the include and lib files), and is not needed unless you want to compile Atlas, it is not included in SVN and should instead be downloaded from http://wildfiregames.com/~code/libraries/wxwidgets.exe (8MB self-extracting 7-Zip).

(Alternatively, you could download and compile wxWidgets yourself - enable the GL canvas in the setup.h file, build the "Unicode Debug" and "Unicode Release" configurations, and hopefully that's all it needs.)
