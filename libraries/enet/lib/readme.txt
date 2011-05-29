To create the Windows .lib/.dll files:

* Get enet-1.3.1
* Open in VC2005
* Switch configuration type to .dll in each configuration
* Add ENET_DLL to defines in each configuration
* Add ws2_32.lib winmm.lib to linker dependencies in each
* Add "d" suffix to linker output file in debug configuration
* Build
* Copy the files into the right places
