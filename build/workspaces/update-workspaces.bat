@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
if not exist ..\workspaces\vc2008\SKIP_PREMAKE_HERE premake4\bin\release\premake4 --outpath="../workspaces/vc2008" --collada %* vs2008
if not exist ..\workspaces\vc2010\SKIP_PREMAKE_HERE premake4\bin\release\premake4 --outpath="../workspaces/vc2010" --collada %* vs2010
if not exist ..\workspaces\vc2012\SKIP_PREMAKE_HERE premake4\bin\release\premake4 --outpath="../workspaces/vc2012" --collada %* vs2012
cd ..\workspaces
