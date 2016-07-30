@echo off
REM ensure header is up to date (simpler than setting include path)
copy ..\..\..\..\..\include\lib\sysdep\os\win\aken\aken.h

cmd /c build_single.bat chk x64
cmd /c build_single.bat fre x64
cmd /c build_single.bat chk x86
REM must come last because each build_single.bat deletes aken.sys,
REM and that is the final output name of this step
cmd /c build_single.bat fre x86

cd amd64
copy /y aken64*.pdb ..\..\..\..\..\..\..\bin\x64
copy /y aken64*.sys ..\..\..\..\..\..\..\bin\x64
cd ..

cd i386
copy /y aken*.pdb ..\..\..\..\..\..\..\bin\Win32
copy /y aken*.sys ..\..\..\..\..\..\..\bin\Win32
cd ..

echo outputs copied to bin directory; will delete ALL output files after pressing a key
pause
if exist amd64 (rmdir /S /Q amd64)
if exist i386 (rmdir /S /Q i386)
if exist objchk_wnet_amd64 (rmdir /S /Q objchk_wnet_amd64)
if exist objfre_wnet_amd64 (rmdir /S /Q objfre_wnet_amd64)
if exist objchk_wnet_x86 (rmdir /S /Q objchk_wnet_x86)
if exist objfre_wnet_x86 (rmdir /S /Q objfre_wnet_x86)
if exist *.log (del /Q *.log)
if exist *.err (del /Q *.err)
if exist *.wrn (del /Q *.wrn)