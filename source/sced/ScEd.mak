# Microsoft Developer Studio Generated NMAKE File, Based on ScEd.dsp
!IF "$(CFG)" == ""
CFG=ScEd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ScEd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ScEd - Win32 Release" && "$(CFG)" != "ScEd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ScEd.mak" CFG="ScEd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ScEd - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ScEd - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ScEd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\binaries\ScEd.exe" "$(OUTDIR)\ScEd.pch"

!ELSE 

ALL : "pslib - Win32 Release" "..\..\binaries\ScEd.exe" "$(OUTDIR)\ScEd.pch"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"pslib - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AlterElevationCommand.obj"
	-@erase "$(INTDIR)\BrushTool.obj"
	-@erase "$(INTDIR)\ColorButton.obj"
	-@erase "$(INTDIR)\CommandManager.obj"
	-@erase "$(INTDIR)\DirectionButton.obj"
	-@erase "$(INTDIR)\EditorData.obj"
	-@erase "$(INTDIR)\ElevationButton.obj"
	-@erase "$(INTDIR)\ElevToolsDlgBar.obj"
	-@erase "$(INTDIR)\HFTracer.obj"
	-@erase "$(INTDIR)\ImageListCtrl.obj"
	-@erase "$(INTDIR)\InfoBox.obj"
	-@erase "$(INTDIR)\LightSettingsDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MapSizeDlg.obj"
	-@erase "$(INTDIR)\MiniMap.obj"
	-@erase "$(INTDIR)\ObjectEntry.obj"
	-@erase "$(INTDIR)\ObjectManager.obj"
	-@erase "$(INTDIR)\OptionsDlg.obj"
	-@erase "$(INTDIR)\PaintObjectCommand.obj"
	-@erase "$(INTDIR)\PaintObjectTool.obj"
	-@erase "$(INTDIR)\PaintTextureCommand.obj"
	-@erase "$(INTDIR)\PaintTextureTool.obj"
	-@erase "$(INTDIR)\RaiseElevationCommand.obj"
	-@erase "$(INTDIR)\RaiseElevationTool.obj"
	-@erase "$(INTDIR)\ScEd.obj"
	-@erase "$(INTDIR)\ScEd.pch"
	-@erase "$(INTDIR)\ScEd.res"
	-@erase "$(INTDIR)\ScEdDoc.obj"
	-@erase "$(INTDIR)\ScEdView.obj"
	-@erase "$(INTDIR)\SimpleEdit.obj"
	-@erase "$(INTDIR)\SMDConverter.obj"
	-@erase "$(INTDIR)\SmoothElevationCommand.obj"
	-@erase "$(INTDIR)\SmoothElevationTool.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TexToolsDlgBar.obj"
	-@erase "$(INTDIR)\TextureManager.obj"
	-@erase "$(INTDIR)\ToolManager.obj"
	-@erase "$(INTDIR)\UIGlobals.obj"
	-@erase "$(INTDIR)\UnitManager.obj"
	-@erase "$(INTDIR)\UnitPropertiesDlgBar.obj"
	-@erase "$(INTDIR)\UnitToolsDlgBar.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WebLinkButton.obj"
	-@erase "$(OUTDIR)\ScEd.pdb"
	-@erase "..\..\binaries\ScEd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G5 /MT /W3 /GX /Zi /O2 /Ob0 /I "..\\" /I "..\lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ScEd.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ScEd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=nafxcw.lib pslib.lib opengl32.lib glu32.lib ws2_32.lib version.lib xerces-c_2.lib /nologo /entry:"entry" /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\ScEd.pdb" /debug /machine:I386 /out:"D:\0ad\binaries\ScEd.exe" /libpath:"..\libs" /fixed:no 
