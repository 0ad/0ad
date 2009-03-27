@echo off
rem ** Create Visual Studio Workspaces on Windows **

if not exist vc2005\pyrogenesis.sln mkdir vc2005
if not exist vc2008\pyrogenesis.sln mkdir vc2008

rem VC2002 and VC2003 removed because no one is using it and generating it wastes time.
rem it's entirely analogous to other cmdlines - just copy+paste if needed again.

cd ..\premake

if not exist ..\workspaces\vc2005\SKIP_PREMAKE_HERE premake --collada --target vs2005 --outpath ../workspaces/vc2005 %*
if not exist ..\workspaces\vc2008\SKIP_PREMAKE_HERE premake --collada --target vs2008 --outpath ../workspaces/vc2008 %*

cd ..\workspaces
