@echo off
rem ** Create Visual Studio Workspaces on Windows **

if not exist vc2003\pyrogenesis.sln mkdir vc2003
if not exist vc2005\pyrogenesis.sln mkdir vc2005

rem VC2002 removed because no one is using it and generating it wastes time.
rem it's entirely analogous to other cmdlines - just copy+paste if needed again.

cd ..\premake

if not exist ..\workspaces\vc2003\SKIP_PREMAKE_HERE premake --target vs2003 --outpath ../workspaces/vc2003 %*
if not exist ..\workspaces\vc2005\SKIP_PREMAKE_HERE premake --target vs2005 --outpath ../workspaces/vc2005 %*

cd ..\workspaces
