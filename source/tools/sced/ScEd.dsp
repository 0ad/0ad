# Microsoft Developer Studio Project File - Name="ScEd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ScEd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ScEd.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ScEd - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GR- /GX /Zi /O2 /Ob2 /I "..\\" /I "..\lib" /I "..\ps" /I "..\simulation" /I "..\maths" /I "..\graphics" /I "..\renderer" /I ".\\" /I ".\ui" /D "NDEBUG" /D "_MBCS" /D "_WINDOWS" /D "WIN32" /D "_NO_WINMAIN_" /D "_WGL_H_INCLUDED" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 nafxcw.lib opengl32.lib glu32.lib ws2_32.lib version.lib xerces-c_2.lib /nologo /entry:"entry" /subsystem:windows /map /debug /machine:I386 /out:"D:\0ad\binaries\system\ScEd.exe" /libpath:"..\libs" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ScEd - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /Gi /GX /Zi /Od /I "..\\" /I "..\lib" /I "..\ps" /I "..\simulation" /I "..\maths" /I "..\graphics" /I "..\renderer" /I ".\\" /I ".\ui" /D "_DEBUG" /D "_MBCS" /D "_WINDOWS" /D "WIN32" /D "_NO_WINMAIN_" /D "_WGL_H_INCLUDED" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 nafxcwd.lib opengl32.lib glu32.lib ws2_32.lib version.lib xerces-c_2D.lib /nologo /entry:"entry" /subsystem:windows /incremental:no /map /debug /machine:I386 /out:"D:\0ad\binaries\system\ScEd_d.exe" /libpath:"..\libs" /fixed:no
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ScEd - Win32 Release"
# Name "ScEd - Win32 Debug"
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Group "res"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lib\res\file.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\file.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\font.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\font.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\h_mgr.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\h_mgr.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\mem.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\mem.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\res.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\res.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\tex.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\tex.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\vfs.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\vfs.h
# End Source File
# Begin Source File

SOURCE=..\lib\res\zip.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res\zip.h
# End Source File
# End Group
# Begin Group "sysdep"

# PROP Default_Filter ""
# Begin Group "win"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lib\sysdep\win\waio.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\waio.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wdetect.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wfam.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wfam.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wgl.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\win.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\win.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\win_internal.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wposix.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wposix.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wsdl.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wsdl.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wsock.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wsock.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wtime.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\win\wtime.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\sysdep\ia32.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\ia32.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\sysdep.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\sysdep.h
# End Source File
# Begin Source File

SOURCE=..\lib\sysdep\x.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\adts.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\adts.h
# End Source File
# Begin Source File

SOURCE=..\lib\config.h
# End Source File
# Begin Source File

SOURCE=..\lib\detect.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\detect.h
# End Source File
# Begin Source File

SOURCE=..\lib\glext_funcs.h
# End Source File
# Begin Source File

SOURCE=..\lib\lib.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\lib.h
# End Source File
# Begin Source File

SOURCE=..\lib\memcpy.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\ogl.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\ogl.h
# End Source File
# Begin Source File

SOURCE=..\lib\posix.h
# End Source File
# Begin Source File

SOURCE=..\lib\precompiled.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\precompiled.h
# End Source File
# Begin Source File

SOURCE=..\lib\sdl.h
# End Source File
# Begin Source File

SOURCE=..\lib\SDL_keysym.h
# End Source File
# Begin Source File

SOURCE=..\lib\SDL_vkeys.h
# End Source File
# Begin Source File

SOURCE=..\lib\timer.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\timer.h
# End Source File
# Begin Source File

SOURCE=..\lib\types.h
# End Source File
# End Group
# Begin Group "simulation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\simulation\BaseEntity.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\BaseEntity.h
# End Source File
# Begin Source File

SOURCE=..\simulation\BaseEntityCollection.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\BaseEntityCollection.h
# End Source File
# Begin Source File

SOURCE=..\simulation\BoundingObjects.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\BoundingObjects.h
# End Source File
# Begin Source File

SOURCE=..\simulation\Collision.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\Collision.h
# End Source File
# Begin Source File

SOURCE=..\simulation\Entity.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\Entity.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityHandles.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityHandles.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityManager.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityManager.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityMessage.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityOrders.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityProperties.h
# End Source File
# Begin Source File

