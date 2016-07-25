@echo off
REM build a single configuration ({chk,fre} x {x64,x64})
REM (must be in a separate file to allow invoking in a new cmd - otherwise,
REM setenv complains that the environment have already been set)
REM arguments: chk|fre x64|x86
if %1==chk (goto configOK)
if %1==fre (goto configOK)
echo first parameter must be either chk or fre
goto :eof
:configOK
if %2==x64 (goto archOK)
if %2==x86 (goto archOK)
echo second parameter must be either x64 or x64
goto :eof
:archOK


call C:\WinDDK\7600.16385.1\bin\setenv.bat C:\WinDDK\7600.16385.1\ %1 %2 WNET
e:
cd \FOM_Work\Programmierung\lowlevel\src\sysdep\os\win\aken


REM delete outputs to ensure they get rebuilt
if %2==x64 (goto delete64) else (goto delete32)

:delete64
if not exist amd64 (goto deleteEnd)
cd amd64
if exist aken.sys (del /Q aken.sys)
if exist aken.pdb (del /Q aken.pdb)
cd ..
goto deleteEnd

:delete32
if not exist i386 (goto deleteEnd)
cd i386
if exist aken.sys (del /Q aken.sys)
if exist aken.pdb (del /Q aken.pdb)
cd ..
goto deleteEnd

:deleteEnd


build


REM rename outputs in preparation for build_all's copying them to the binaries directories
if %2==x64 (goto rename64) else (goto rename32)

:rename64
if not exist amd64 (goto renameEnd)
cd amd64
if %1==chk (ren aken.pdb aken64d.pdb) else (ren aken.pdb aken64.pdb)
if %1==chk (ren aken.sys aken64d.sys) else (ren aken.sys aken64.sys)
cd ..
goto renameEnd

:rename32
if not exist i386 (goto renameEnd)
cd i386
if %1==chk (ren aken.pdb akend.pdb)
if %1==chk (ren aken.sys akend.sys)
cd ..
goto renameEnd

:renameEnd
