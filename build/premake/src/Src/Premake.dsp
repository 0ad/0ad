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
!MESSAGE "Premake - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Premake - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Premake - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "../bin"
# PROP BASE Intermediate_Dir "obj/Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "obj/Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GR /GX /O1 /D "_CRT_SECURE_NO_DEPRECATE" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GR /GX /O1 /D "_CRT_SECURE_NO_DEPRECATE" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 m.lib /nologo /entry:"mainCRTStartup" /subsystem:console /machine:I386 /out:"../bin/premake" /libpath:".."
# ADD LINK32 m.lib /nologo /entry:"mainCRTStartup" /subsystem:console /machine:I386 /out:"../bin/premake" /libpath:".."

!ELSEIF  "$(CFG)" == "Premake - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../bin"
# PROP BASE Intermediate_Dir "obj/Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "obj/Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /D "_CRT_SECURE_NO_DEPRECATE" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GR /GX /ZI /Od /D "_CRT_SECURE_NO_DEPRECATE" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 m.lib /nologo /entry:"mainCRTStartup" /subsystem:console /incremental:yes /debug /machine:I386 /out:"../bin/premake" /pdbtype:sept /libpath:".."
# ADD LINK32 m.lib /nologo /entry:"mainCRTStartup" /subsystem:console /incremental:yes /debug /machine:I386 /out:"../bin/premake" /pdbtype:sept /libpath:".."

!ENDIF

# Begin Target

# Name "Premake - Win32 Release"
# Name "Premake - Win32 Debug"
# Begin Group "Lua"

# PROP Default_Filter ""
# Begin Source File

SOURCE=Lua/lauxlib.h
# End Source File
# Begin Source File

SOURCE=Lua/ldebug.h
# End Source File
# Begin Source File

SOURCE=Lua/lualib.h
# End Source File
# Begin Source File

SOURCE=Lua/ldo.h
# End Source File
# Begin Source File

SOURCE=Lua/lundump.h
# End Source File
# Begin Source File

SOURCE=Lua/lmem.h
# End Source File
# Begin Source File

SOURCE=Lua/lstate.h
# End Source File
# Begin Source File

SOURCE=Lua/ltm.h
# End Source File
# Begin Source File

SOURCE=Lua/lvm.h
# End Source File
# Begin Source File

SOURCE=Lua/ltable.h
# End Source File
# Begin Source File

SOURCE=Lua/llex.h
# End Source File
# Begin Source File

SOURCE=Lua/lgc.h
# End Source File
# Begin Source File

SOURCE=Lua/lfunc.h
# End Source File
# Begin Source File

SOURCE=Lua/lopcodes.h
# End Source File
# Begin Source File

SOURCE=Lua/lparser.h
# End Source File
# Begin Source File

SOURCE=Lua/llimits.h
# End Source File
# Begin Source File

SOURCE=Lua/lzio.h
# End Source File
# Begin Source File

SOURCE=Lua/lua.h
# End Source File
# Begin Source File

SOURCE=Lua/lobject.h
# End Source File
# Begin Source File

SOURCE=Lua/lstring.h
# End Source File
# Begin Source File

SOURCE=Lua/lapi.h
# End Source File
# Begin Source File

SOURCE=Lua/lcode.h
# End Source File
# Begin Source File

SOURCE=Lua/luadebug.h
# End Source File
# Begin Source File

SOURCE=Lua/lauxlib.c
# End Source File
# Begin Source File

SOURCE=Lua/ldebug.c
# End Source File
# Begin Source File

SOURCE=Lua/ltablib.c
# End Source File
# Begin Source File

SOURCE=Lua/liolib.c
# End Source File
# Begin Source File

SOURCE=Lua/lstrlib.c
# End Source File
# Begin Source File

SOURCE=Lua/ldo.c
# End Source File
# Begin Source File

SOURCE=Lua/ltests.c
# End Source File
# Begin Source File

SOURCE=Lua/ldump.c
# End Source File
# Begin Source File

SOURCE=Lua/lundump.c
# End Source File
# Begin Source File