SOURCE=..\simulation\EntityStateProcessing.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\PathfindEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\PathfindEngine.h
# End Source File
# Begin Source File

SOURCE=..\simulation\PathfindSparse.cpp
# End Source File
# Begin Source File

SOURCE=..\simulation\PathfindSparse.h
# End Source File
# End Group
# Begin Group "graphics"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\graphics\Camera.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Camera.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Color.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Frustum.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Frustum.h
# End Source File
# Begin Source File

SOURCE=..\graphics\HFTracer.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\HFTracer.h
# End Source File
# Begin Source File

SOURCE=..\graphics\LightEnv.h
# End Source File
# Begin Source File

SOURCE=..\graphics\MapIO.h
# End Source File
# Begin Source File

SOURCE=..\graphics\MapReader.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\MapReader.h
# End Source File
# Begin Source File

SOURCE=..\graphics\MapWriter.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\MapWriter.h
# End Source File
# Begin Source File

SOURCE=..\graphics\MiniPatch.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\MiniPatch.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Model.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Model.h
# End Source File
# Begin Source File

SOURCE=..\graphics\ModelDef.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\ModelDef.h
# End Source File
# Begin Source File

SOURCE=..\graphics\ObjectEntry.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\ObjectEntry.h
# End Source File
# Begin Source File

SOURCE=..\graphics\ObjectManager.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\ObjectManager.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Particle.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Particle.h
# End Source File
# Begin Source File

SOURCE=..\graphics\ParticleEmitter.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\ParticleEmitter.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Patch.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Patch.h
# End Source File
# Begin Source File

SOURCE=..\graphics\RenderableObject.h
# End Source File
# Begin Source File

SOURCE=..\graphics\SkeletonAnim.h
# End Source File
# Begin Source File

SOURCE=..\graphics\SkeletonAnimDef.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\SkeletonAnimDef.h
# End Source File
# Begin Source File

SOURCE=..\graphics\SkeletonAnimManager.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\SkeletonAnimManager.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Sprite.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Sprite.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Terrain.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\Terrain.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Texture.h
# End Source File
# Begin Source File

SOURCE=..\graphics\TextureEntry.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\TextureEntry.h
# End Source File
# Begin Source File

SOURCE=..\graphics\TextureManager.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\TextureManager.h
# End Source File
# Begin Source File

SOURCE=..\graphics\Unit.h
# End Source File
# Begin Source File

SOURCE=..\graphics\UnitManager.cpp
# End Source File
# Begin Source File

SOURCE=..\graphics\UnitManager.h
# End Source File
# End Group
# Begin Group "maths"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\maths\Bound.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\Bound.h
# End Source File
# Begin Source File

SOURCE=..\maths\MathUtil.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\MathUtil.h
# End Source File
# Begin Source File

SOURCE=..\maths\Matrix3D.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\Matrix3D.h
# End Source File
# Begin Source File

SOURCE=..\maths\Plane.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\Plane.h
# End Source File
# Begin Source File

SOURCE=..\maths\Quaternion.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\Quaternion.h
# End Source File
# Begin Source File

SOURCE=..\maths\Vector3D.cpp
# End Source File
# Begin Source File

SOURCE=..\maths\Vector3D.h
# End Source File
# Begin Source File

SOURCE=..\maths\Vector4D.h
# End Source File
# End Group
# Begin Group "renderer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\renderer\AlphaMapCalculator.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\AlphaMapCalculator.h
# End Source File
# Begin Source File

SOURCE=..\renderer\BlendShapes.h
# End Source File
# Begin Source File

SOURCE=..\renderer\ModelRData.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\ModelRData.h
# End Source File
# Begin Source File

SOURCE=..\renderer\PatchRData.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\PatchRData.h
# End Source File
# Begin Source File

SOURCE=..\renderer\Renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\Renderer.h
# End Source File
# Begin Source File

SOURCE=..\renderer\SHCoeffs.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\SHCoeffs.h
# End Source File
# Begin Source File

SOURCE=..\renderer\TransparencyRenderer.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\TransparencyRenderer.h
# End Source File
# Begin Source File

SOURCE=..\renderer\VertexBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\VertexBuffer.h
# End Source File
# Begin Source File

SOURCE=..\renderer\VertexBufferManager.cpp
# End Source File
# Begin Source File

