# Microsoft Developer Studio Project File - Name="PMDExp6" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=PMDExp6 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PMDExp6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PMDExp6.mak" CFG="PMDExp6 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PMDExp6 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "PMDExp6 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/dev/dylan/plugins/PMDExp", YVAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PMDExp6 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseR6"
# PROP Intermediate_Dir "ReleaseR6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /G5 /MT /W3 /GX /I "d:\3dsmax6\maxsdk\include" /I "..\\" /I "..\lib" /I "..\ps" /I "..\simulation" /I "..\terrain" /D "NDEBUG" /D "_USRDLL" /D "_3DSMAX_" /D "_WINDOWS" /D "WIN32" /D "_NO_WINMAIN_" /FD /LD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 pslib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x50810000" /entry:"" /subsystem:windows /dll /machine:I386 /out:"d:\3dsmax6\plugins\pmdexp6.dle" /libpath:"d:\3dsmax6\maxsdk\lib" /libpath:"..\libs" /release
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PMDExp6 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugR6"
# PROP Intermediate_Dir "DebugR6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I "d:\3dsmax6\maxsdk\include" /I "..\\" /I "..\lib" /I "..\ps" /I "..\simulation" /I "..\terrain" /D "_DEBUG" /D "_USRDLL" /D "_3DSMAX_" /D "_WINDOWS" /D "WIN32" /D "_NO_WINMAIN_" /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 pslib_d.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x50810000" /subsystem:windows /dll /debug /machine:I386 /out:"d:\3dsmax6\plugins\pmdexp6.dle" /pdbtype:sept /libpath:"d:\3dsmax6\maxsdk\lib" /libpath:"..\libs"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "PMDExp6 - Win32 Release"
# Name "PMDExp6 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DllEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpMesh.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpProp.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpSkeleton.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\PMDExp.cpp
# End Source File
# Begin Source File

SOURCE=.\PMDExp.rc
# End Source File
# Begin Source File

SOURCE=.\PMDExp6.def
# End Source File
# Begin Source File

SOURCE=.\PSAExp.cpp
# End Source File
# Begin Source File

SOURCE=.\PSProp.cpp
# End Source File
# Begin Source File

SOURCE=.\VNormal.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ExpMesh.h
# End Source File
# Begin Source File

SOURCE=.\ExpProp.h
# End Source File
# Begin Source File

SOURCE=.\ExpSkeleton.h
# End Source File
# Begin Source File

SOURCE=.\ExpUtil.h
# End Source File
# Begin Source File

SOURCE=.\ExpVertex.h
# End Source File
# Begin Source File

SOURCE=.\MaxInc.h
# End Source File
# Begin Source File

SOURCE=.\PMDExp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Tree.h
# End Source File
# Begin Source File

SOURCE=.\VertexTree.h
# End Source File
# Begin Source File

SOURCE=.\VNormal.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
