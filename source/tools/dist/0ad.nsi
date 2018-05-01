; To generate the installer (on Linux):
;  Do an 'svn export' into a directory called e.g. "export-win32"
;  makensis -nocd -dcheckoutpath=export-win32 -drevision=1234 -dprefix=0ad-0.1.2-alpha export-win32/source/tools/dist/0ad.nsi

  SetCompressor /SOLID lzma

  !include "MUI2.nsh"
  !include "LogicLib.nsh"
  !include "FileAssociation.nsh"

  ;Control whether to include source code (and component selection screen)
  !define INCLUDE_SOURCE 0

;--------------------------------
;General

  ;Properly display all languages (Installer will not work on Windows 95, 98 or ME!)
  Unicode true

  ;Name and file
  Name "0 A.D."
  OutFile "${PREFIX}-win32.exe"

  ;Default installation folder
  InstallDir "$LOCALAPPDATA\0 A.D. alpha"
  ; NOTE: we can't use folder names ending in "." because they seemingly get stripped

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\0 A.D." ""

  RequestExecutionLevel user

;--------------------------------
;Variables

  Var StartMenuFolder

;--------------------------------
;Interface Settings

  !define MUI_WELCOMEFINISHPAGE_BITMAP ${CHECKOUTPATH}\build\resources\installer.bmp
  !define MUI_ICON ${CHECKOUTPATH}\build\resources\ps.ico
  !define MUI_ABORTWARNING
  !define MUI_LANGDLL_ALLLANGUAGES

;--------------------------------
;Language Selection Dialog Settings

  ;Remember the installer language
  !define MUI_LANGDLL_REGISTRY_ROOT "HKCU" 
  !define MUI_LANGDLL_REGISTRY_KEY "Software\0 A.D." 
  !define MUI_LANGDLL_REGISTRY_VALUENAME "Installer Language"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !if INCLUDE_SOURCE
    !insertmacro MUI_PAGE_COMPONENTS
  !endif
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\0 A.D."
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "0 A.D. alpha"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

  !define MUI_FINISHPAGE_RUN $INSTDIR\binaries\system\pyrogenesis.exe
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages
; Keep in sync with remove-incomplete-translations.sh
 
  !insertmacro MUI_LANGUAGE "English" ;first language is the default language
  !insertmacro MUI_LANGUAGE "Asturian"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "ScotsGaelic"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Ukrainian"

;--------------------------------
;Installer Sections

Section "!Game and data files" GameSection

  SetOutPath "$INSTDIR"
  File "${CHECKOUTPATH}\*.txt"
  File "${CHECKOUTPATH}\source\tools\openlogsfolder\*.*"
  !if INCLUDE_SOURCE
    File /r /x "public" /x "mod" /x "dev.cfg" "${CHECKOUTPATH}\binaries"
  !else
    ;Exclude debug DLLs and related files
    File /r /x "public" /x "mod" /x "dev.cfg" /x "*_d.dll" /x "enetd.dll" /x "FColladaD.dll" /x "gloox-1.0d.dll" /x "glooxwrapper_dbg.*" /x "libcurld.dll" /x "libpng16d.dll" /x "libsodiumd.dll" /x "miniupnpcd.dll" /x "mozjs*-ps-debug*" /x "mozjs*vc140.*" /x "msvc*d.dll" /x "zlib1d.dll" "${CHECKOUTPATH}\binaries"
  !endif

  SetOutPath "$INSTDIR\binaries\data\mods\public"
  File "${CHECKOUTPATH}\binaries\data\mods\public\public.zip"
  File "${CHECKOUTPATH}\binaries\data\mods\public\mod.json"
  SetOutPath "$INSTDIR\binaries\data\mods\mod"
  File "${CHECKOUTPATH}\binaries\data\mods\mod\mod.zip"

  ;Store installation folder
  WriteRegStr SHCTX "Software\0 A.D." "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ;Add uninstall information
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "DisplayName" "0 A.D."
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "DisplayVersion" "r${REVISION}-alpha"
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "VersionMajor" 0
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "VersionMinor" ${REVISION}
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "Publisher" "Wildfire Games"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "DisplayIcon" "$\"$INSTDIR\binaries\system\pyrogenesis.exe$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "InstallLocation" "$\"$INSTDIR$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
  WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "URLInfoAbout" "http://play0ad.com"
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "NoModify" 1
  WriteRegDWORD SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "NoRepair" 1

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  SetOutPath "$INSTDIR\binaries\system" ;Set working directory of shortcuts
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\0 A.D..lnk" "$INSTDIR\binaries\system\pyrogenesis.exe" ""
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Map editor.lnk" "$INSTDIR\binaries\system\pyrogenesis.exe" "-editor" "$INSTDIR\binaries\data\tools\atlas\icons\ScenarioEditor.ico"
  SetOutPath "$INSTDIR"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Open logs folder.lnk" "$INSTDIR\OpenLogsFolder.bat"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  WriteINIStr "$SMPROGRAMS\$StartMenuFolder\Web site.url" "InternetShortcut" "URL" "http://play0ad.com/"

  !insertmacro MUI_STARTMENU_WRITE_END

  ;Register .pyromod file association
  ${registerExtension} "$INSTDIR\binaries\system\pyrogenesis.exe" ".pyromod" "Pyrogenesis mod"

