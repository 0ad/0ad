# Microsoft Developer Studio Project File - Name="Premake" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Premake - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Premake.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Premake.mak" CFG="Premake - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Premake - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Premake - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Premake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug\"
# PROP BASE Intermediate_Dir "Debug\"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\"
# PROP Intermediate_Dir "obj\Debug\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /GX /GR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /debug /pdbtype:sept
# ADD LINK32 /nologo /subsystem:console /out:".\premake.exe" /machine:I386 /pdbtype:sept /libpath:".\" /pdb:".\premake.pdb" /debug
!ELSEIF  "$(CFG)" == "Premake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release\"
# PROP BASE Intermediate_Dir "Release\"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\"
# PROP Intermediate_Dir "obj\Release\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /GR /O1 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 
# ADD LINK32 /nologo /subsystem:console /out:".\premake.exe" /machine:I386 /pdbtype:sept /libpath:".\"
!ENDIF 

# Begin Target

# Name "Premake - Win32 Debug"
# Name "Premake - Win32 Release"
# Begin Group "Src"

# PROP Default_Filter ""
# Begin Group "Lua"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Src\Lua\lapi.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lauxlib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lbaselib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lcode.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ldebug.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ldo.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lfunc.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lgc.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\liolib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\llex.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lmathlib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lmem.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lobject.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lparser.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lstate.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lstring.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lstrlib.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ltable.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ltests.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ltm.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lundump.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lvm.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lzio.c
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lapi.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lauxlib.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lcode.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ldebug.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ldo.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lfunc.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lgc.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\llex.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\llimits.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lmem.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lobject.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lopcodes.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lparser.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lstate.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lstring.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ltable.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\ltm.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lua.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\luadebug.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lualib.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lundump.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lvm.h
# End Source File
# Begin Source File

SOURCE=.\Src\Lua\lzio.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Src\premake.c
# End Source File
# Begin Source File

SOURCE=.\Src\project.h
# End Source File
# Begin Source File

SOURCE=.\Src\project.c
# End Source File
# Begin Source File

SOURCE=.\Src\util.h
# End Source File
# Begin Source File

SOURCE=.\Src\util.c
# End Source File
# Begin Source File

SOURCE=.\Src\clean.c
# End Source File
# Begin Source File

SOURCE=.\Src\gnu.c
# End Source File
# Begin Source File

SOURCE=.\Src\sharpdev.c
# End Source File
# Begin Source File

SOURCE=.\Src\vs7.c
# End Source File
# Begin Source File

SOURCE=.\Src\vs6.c
# End Source File
# Begin Source File

SOURCE=.\Src\windows.c
# End Source File
# End Group
# End Target
# End Project