SOURCE=..\renderer\VertexBufferManager.h
# End Source File
# End Group
# Begin Group "sced"

# PROP Default_Filter ""
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\0ad_logo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\addunit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap_s.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\res\cursor\camera_dolly.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor\camera_pan.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor\camera_zoom.cur
# End Source File
# Begin Source File

SOURCE=.\res\elevationtools.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ico00001.ico
# End Source File
# Begin Source File

SOURCE=.\res\ico00002.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\modeltools.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ScEd.ico
# End Source File
# Begin Source File

SOURCE=.\ui\res\ScEd.ico
# End Source File
# Begin Source File

SOURCE=.\res\ScEd.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ScEdDoc.ico
# End Source File
# Begin Source File

SOURCE=.\ui\res\ScEdDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\select.bmp
# End Source File
# Begin Source File

SOURCE=.\res\selectunit.bmp
# End Source File
# Begin Source File

SOURCE=.\res\terraintools.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "UI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ui\BrushShapeEditorDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\BrushShapeEditorDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\ColorButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ColorButton.h
# End Source File
# Begin Source File

SOURCE=.\ui\DirectionButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\DirectionButton.h
# End Source File
# Begin Source File

SOURCE=.\ui\ElevationButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ElevationButton.h
# End Source File
# Begin Source File

SOURCE=.\ui\ElevToolsDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ElevToolsDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\ImageListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ImageListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\ui\LightSettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\LightSettingsDlg.h
# End Source File
# Begin Source File

SOURCE=.\ui\MainFrameDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\MainFrameDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\ui\MapSizeDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\MapSizeDlg.h
# End Source File
# Begin Source File

SOURCE=.\ui\NavigatePropPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\NavigatePropPage.h
# End Source File
# Begin Source File

SOURCE=.\ui\OptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\OptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\ui\OptionsPropSheet.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\OptionsPropSheet.h
# End Source File
# Begin Source File

SOURCE=.\ui\resource.h
# End Source File
# Begin Source File

SOURCE=.\ui\ScEd.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ScEd.h
# End Source File
# Begin Source File

SOURCE=.\ui\ScEd.rc
# End Source File
# Begin Source File

SOURCE=.\ui\ScEdDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ScEdDoc.h
# End Source File
# Begin Source File

SOURCE=.\ui\ScEdView.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ScEdView.h
# End Source File
# Begin Source File

SOURCE=.\ui\ShadowsPropPage.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\ShadowsPropPage.h
# End Source File
# Begin Source File

SOURCE=.\ui\SimpleEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\SimpleEdit.h
# End Source File
# Begin Source File

SOURCE=.\ui\TexToolsDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\TexToolsDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\UIGlobals.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UIGlobals.h
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesAnimationsTab.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesAnimationsTab.h
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesTabCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesTabCtrl.h
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesTexturesTab.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UnitPropertiesTexturesTab.h
# End Source File
# Begin Source File

SOURCE=.\ui\UnitToolsDlgBar.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\UnitToolsDlgBar.h
# End Source File
# Begin Source File

SOURCE=.\ui\WebLinkButton.cpp
# End Source File
# Begin Source File

SOURCE=.\ui\WebLinkButton.h
# End Source File
# End Group
# Begin Group "Commands"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\AlterElevationCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\AlterElevationCommand.h
# End Source File
# Begin Source File

SOURCE=.\AlterLightEnvCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\AlterLightEnvCommand.h
# End Source File
# Begin Source File

SOURCE=.\Command.h
# End Source File
# Begin Source File

SOURCE=.\PaintObjectCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\PaintObjectCommand.h
# End Source File
# Begin Source File

SOURCE=.\PaintTextureCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\PaintTextureCommand.h
# End Source File
# Begin Source File

SOURCE=.\RaiseElevationCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\RaiseElevationCommand.h
# End Source File
# Begin Source File

SOURCE=.\SmoothElevationCommand.cpp
# End Source File
# Begin Source File

SOURCE=.\SmoothElevationCommand.h
# End Source File
# End Group
# Begin Group "Tools"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\BrushShapeEditorTool.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushShapeEditorTool.h
# End Source File
# Begin Source File

SOURCE=.\BrushTool.cpp
# End Source File
# Begin Source File

SOURCE=.\BrushTool.h
# End Source File
# Begin Source File

