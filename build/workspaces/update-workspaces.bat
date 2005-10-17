@ECHO OFF
REM Create Visual Studio Workspaces on Windows

mkdir vc6
mkdir vc7
mkdir vc2003

REM Change to the lua project name, this must correspond to the base file name
REM of the created project files
SET PROJECT=pyrogenesis

cd ..\premake

REM Minor hack to make sure the relative paths are correct:
mkdir tmp
copy premake.lua tmp
cd tmp

REM Just copy *.sln/etc indiscriminately, because it might include both pyrogenesis.sln and sced.sln (or might not)

..\premake --target vs6 %*
move *.dsw ..\..\workspaces\vc6
move *.dsp ..\..\workspaces\vc6

..\premake --target vs7 %*
move *.sln    ..\..\workspaces\vc7
move *.vcproj ..\..\workspaces\vc7

..\premake --target vs2003 %*
move *.sln    ..\..\workspaces\vc2003
move *.vcproj ..\..\workspaces\vc2003

cd ..\..\workspaces
