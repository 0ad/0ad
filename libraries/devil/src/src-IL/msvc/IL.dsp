# Microsoft Developer Studio Project File - Name="IL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=IL - WIN32 DEBUG
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "IL.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "IL.mak" CFG="IL - WIN32 DEBUG"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "IL - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IL - Win32 Dynamic" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "IL - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "IL"
# PROP Scc_LocalPath "..\.."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "IL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "../../lib/debug"
# PROP BASE Intermediate_Dir "../src/obj/debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../lib/debug"
# PROP Intermediate_Dir "../src/obj/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../projects/msvc/extlibs/" /I "../include" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"../src/obj/debug/DevIL.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /def:".\il.def" /out:"../../lib/debug/DevIL-d.dll" /pdbtype:sept /libpath:"../../projects/msvc/extlibs"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "IL - Win32 Dynamic"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../../lib"
# PROP BASE Intermediate_Dir "../src/obj/dynamic"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "../src/obj/dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "../include" /I "../../extlibs/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../src/obj/dynamic/DevIL.bsc"
# ADD BSC32 /nologo /o"../src/obj/dynamic/DevIL.bsc"
LINK32=link.exe
# ADD BASE LINK32 delayimp.lib user32.lib gdi32.lib comdlg32.lib shell32.lib /nologo /dll /machine:I386 /def:".\il.def" /out:"../../lib/DevIL-l.dll" /OPT:NOWIN98 /delayload:libpng3.dll /delayload:lcms108.dll
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 delayimp.lib user32.lib gdi32.lib comdlg32.lib shell32.lib /nologo /dll /machine:I386 /def:".\il.def" /out:"../../lib/DevIL-l.dll" /libpath:"../../extlibs/lib" /OPT:NOWIN98 /delayload:lcms108.dll /delayload:libpng.dll
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetName=DevIL-l
SOURCE="$(InputPath)"
PostBuild_Cmds=..\..\projects\msvc\insdll.bat ..\..\lib\$(TargetName).dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "IL - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../../lib"
# PROP BASE Intermediate_Dir "../src/obj"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../lib"
# PROP Intermediate_Dir "../src/obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /D "IL_STATIC_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O1 /I "../../projects/msvc/extlibs/" /I "../include" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IL_EXPORTS" /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\include" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"../src/obj/DevIL.bsc"
# ADD BSC32 /nologo /o"../src/obj/DevIL.bsc"
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib comdlg32.lib shell32.lib /nologo /dll /machine:I386 /def:".\il.def" /out:"../../lib/DevIL.dll" /OPT:NOWIN98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 user32.lib gdi32.lib comdlg32.lib shell32.lib /nologo /dll /machine:I386 /def:".\il.def" /out:"../../lib/DevIL.dll" /libpath:"../../projects/msvc/extlibs" /OPT:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "IL - Win32 Debug"
# Name "IL - Win32 Dynamic"
# Name "IL - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\il.def

!IF  "$(CFG)" == "IL - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "IL - Win32 Dynamic"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "IL - Win32 Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\il_alloc.c
# End Source File
# Begin Source File

SOURCE=..\src\il_bits.c
# End Source File
# Begin Source File

SOURCE=..\src\il_bmp.c
# End Source File
# Begin Source File

SOURCE=..\src\il_convbuff.c
# End Source File
# Begin Source File

SOURCE=..\src\il_convert.c
# End Source File
# Begin Source File

SOURCE=..\src\il_cut.c
# End Source File
# Begin Source File

SOURCE=..\src\il_dcx.c
# End Source File
# Begin Source File

SOURCE="..\src\il_dds-save.c"
# End Source File
# Begin Source File

SOURCE=..\src\il_dds.c
# End Source File
# Begin Source File

SOURCE=..\src\il_devil.c
# End Source File
# Begin Source File

SOURCE=..\src\il_doom.c
# End Source File
# Begin Source File

SOURCE=..\src\il_endian.c
# End Source File
# Begin Source File

SOURCE=..\src\il_error.c
# End Source File
# Begin Source File

SOURCE=..\src\il_fastconv.c
# End Source File
# Begin Source File

SOURCE=..\src\il_files.c
# End Source File
# Begin Source File

SOURCE=..\src\il_gif.c
# End Source File
# Begin Source File

SOURCE=..\src\il_header.c
# End Source File
# Begin Source File

SOURCE=..\src\il_icon.c
# End Source File
# Begin Source File

SOURCE=..\src\il_internal.c
# End Source File
# Begin Source File

SOURCE=..\src\il_io.c
# End Source File
# Begin Source File

SOURCE=..\src\il_jpeg.c
# End Source File
# Begin Source File

SOURCE=..\src\il_lif.c
# End Source File
# Begin Source File

SOURCE=..\src\il_main.c
# End Source File
# Begin Source File

