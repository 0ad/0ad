@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
if not exist ..\workspaces\vc2013\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vc2013" --use-shared-glooxwrapper %* vs2013
if not exist ..\workspaces\vc2015\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vc2015" --use-shared-glooxwrapper %* vs2015
cd ..\workspaces
