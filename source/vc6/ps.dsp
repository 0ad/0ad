# Microsoft Developer Studio Project File - Name="ps" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ps - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ps.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ps.mak" CFG="ps - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ps - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ps - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ps - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\lib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "NO_GUI" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /entry:"entry" /subsystem:windows /pdb:"..\..\binaries/ps.vc6.pdb" /machine:I386 /out:"..\..\binaries\ps.vc6.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ps - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\lib" /I "..\ps" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib /nologo /entry:"entry" /subsystem:windows /pdb:"..\..\binaries/ps_dbg.vc6.pdb" /debug /machine:I386 /out:"..\..\binaries\ps_dbg.vc6.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ps - Win32 Release"
# Name "ps - Win32 Debug"
# Begin Group "terrain"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\terrain\Bound.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Bound.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Camera.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Camera.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Frustum.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Frustum.h
# End Source File
# Begin Source File

SOURCE=..\terrain\LightEnv.h
# End Source File
# Begin Source File

SOURCE=..\terrain\MathUtil.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Matrix3D.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Matrix3D.h
# End Source File
# Begin Source File

SOURCE=..\terrain\MiniPatch.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\MiniPatch.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Model.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Model.h
# End Source File
# Begin Source File

SOURCE=..\terrain\ModelDef.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\ModelDef.h
# End Source File
# Begin Source File

SOURCE=..\terrain\ModelFile.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\ModelFile.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Patch.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Patch.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Plane.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Plane.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Quaternion.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Quaternion.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Renderer.h
# End Source File
# Begin Source File

SOURCE=..\terrain\SHCoeffs.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\SHCoeffs.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Terrain.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Terrain.h
# End Source File
# Begin Source File

SOURCE=..\terrain\terrainMain.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\TerrGlobals.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Texture.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Types.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Vector3D.cpp
# End Source File
# Begin Source File

SOURCE=..\terrain\Vector3D.h
# End Source File
# Begin Source File

SOURCE=..\terrain\Vector4D.h
# End Source File
# End Group
# Begin Group "gui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\gui\CButton.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\CButton.h
# End Source File
# Begin Source File

SOURCE=..\gui\CGUI.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\CGUI.h
# End Source File
# Begin Source File

SOURCE=..\gui\CGUISprite.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\CGUISprite.h
# End Source File
# Begin Source File

SOURCE=..\gui\GUI.h
# End Source File
# Begin Source File

SOURCE=..\gui\GUIbase.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\GUIbase.h
# End Source File
# Begin Source File

SOURCE=..\gui\GUIutil.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\GUIutil.h
# End Source File
# Begin Source File

SOURCE=..\gui\IGUIButtonBehavior.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\IGUIButtonBehavior.h
# End Source File
# Begin Source File

SOURCE=..\gui\IGUIObject.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\IGUIObject.h
# End Source File
# Begin Source File

SOURCE=..\gui\IGUISettingsObject.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\IGUISettingsObject.h
# End Source File
# Begin Source File

SOURCE=..\gui\XercesErrorHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\gui\XercesErrorHandler.h
# End Source File
# End Group
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Group "posix"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lib\posix\aio.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\posix\aio.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\lib\detect.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\detect.h
# End Source File
# Begin Source File

SOURCE=..\lib\font.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\font.h
# End Source File
# Begin Source File

SOURCE=..\lib\glext_funcs.h
# End Source File
# Begin Source File

SOURCE=..\lib\ia32.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\ia32.h
# End Source File
# Begin Source File

SOURCE=..\lib\input.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\input.h
# End Source File
# Begin Source File

SOURCE=..\lib\mem.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\mem.h
# End Source File
# Begin Source File

SOURCE=..\lib\misc.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\misc.h
# End Source File
# Begin Source File

SOURCE=..\lib\ogl.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\ogl.h
# End Source File
# Begin Source File

SOURCE=..\lib\posix.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\posix.h
# End Source File
# Begin Source File

SOURCE=..\lib\res.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\res.h
# End Source File
# Begin Source File

SOURCE=..\lib\tex.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\tex.h
# End Source File
# Begin Source File

SOURCE=..\lib\time.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\time.h
# End Source File
# Begin Source File

SOURCE=..\lib\types.h
# End Source File
# Begin Source File

SOURCE=..\lib\vfs.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\vfs.h
# End Source File
# Begin Source File

SOURCE=..\lib\win.h
# End Source File
# Begin Source File

SOURCE=..\lib\wsdl.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\wsdl.h
# End Source File
# Begin Source File

SOURCE=..\lib\zip.cpp
# End Source File
# Begin Source File

SOURCE=..\lib\zip.h
# End Source File
# End Group
# Begin Group "ps"

# PROP Default_Filter ""
# Begin Group "Network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\ps\Network\AllNetMessages.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\NetMessage.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Network\NetMessage.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\Network.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Network\Network.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\NetworkInternal.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\NMTCreator.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\Serialization.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\ServerSocket.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Network\SocketBase.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Network\SocketBase.h
# End Source File
# Begin Source File

SOURCE=..\ps\Network\StreamSocket.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\Network\StreamSocket.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\ps\BaseEntity.h
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

SOURCE=..\ps\LogFile.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\LogFile.h
# End Source File
# Begin Source File

SOURCE=..\ps\MathUtil.cpp
# End Source File
# Begin Source File

SOURCE=..\ps\MathUtil.h
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

SOURCE=..\ps\Singleton.h
# End Source File
# Begin Source File

SOURCE=..\ps\Sound.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\main.cpp
# End Source File
# End Target
# End Project