SOURCE=.\PaintObjectTool.cpp
# End Source File
# Begin Source File

SOURCE=.\PaintObjectTool.h
# End Source File
# Begin Source File

SOURCE=.\PaintTextureTool.cpp
# End Source File
# Begin Source File

SOURCE=.\PaintTextureTool.h
# End Source File
# Begin Source File

SOURCE=.\RaiseElevationTool.cpp
# End Source File
# Begin Source File

SOURCE=.\RaiseElevationTool.h
# End Source File
# Begin Source File

SOURCE=.\SelectObjectTool.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectObjectTool.h
# End Source File
# Begin Source File

SOURCE=.\SmoothElevationTool.cpp
# End Source File
# Begin Source File

SOURCE=.\SmoothElevationTool.h
# End Source File
# Begin Source File

SOURCE=.\Tool.h
# End Source File
# End Group
# Begin Group "Core"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Array2D.h
# End Source File
# Begin Source File

SOURCE=.\CommandManager.cpp
# End Source File
# Begin Source File

SOURCE=.\CommandManager.h
# End Source File
# Begin Source File

SOURCE=.\EditorData.cpp
# End Source File
# Begin Source File

SOURCE=.\EditorData.h
# End Source File
# Begin Source File

SOURCE=.\InfoBox.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoBox.h
# End Source File
# Begin Source File

SOURCE=.\MiniMap.cpp
# End Source File
# Begin Source File

SOURCE=.\MiniMap.h
# End Source File
# Begin Source File

SOURCE=.\MouseHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\MouseHandler.h
# End Source File
# Begin Source File

SOURCE=.\NaviCam.cpp
# End Source File
# Begin Source File

SOURCE=.\NaviCam.h
# End Source File
# Begin Source File

SOURCE=.\ToolManager.cpp
# End Source File
# Begin Source File

SOURCE=.\ToolManager.h
# End Source File
# Begin Source File

SOURCE=.\UserConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\UserConfig.h
# End Source File
# End Group
# Begin Group "Doc Notes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\binaries\changes.txt
# End Source File
# Begin Source File

SOURCE=..\..\binaries\readme.txt
# End Source File
# End Group
# End Group
# Begin Group "ps"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ps\CLogger.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\CLogger.h
# End Source File
# Begin Source File

SOURCE=..\ps\Config.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Config.h
# End Source File
# Begin Source File

SOURCE=..\ps\CStr.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\CStr.h
# End Source File
# Begin Source File

SOURCE=..\ps\Encryption.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Encryption.h
# End Source File
# Begin Source File

SOURCE=..\ps\Error.h
# End Source File
# Begin Source File

SOURCE=..\ps\FilePacker.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\FilePacker.h
# End Source File
# Begin Source File

SOURCE=..\ps\FileUnpacker.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\FileUnpacker.h
# End Source File
# Begin Source File

SOURCE=..\ps\LogFile.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\LogFile.h
# End Source File
# Begin Source File

SOURCE=..\ps\NPFont.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\NPFont.h
# End Source File
# Begin Source File

SOURCE=..\ps\NPFontManager.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\NPFontManager.h
# End Source File
# Begin Source File

SOURCE=..\ps\Overlay.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Overlay.h
# End Source File
# Begin Source File

SOURCE=..\ps\OverlayText.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\OverlayText.h
# End Source File
# Begin Source File

SOURCE=..\ps\Parser.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Parser.h
# End Source File
# Begin Source File

SOURCE=..\ps\Prometheus.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Prometheus.h
# End Source File
# Begin Source File

SOURCE=..\ps\rc_array.h
# End Source File
# Begin Source File

SOURCE=..\ps\rc_vector.h
# End Source File
# Begin Source File

SOURCE=..\ps\Singleton.h
# End Source File
# Begin Source File

SOURCE=..\ps\Sound.h
# End Source File
# Begin Source File

SOURCE=..\ps\ThreadUtil.h
# End Source File
# Begin Source File

SOURCE=..\ps\Vector2D.h
# End Source File
# Begin Source File

SOURCE=..\ps\XercesErrorHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\XercesErrorHandler.h
# End Source File
# Begin Source File

SOURCE=..\ps\XML.h
# End Source File
# Begin Source File

SOURCE=..\ps\XMLUtils.cpp
# End Source File
# End Group
# End Target
# End Project
