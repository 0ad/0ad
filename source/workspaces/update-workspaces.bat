@ECHO OFF
REM Create Visual Studio Workspaces on Windows

mkdir vc6
mkdir vc7
mkdir vc2003

REM Change to the lua project name, this must correspond to the base file name
REM of the created project files
SET PROJECT=prometheus

CD premake
premake --target vs6
MOVE %PROJECT%.dsw ..\vc6
MOVE %PROJECT%.dsp ..\vc6

premake --target vs7
move %PROJECT%.sln ..\vc7
move %PROJECT%.vcproj ..\vc7

premake --target vs2003
move %PROJECT%.sln ..\vc2003
move %PROJECT%.vcproj ..\vc2003

cd ..

pause