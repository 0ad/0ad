@ECHO OFF

"%systemroot%\system32\cacls.exe" "%systemroot%\system32\config\system" >nul 2>&1
IF ERRORLEVEL 1 GOTO relaunch

REM detect whether OS is 32/64 bit
IF "%ProgramW6432%" == "%ProgramFiles%" (
 SET aken_bits=64
) ELSE (
 SET aken_bits=32
)

IF "%1" == "enabletest" GOTO enabletest
IF "%1" == "disabletest" GOTO disabletest
IF "%1" == "install" GOTO install
IF "%1" == "remove" GOTO remove
GOTO usage

:enabletest
bcdedit.exe /set TESTSIGNING ON
GOTO end

:disabletest
bcdedit.exe /set TESTSIGNING OFF
GOTO end

:install
IF (%2) == () (
 SET aken_path="%~p0\aken%aken_bits%.sys"
) ELSE (
 echo %2\aken%aken_bits%.sys
 SET aken_path=%2\aken%aken_bits%.sys
)
echo %aken_path%
IF NOT EXIST %aken_path% GOTO notfound
sc create Aken DisplayName= Aken type= kernel start= auto binpath= %aken_path%
REM error= normal is default
IF ERRORLEVEL 1 GOTO failed
sc start Aken
IF ERRORLEVEL 1 GOTO failed
ECHO Success!
GOTO end

:remove
sc stop Aken
sc delete Aken
IF ERRORLEVEL 1 GOTO failed
ECHO Success! (The previous line should read: [SC] DeleteService SUCCESS)
GOTO end

:usage
ECHO To install the driver, please first enable test mode:
ECHO %0 enabletest
ECHO (This is necessary because Vista/Win7 x64 require signing with
ECHO  a Microsoft "cross certificate". The Fraunhofer code signing certificate
ECHO  is not enough, even though its chain of trust is impeccable.
ECHO  Going the WHQL route, perhaps as an "unclassified" driver, might work.
ECHO  see http://www.freeotfe.org/docs/Main/impact_of_kernel_driver_signing.htm )
ECHO Then reboot (!) and install the driver:
ECHO %0 install ["path_to_directory_containing_aken*.sys"]
ECHO (If no path is given, we will use the directory of this batch file)
ECHO To remove the driver and disable test mode, execute the following:
ECHO %0 remove
ECHO %0 disabletest
PAUSE
GOTO end

:relaunch
SET aken_vbs="%temp%\aken_run.vbs"
ECHO Set UAC = CreateObject^("Shell.Application"^) > %aken_vbs%
ECHO UAC.ShellExecute "cmd.exe", "/k %~s0 %1 %2", "", "runas", 1 >> %aken_vbs%
ECHO "To re-run this batch file as admin, we have created %aken_vbs% with the following contents:"
type %aken_vbs%
PAUSE
cscript //Nologo %aken_vbs%
DEL %aken_vbs%
GOTO end

:notfound
ECHO Driver not found at specified path (%aken_path%)
GOTO end

:failed
ECHO Something went wrong -- see previous line
GOTO end

:end