LINK32_OBJS= \
	"$(INTDIR)\ColorButton.obj" \
	"$(INTDIR)\DirectionButton.obj" \
	"$(INTDIR)\ElevationButton.obj" \
	"$(INTDIR)\ElevToolsDlgBar.obj" \
	"$(INTDIR)\ImageListCtrl.obj" \
	"$(INTDIR)\LightSettingsDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MapSizeDlg.obj" \
	"$(INTDIR)\OptionsDlg.obj" \
	"$(INTDIR)\ScEd.obj" \
	"$(INTDIR)\ScEdDoc.obj" \
	"$(INTDIR)\ScEdView.obj" \
	"$(INTDIR)\SimpleEdit.obj" \
	"$(INTDIR)\TexToolsDlgBar.obj" \
	"$(INTDIR)\UIGlobals.obj" \
	"$(INTDIR)\UnitPropertiesDlgBar.obj" \
	"$(INTDIR)\UnitToolsDlgBar.obj" \
	"$(INTDIR)\WebLinkButton.obj" \
	"$(INTDIR)\AlterElevationCommand.obj" \
	"$(INTDIR)\PaintObjectCommand.obj" \
	"$(INTDIR)\PaintTextureCommand.obj" \
	"$(INTDIR)\RaiseElevationCommand.obj" \
	"$(INTDIR)\SmoothElevationCommand.obj" \
	"$(INTDIR)\BrushTool.obj" \
	"$(INTDIR)\PaintObjectTool.obj" \
	"$(INTDIR)\PaintTextureTool.obj" \
	"$(INTDIR)\RaiseElevationTool.obj" \
	"$(INTDIR)\SmoothElevationTool.obj" \
	"$(INTDIR)\CommandManager.obj" \
	"$(INTDIR)\EditorData.obj" \
	"$(INTDIR)\InfoBox.obj" \
	"$(INTDIR)\MiniMap.obj" \
	"$(INTDIR)\ObjectEntry.obj" \
	"$(INTDIR)\ObjectManager.obj" \
	"$(INTDIR)\SMDConverter.obj" \
	"$(INTDIR)\TextureManager.obj" \
	"$(INTDIR)\ToolManager.obj" \
	"$(INTDIR)\UnitManager.obj" \
	"$(INTDIR)\HFTracer.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\ScEd.res"

"..\..\binaries\ScEd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "ScEd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "..\..\binaries\ScEd_d.exe" "$(OUTDIR)\ScEd.pch"

!ELSE 

ALL : "pslib - Win32 Debug" "..\..\binaries\ScEd_d.exe" "$(OUTDIR)\ScEd.pch"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"pslib - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AlterElevationCommand.obj"
	-@erase "$(INTDIR)\BrushTool.obj"
	-@erase "$(INTDIR)\ColorButton.obj"
	-@erase "$(INTDIR)\CommandManager.obj"
	-@erase "$(INTDIR)\DirectionButton.obj"
	-@erase "$(INTDIR)\EditorData.obj"
	-@erase "$(INTDIR)\ElevationButton.obj"
	-@erase "$(INTDIR)\ElevToolsDlgBar.obj"
	-@erase "$(INTDIR)\HFTracer.obj"
	-@erase "$(INTDIR)\ImageListCtrl.obj"
	-@erase "$(INTDIR)\InfoBox.obj"
	-@erase "$(INTDIR)\LightSettingsDlg.obj"
	-@erase "$(INTDIR)\MainFrm.obj"
	-@erase "$(INTDIR)\MapSizeDlg.obj"
	-@erase "$(INTDIR)\MiniMap.obj"
	-@erase "$(INTDIR)\ObjectEntry.obj"
	-@erase "$(INTDIR)\ObjectManager.obj"
	-@erase "$(INTDIR)\OptionsDlg.obj"
	-@erase "$(INTDIR)\PaintObjectCommand.obj"
	-@erase "$(INTDIR)\PaintObjectTool.obj"
	-@erase "$(INTDIR)\PaintTextureCommand.obj"
	-@erase "$(INTDIR)\PaintTextureTool.obj"
	-@erase "$(INTDIR)\RaiseElevationCommand.obj"
	-@erase "$(INTDIR)\RaiseElevationTool.obj"
	-@erase "$(INTDIR)\ScEd.obj"
	-@erase "$(INTDIR)\ScEd.pch"
	-@erase "$(INTDIR)\ScEd.res"
	-@erase "$(INTDIR)\ScEdDoc.obj"
	-@erase "$(INTDIR)\ScEdView.obj"
	-@erase "$(INTDIR)\SimpleEdit.obj"
	-@erase "$(INTDIR)\SMDConverter.obj"
	-@erase "$(INTDIR)\SmoothElevationCommand.obj"
	-@erase "$(INTDIR)\SmoothElevationTool.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\TexToolsDlgBar.obj"
	-@erase "$(INTDIR)\TextureManager.obj"
	-@erase "$(INTDIR)\ToolManager.obj"
	-@erase "$(INTDIR)\UIGlobals.obj"
	-@erase "$(INTDIR)\UnitManager.obj"
	-@erase "$(INTDIR)\UnitPropertiesDlgBar.obj"
	-@erase "$(INTDIR)\UnitToolsDlgBar.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\WebLinkButton.obj"
	-@erase "$(OUTDIR)\ScEd_d.pdb"
	-@erase "..\..\binaries\ScEd_d.exe"
	-@erase "..\..\binaries\ScEd_d.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G6 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\\" /I "..\lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\ScEd.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\ScEd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=nafxcwd.lib pslib_d.lib opengl32.lib glu32.lib ws2_32.lib version.lib xerces-c_2D.lib /nologo /entry:"entry" /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\ScEd_d.pdb" /debug /machine:I386 /out:"D:\0ad\binaries\ScEd_d.exe" /pdbtype:sept /libpath:"..\libs" 
