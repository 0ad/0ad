@echo off
rem ** Create Visual Studio Workspaces on Windows **

mkdir vc2003
mkdir vc2005

cd ..\premake

rem VC2002 removed because no one is using it and generating it wastes time.
rem it's entirely analogous to other cmdlines - just copy+paste if needed again.

premake --target vs2003 --outpath ../workspaces/vc2003 %*
premake --target vs2005 --outpath ../workspaces/vc2005 %*

cd ..\workspaces
