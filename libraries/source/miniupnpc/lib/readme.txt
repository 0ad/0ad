To create the Windows .lib/.dll files:

* Open src/msvc/miniupnpc in Visual Studio.
* Build libs.
* Switch configuration type to .dll in each configuration.
* Add "ws2_32.lib" and "Iphlpapi.lib" to project linker dependencies.
* Build dlls.
* Copy the files into the right places adding a "d" suffix to linker output files in debug configuration.