LINK32_OBJS= \
	"$(INTDIR)\ColorButton.obj" \
	"$(INTDIR)\DirectionButton.obj" \
	"$(INTDIR)\ElevationButton.obj" \
	"$(INTDIR)\ElevToolsDlgBar.obj" \
	"$(INTDIR)\ImageListCtrl.obj" \
	"$(INTDIR)\LightSettingsDlg.obj" \
	"$(INTDIR)\MainFrm.obj" \
	"$(INTDIR)\MapSizeDlg.obj" \
	"$(INTDIR)\OptionsDlg.obj" \
	"$(INTDIR)\ScEd.obj" \
	"$(INTDIR)\ScEdDoc.obj" \
	"$(INTDIR)\ScEdView.obj" \
	"$(INTDIR)\SimpleEdit.obj" \
	"$(INTDIR)\TexToolsDlgBar.obj" \
	"$(INTDIR)\UIGlobals.obj" \
	"$(INTDIR)\UnitPropertiesDlgBar.obj" \
	"$(INTDIR)\UnitToolsDlgBar.obj" \
	"$(INTDIR)\WebLinkButton.obj" \
	"$(INTDIR)\AlterElevationCommand.obj" \
	"$(INTDIR)\PaintObjectCommand.obj" \
	"$(INTDIR)\PaintTextureCommand.obj" \
	"$(INTDIR)\RaiseElevationCommand.obj" \
	"$(INTDIR)\SmoothElevationCommand.obj" \
	"$(INTDIR)\BrushTool.obj" \
	"$(INTDIR)\PaintObjectTool.obj" \
	"$(INTDIR)\PaintTextureTool.obj" \
	"$(INTDIR)\RaiseElevationTool.obj" \
	"$(INTDIR)\SmoothElevationTool.obj" \
	"$(INTDIR)\CommandManager.obj" \
	"$(INTDIR)\EditorData.obj" \
	"$(INTDIR)\InfoBox.obj" \
	"$(INTDIR)\MiniMap.obj" \
	"$(INTDIR)\ObjectEntry.obj" \
	"$(INTDIR)\ObjectManager.obj" \
	"$(INTDIR)\SMDConverter.obj" \
	"$(INTDIR)\TextureManager.obj" \
	"$(INTDIR)\ToolManager.obj" \
	"$(INTDIR)\UnitManager.obj" \
	"$(INTDIR)\HFTracer.obj" \
	"$(INTDIR)\StdAfx.obj" \
	"$(INTDIR)\ScEd.res"

"..\..\binaries\ScEd_d.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("ScEd.dep")
!INCLUDE "ScEd.dep"
!ELSE 
!MESSAGE Warning: cannot find "ScEd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "ScEd - Win32 Release" || "$(CFG)" == "ScEd - Win32 Debug"
SOURCE=.\ColorButton.cpp

"$(INTDIR)\ColorButton.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\DirectionButton.cpp

"$(INTDIR)\DirectionButton.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ElevationButton.cpp