SOURCE=Lua/ldblib.c
# End Source File
# Begin Source File

SOURCE=Lua/lmem.c
# End Source File
# Begin Source File

SOURCE=Lua/lmathlib.c
# End Source File
# Begin Source File

SOURCE=Lua/lstate.c
# End Source File
# Begin Source File

SOURCE=Lua/ltm.c
# End Source File
# Begin Source File

SOURCE=Lua/lvm.c
# End Source File
# Begin Source File

SOURCE=Lua/ltable.c
# End Source File
# Begin Source File

SOURCE=Lua/llex.c
# End Source File
# Begin Source File

SOURCE=Lua/lgc.c
# End Source File
# Begin Source File

SOURCE=Lua/loadlib.c
# End Source File
# Begin Source File

SOURCE=Lua/lfunc.c
# End Source File
# Begin Source File

SOURCE=Lua/lparser.c
# End Source File
# Begin Source File

SOURCE=Lua/lopcodes.c
# End Source File
# Begin Source File

SOURCE=Lua/lbaselib.c
# End Source File
# Begin Source File

SOURCE=Lua/lzio.c
# End Source File
# Begin Source File

SOURCE=Lua/lobject.c
# End Source File
# Begin Source File

SOURCE=Lua/lstring.c
# End Source File
# Begin Source File

SOURCE=Lua/lapi.c
# End Source File
# Begin Source File

SOURCE=Lua/lcode.c
# End Source File
# End Group
# Begin Source File

SOURCE=path.h
# End Source File
# Begin Source File

SOURCE=vs2002.h
# End Source File
# Begin Source File

SOURCE=vs2005.h
# End Source File
# Begin Source File

SOURCE=script.h
# End Source File
# Begin Source File

SOURCE=io.h
# End Source File
# Begin Source File

SOURCE=arg.h
# End Source File
# Begin Source File

SOURCE=sharpdev.h
# End Source File
# Begin Source File

SOURCE=util.h
# End Source File
# Begin Source File

SOURCE=vs6.h
# End Source File
# Begin Source File

SOURCE=os.h
# End Source File
# Begin Source File

SOURCE=platform.h
# End Source File
# Begin Source File

SOURCE=gnu.h
# End Source File
# Begin Source File

SOURCE=project.h
# End Source File
# Begin Source File

SOURCE=premake.h
# End Source File
# Begin Source File

SOURCE=vs.h
# End Source File
# Begin Source File

SOURCE=sharpdev.c
# End Source File
# Begin Source File

SOURCE=vs2005.c
# End Source File
# Begin Source File

SOURCE=io.c
# End Source File
# Begin Source File

SOURCE=sharpdev_cs.c
# End Source File
# Begin Source File

SOURCE=script.c
# End Source File
# Begin Source File

SOURCE=platform_posix.c
# End Source File
# Begin Source File

SOURCE=arg.c
# End Source File
# Begin Source File

SOURCE=vs6_cpp.c
# End Source File
# Begin Source File

SOURCE=util.c
# End Source File
# Begin Source File

SOURCE=vs6.c
# End Source File
# Begin Source File

SOURCE=gnu_helpers.c
# End Source File
# Begin Source File

SOURCE=os.c
# End Source File
# Begin Source File

SOURCE=vs2005_cs.c
# End Source File
# Begin Source File

SOURCE=gnu.c
# End Source File
# Begin Source File

SOURCE=gnu_cpp.c
# End Source File
# Begin Source File

SOURCE=gnu_cs.c
# End Source File
# Begin Source File

SOURCE=platform_windows.c
# End Source File
# Begin Source File

SOURCE=project.c
# End Source File
# Begin Source File

SOURCE=vs2002_cs.c
# End Source File
# Begin Source File

SOURCE=clean.c
# End Source File
# Begin Source File

SOURCE=premake.c
# End Source File
# Begin Source File

SOURCE=vs.c
# End Source File
# Begin Source File

SOURCE=path.c
# End Source File
# Begin Source File

SOURCE=vs2002.c
# End Source File
# End Target
# End Project
