@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
if not exist ..\workspaces\vc2015\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vc2015" --use-shared-glooxwrapper %* vs2015
if not exist ..\workspaces\vc2017\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vs2017" --use-shared-glooxwrapper %* vs2017
cd ..\workspaces
