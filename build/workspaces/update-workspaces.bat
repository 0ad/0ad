@echo off
rem ** Create Visual Studio Workspaces on Windows **

mkdir vc2002
mkdir vc2003
mkdir vc2005

cd ..\premake

premake --target vs2002 --outpath ../workspaces/vc2002 %*
premake --target vs2003 --outpath ../workspaces/vc2003 %*
premake --target vs2005 --outpath ../workspaces/vc2005 %*

cd ..\workspaces
