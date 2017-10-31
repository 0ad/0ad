@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
if not exist ..\workspaces\vc2013\SKIP_PREMAKE_HERE premake5\bin\release\premake5 --outpath="../workspaces/vc2013" --use-shared-glooxwrapper %* vs2013
cd ..\workspaces
