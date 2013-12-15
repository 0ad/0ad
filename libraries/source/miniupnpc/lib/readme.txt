To create the Windows .lib/.dll files

The included MSVC project doesn't really work, use CMake instead. The process is fairly standard:

* Download/install CMake binaries for Windows, open CMake GUI.
* Select src as source code directory.
* Choose a build directory.
* Configure the project (Tools > Configure), select your version of MSVC.
* A few libraries will be missing, set IPHLPAPI_LIBRARY and WINSOCK2_LIBRARY variables to the appropriate SDK lib paths.
* Configure again.
* When configure is OK, generate the project (Tools > Generate).
* Open the generated MSVC solution in Visual Studio.
* Build release and debug libs (append "d" to names of output files in debug build).
* Copy the files into the right places.