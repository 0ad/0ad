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

..\premake --target vs6
move %PROJECT%.dsw ..\..\workspaces\vc6
move %PROJECT%.dsp ..\..\workspaces\vc6

..\premake --target vs7
move %PROJECT%.sln    ..\..\workspaces\vc7
move %PROJECT%.vcproj ..\..\workspaces\vc7

..\premake --target vs2003
move %PROJECT%.sln    ..\..\workspaces\vc2003
move %PROJECT%.vcproj ..\..\workspaces\vc2003

cd ..\..\workspaces