"$(INTDIR)\ElevationButton.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ElevToolsDlgBar.cpp

"$(INTDIR)\ElevToolsDlgBar.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ImageListCtrl.cpp

"$(INTDIR)\ImageListCtrl.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\LightSettingsDlg.cpp

"$(INTDIR)\LightSettingsDlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MainFrm.cpp

"$(INTDIR)\MainFrm.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MapSizeDlg.cpp

"$(INTDIR)\MapSizeDlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OptionsDlg.cpp

"$(INTDIR)\OptionsDlg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ScEd.cpp

"$(INTDIR)\ScEd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ScEd.rc

"$(INTDIR)\ScEd.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\ScEdDoc.cpp

"$(INTDIR)\ScEdDoc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ScEdView.cpp

"$(INTDIR)\ScEdView.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SimpleEdit.cpp

"$(INTDIR)\SimpleEdit.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TexToolsDlgBar.cpp

"$(INTDIR)\TexToolsDlgBar.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\UIGlobals.cpp

"$(INTDIR)\UIGlobals.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\UnitPropertiesDlgBar.cpp

"$(INTDIR)\UnitPropertiesDlgBar.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\UnitToolsDlgBar.cpp

"$(INTDIR)\UnitToolsDlgBar.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\WebLinkButton.cpp

"$(INTDIR)\WebLinkButton.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\AlterElevationCommand.cpp

"$(INTDIR)\AlterElevationCommand.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PaintObjectCommand.cpp

"$(INTDIR)\PaintObjectCommand.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PaintTextureCommand.cpp

"$(INTDIR)\PaintTextureCommand.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\RaiseElevationCommand.cpp

"$(INTDIR)\RaiseElevationCommand.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SmoothElevationCommand.cpp

"$(INTDIR)\SmoothElevationCommand.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\BrushTool.cpp

"$(INTDIR)\BrushTool.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PaintObjectTool.cpp

"$(INTDIR)\PaintObjectTool.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\PaintTextureTool.cpp

"$(INTDIR)\PaintTextureTool.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\RaiseElevationTool.cpp

"$(INTDIR)\RaiseElevationTool.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SmoothElevationTool.cpp

"$(INTDIR)\SmoothElevationTool.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\CommandManager.cpp

"$(INTDIR)\CommandManager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\EditorData.cpp

"$(INTDIR)\EditorData.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\InfoBox.cpp

"$(INTDIR)\InfoBox.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\MiniMap.cpp

"$(INTDIR)\MiniMap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ObjectEntry.cpp

"$(INTDIR)\ObjectEntry.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ObjectManager.cpp

"$(INTDIR)\ObjectManager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SMDConverter.cpp

"$(INTDIR)\SMDConverter.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\TextureManager.cpp

"$(INTDIR)\TextureManager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ToolManager.cpp

"$(INTDIR)\ToolManager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\UnitManager.cpp

"$(INTDIR)\UnitManager.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\HFTracer.cpp

"$(INTDIR)\HFTracer.obj" : $(SOURCE) "$(INTDIR)"


!IF  "$(CFG)" == "ScEd - Win32 Release"

"pslib - Win32 Release" : 
   cd "\0ad\fw\pslib"
   NMAKE /f pslib.mak
   cd "..\ScEd"

"pslib - Win32 ReleaseCLEAN" : 
   cd "\0ad\fw\pslib"
   cd "..\ScEd"

!ELSEIF  "$(CFG)" == "ScEd - Win32 Debug"

"pslib - Win32 Debug" : 
   cd "\0ad\fw\pslib"
   NMAKE /f pslib.mak
   cd "..\ScEd"

"pslib - Win32 DebugCLEAN" : 
   cd "\0ad\fw\pslib"
   cd "..\ScEd"

!ENDIF 

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "ScEd - Win32 Release"

CPP_SWITCHES=/nologo /G5 /MT /W3 /GX /Zi /O2 /Ob0 /I "..\\" /I "..\lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\ScEd.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\ScEd.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ScEd - Win32 Debug"

CPP_SWITCHES=/nologo /G6 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\\" /I "..\lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fp"$(INTDIR)\ScEd.pch" /Yc"stdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\StdAfx.obj"	"$(INTDIR)\ScEd.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