SOURCE=..\src\il_manip.c
# End Source File
# Begin Source File

SOURCE=..\src\il_mdl.c
# End Source File
# Begin Source File

SOURCE=..\src\il_mng.c
# End Source File
# Begin Source File

SOURCE=..\src\il_neuquant.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pal.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pcd.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pcx.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pic.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pix.c
# End Source File
# Begin Source File

SOURCE=..\src\il_png.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pnm.c
# End Source File
# Begin Source File

SOURCE=..\src\il_profiles.c
# End Source File
# Begin Source File

SOURCE=..\src\il_psd.c
# End Source File
# Begin Source File

SOURCE=..\src\il_psp.c
# End Source File
# Begin Source File

SOURCE=..\src\il_pxr.c
# End Source File
# Begin Source File

SOURCE=..\src\il_quantizer.c
# End Source File
# Begin Source File

SOURCE=..\src\il_raw.c
# End Source File
# Begin Source File

SOURCE=..\src\il_rawdata.c
# End Source File
# Begin Source File

SOURCE=..\src\il_register.c
# End Source File
# Begin Source File

SOURCE=..\src\il_rle.c
# End Source File
# Begin Source File

SOURCE=..\src\il_sgi.c
# End Source File
# Begin Source File

SOURCE=..\src\il_stack.c
# End Source File
# Begin Source File

SOURCE=..\src\il_states.c
# End Source File
# Begin Source File

SOURCE=..\src\il_targa.c
# End Source File
# Begin Source File

SOURCE=..\src\il_tiff.c
# End Source File
# Begin Source File

SOURCE=..\src\il_utility.c
# End Source File
# Begin Source File

SOURCE=..\src\il_wal.c
# End Source File
# Begin Source File

SOURCE=..\src\il_xpm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\IL\config.h
# End Source File
# Begin Source File

SOURCE=..\..\include\IL\devil_internal_exports.h
# End Source File
# Begin Source File

SOURCE=..\..\include\il\il.h
# End Source File
# Begin Source File

SOURCE=..\include\il_alloc.h
# End Source File
# Begin Source File

SOURCE=..\include\il_bits.h
# End Source File
# Begin Source File

SOURCE=..\include\il_bmp.h
# End Source File
# Begin Source File

SOURCE=..\include\il_dcx.h
# End Source File
# Begin Source File

SOURCE=..\include\il_dds.h
# End Source File
# Begin Source File

SOURCE=..\include\il_doompal.h
# End Source File
# Begin Source File

SOURCE=..\include\il_endian.h
# End Source File
# Begin Source File

SOURCE=..\include\il_files.h
# End Source File
# Begin Source File

SOURCE=..\include\il_gif.h
# End Source File
# Begin Source File

SOURCE=..\include\il_icon.h
# End Source File
# Begin Source File

SOURCE=..\include\il_internal.h
# End Source File
# Begin Source File

SOURCE=..\include\il_jpeg.h
# End Source File
# Begin Source File

SOURCE=..\include\il_lif.h
# End Source File
# Begin Source File

SOURCE=..\include\il_manip.h
# End Source File
# Begin Source File

SOURCE=..\include\il_mdl.h
# End Source File
# Begin Source File

SOURCE=..\include\il_pal.h
# End Source File
# Begin Source File

SOURCE=..\include\il_pcx.h
# End Source File
# Begin Source File

SOURCE=..\include\il_pic.h
# End Source File
# Begin Source File

SOURCE=..\include\il_pnm.h
# End Source File
# Begin Source File

SOURCE=..\include\il_psd.h
# End Source File
# Begin Source File

SOURCE=..\include\il_psp.h
# End Source File
# Begin Source File

SOURCE=..\include\il_q2pal.h
# End Source File
# Begin Source File

SOURCE=..\include\il_register.h
# End Source File
# Begin Source File

SOURCE=..\include\il_rle.h
# End Source File
# Begin Source File

SOURCE=..\include\il_sgi.h
# End Source File
# Begin Source File

SOURCE=..\include\il_stack.h
# End Source File
# Begin Source File

SOURCE=..\include\il_states.h
# End Source File
# Begin Source File

SOURCE=..\include\il_targa.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\resources\IL Logo.ico"
# End Source File
# Begin Source File

SOURCE=.\IL.rc
# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter "txt"
# Begin Source File

SOURCE=..\..\AUTHORS
# End Source File
# Begin Source File

SOURCE=..\..\BUGS
# End Source File
# Begin Source File

SOURCE=..\..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\..\CREDITS
# End Source File
# Begin Source File

SOURCE=..\..\libraries.txt
# End Source File
# Begin Source File

SOURCE="..\..\MSVC++.txt"
# End Source File
# Begin Source File

SOURCE=..\..\README.unix
# End Source File
# Begin Source File

SOURCE=..\..\README.win
# End Source File
# End Group
# End Target
# End Project