SectionEnd

!if INCLUDE_SOURCE
Section /o "Source code" SourceSection

  SetOutPath "$INSTDIR"
  File /r ${CHECKOUTPATH}\source
  File /r ${CHECKOUTPATH}\docs
  File /r ${CHECKOUTPATH}\build
  File /r ${CHECKOUTPATH}\libraries

SectionEnd
!endif

;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY

  ReadRegStr $R0 SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D." "UninstallString"
  StrCmp $R0 "" done
 
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "0 A.D. is already installed.$\n$\nClick $\"OK$\" to remove the previous version, or $\"Cancel$\" to stop this installation." \
  IDOK uninst
  Abort
 
;Run the uninstaller
uninst:
  ClearErrors
  ExecWait '$R0 _?=$INSTDIR' ;Do not copy the uninstaller to a temp file
 
done:

FunctionEnd

;--------------------------------
;Descriptions

  !if INCLUDE_SOURCE

    ;Assign descriptions to sections
    !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
      !insertmacro MUI_DESCRIPTION_TEXT ${GameSection} "0 A.D. game executable and data."
      !insertmacro MUI_DESCRIPTION_TEXT ${SourceSection} "Source code and build tools."
    !insertmacro MUI_FUNCTION_DESCRIPTION_END

  !endif

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  RMDir /r "$INSTDIR\binaries"
  !if INCLUDE_SOURCE
    RMDir /r "$INSTDIR\source"
    RMDir /r "$INSTDIR\docs"
    RMDir /r "$INSTDIR\build"
    RMDir /r "$INSTDIR\libraries"
  !endif
  Delete "$INSTDIR\*.txt"
  Delete "$INSTDIR\*.bat"
  Delete "$INSTDIR\OpenLogsFolder.vbs"
  Delete "$INSTDIR\Uninstall.exe"

  RMDir "$INSTDIR"

  RMDir /r "$LOCALAPPDATA\0ad\cache"
  RMDir /r "$LOCALAPPDATA\0ad\logs"
  ; leave the other directories (screenshots, config files, etc)

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

  Delete "$SMPROGRAMS\$StartMenuFolder\Open logs folder.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Map editor.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\0 A.D..lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Web site.url"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\0 A.D."
  DeleteRegKey /ifempty SHCTX "Software\0 A.D."

  ;Unregister .pyromod file association
  ${unregisterExtension} ".pyromod" "Pyrogenesis mod"

SectionEnd

;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd
