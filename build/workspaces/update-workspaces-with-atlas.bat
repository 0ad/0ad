@echo off
rem ** Create Visual Studio Workspaces on Windows **

cd ..\premake
if not exist ..\workspaces\vc2012\SKIP_PREMAKE_HERE premake4\bin\release\premake4 --outpath="../workspaces/vc2012" --collada --atlas %* vs2012
cd ..\workspaces
