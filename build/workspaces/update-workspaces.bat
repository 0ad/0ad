@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
rem ** Support for Visual Studio versions <2013 has been dropped (check #2669).
if not exist ..\workspaces\vc2013\SKIP_PREMAKE_HERE premake4\bin\release\premake4 --outpath="../workspaces/vc2013" --collada --use-shared-glooxwrapper --sdl2 %* vs2013
cd ..\workspaces
