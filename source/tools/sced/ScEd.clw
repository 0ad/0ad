; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CScEdView
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ScEd.h"
LastPage=0

ClassCount=27
Class1=CScEdApp
Class2=CScEdDoc
Class3=CScEdView
Class4=CMainFrame

ResourceCount=27
Resource1=IDD_DIALOGBAR_UNITTOOLS
Class5=CAboutDlg
Resource2=IDD_ABOUTBOX
Class6=CLightSettingsDlg
Class7=CColorButton
Class8=CElevationButton
Class9=CDirectionButton
Class10=CWebLinkButton
Resource3=IDD_DIALOG_LIGHTSETTINGS
Class11=CTextureToolBar
Class12=CImageListCtrl
Class13=TexToolBar
Resource4=IDR_MAINFRAME
Class14=CModelToolBar
Resource5=IDD_DIALOGBAR_TEXTURETOOLS
Class15=COptionsDlg
Resource6=IDD_DIALOG_SIMPLEEDIT
Class16=CElevToolsDlgBar
Resource7=IDD_DIALOGBAR_UNITPROPERTIES
Class17=CUnitPropertiesDlg
Resource8=IDD_DIALOG_MAPSIZE
Class18=CSimpleEdit
Resource9=IDD_DIALOG_OPTIONS
Class19=CMapSizeDlg
Class20=CMainFrameDlgBar
Resource10=IDD_DIALOGBAR_ELEVATIONTOOLS
Resource11=IDD_DIALOGBAR_BRUSHSHAPEEDITOR
Resource12=IDD_PROPPAGE_NAVIGATION (English (U.S.))
Resource13=IDD_ABOUTBOX (English (U.S.))
Resource14=IDD_DIALOG_SIMPLEEDIT (English (U.S.))
Resource15=IDD_DIALOG_MAPSIZE (English (U.S.))
Resource16=IDD_DIALOG_LIGHTSETTINGS (English (U.S.))
Resource17=IDD_DIALOGBAR_ELEVATIONTOOLS (English (U.S.))
Resource18=IDD_PROPPAGE_SHADOWS (English (U.S.))
Resource19=IDD_DIALOGBAR_UNITPROPERTIES (English (U.S.))
Resource20=IDD_DIALOGBAR_BRUSHSHAPEEDITOR (English (U.S.))
Resource21=IDD_DIALOGBAR_TEXTURETOOLS (English (U.S.))
Resource22=IDD_UNITPROPERTIES_TEXTURES (English (U.S.))
Resource23=IDD_PROPPAGE_MEDIUM (English (U.S.))
Resource24=IDD_DIALOGBAR_UNITTOOLS (English (U.S.))
Class21=COptionsPropSheet
Class22=CNavigationPropPage
Class23=CShadowsPropPage
Class24=CNavigatePropPage
Resource25=IDD_DIALOG_OPTIONS (English (U.S.))
Resource26=IDD_UNITPROPERTIES_ANIMATIONS (English (U.S.))
Class25=CUnitPropertiesTabCtrl
Class26=CUnitPropertiesTexturesTab
Class27=CUnitPropertiesAnimationsTab
Resource27=IDR_MAINFRAME (English (U.S.))

[CLS:CScEdApp]
Type=0
HeaderFile=ScEd.h
ImplementationFile=ScEd.cpp
Filter=N
LastObject=CScEdApp
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CScEdDoc]
Type=0
HeaderFile=ScEdDoc.h
ImplementationFile=ScEdDoc.cpp
Filter=N

[CLS:CScEdView]
Type=0
HeaderFile=ScEdView.h
ImplementationFile=ScEdView.cpp
Filter=C
BaseClass=CView
VirtualFilter=VWC
LastObject=CScEdView


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
BaseClass=CFrameWnd
VirtualFilter=fWC
LastObject=CMainFrame




[CLS:CAboutDlg]
Type=0
HeaderFile=ScEd.cpp
ImplementationFile=ScEd.cpp
Filter=D
LastObject=IDC_BUTTON_LAUNCHWFG
BaseClass=CDialog
VirtualFilter=dWC

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177294
Control2=IDC_STATIC_VERSION,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_BUTTON_LAUNCHWFG,button,1342275595

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_LOADMAP
Command2=ID_FILE_SAVEMAP
Command3=ID_APP_EXIT
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_REDO
Command6=ID_VIEW_TERRAIN_SOLID
Command7=ID_VIEW_TERRAIN_GRID
Command8=ID_VIEW_TERRAIN_WIREFRAME
Command9=ID_VIEW_MODEL_SOLID
Command10=ID_VIEW_MODEL_GRID
Command11=ID_VIEW_MODEL_WIREFRAME
Command12=ID_VIEW_RENDERSTATS
Command13=ID_VIEW_SCREENSHOT
Command14=IDR_TEXTURE_TOOLS
Command15=IDR_ELEVATION_TOOLS
Command16=IDR_UNIT_TOOLS
Command17=ID_LIGHTING_SETTINGS
Command18=ID_TERRAIN_LOAD
Command19=IDR_RESIZE_MAP
Command20=IDR_TOOLS_CONVERTSMD
Command21=ID_TOOLS_OPTIONS
Command22=ID_APP_ABOUT
CommandCount=22

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_FILE_NEW
Command3=ID_FILE_OPEN
Command4=ID_FILE_SAVE
Command5=ID_EDIT_PASTE
Command6=ID_EDIT_UNDO
Command7=ID_EDIT_CUT
Command8=ID_VIEW_RENDERSTATS
Command9=ID_NEXT_PANE
Command10=ID_PREV_PANE
Command11=ID_VIEW_SCREENSHOT
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_EDIT_CUT
Command15=ID_EDIT_REDO
Command16=ID_EDIT_UNDO
CommandCount=16

[TB:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
CommandCount=8

[DLG:IDR_MAINFRAME]
Type=1
Class=CMainFrameDlgBar
ControlCount=4
Control1=IDC_BUTTON_SELECT,button,1342242944
Control2=IDC_BUTTON_ELEVATIONTOOLS,button,1342242944
Control3=IDC_BUTTON_TEXTURETOOLS,button,1342242944
Control4=IDC_BUTTON_MODELTOOLS,button,1342177408

[DLG:IDD_DIALOG_LIGHTSETTINGS]
Type=1
Class=CLightSettingsDlg
ControlCount=19
Control1=IDOK,button,1342242817
Control2=IDC_BUTTON_APPLY,button,1342242816
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_BUTTON_SUNCOLOR,button,1342210059
Control8=IDC_BUTTON_TERRAINAMBIENTCOLOR,button,1342210059
Control9=IDC_STATIC,button,1342177287
Control10=IDC_STATIC,button,1342177287
Control11=IDC_STATIC,static,1342308352
Control12=IDC_BUTTON_DIRECTION,button,1342210059
Control13=IDC_BUTTON_ELEVATION,button,1342210059
Control14=IDC_EDIT_DIRECTION,edit,1350574208
Control15=IDC_SPIN_DIRECTION,msctls_updown32,1342177463
Control16=IDC_STATIC,static,1342308352
Control17=IDC_BUTTON_UNITSAMBIENTCOLOR,button,1342210059
Control18=IDC_EDIT_ELEVATION,edit,1350574208
Control19=IDC_SPIN_ELEVATION,msctls_updown32,1342177462

[CLS:CLightSettingsDlg]
Type=0
HeaderFile=LightSettingsDlg.h
ImplementationFile=LightSettingsDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CLightSettingsDlg
VirtualFilter=dWC

[CLS:CColorButton]
Type=0
HeaderFile=ColorButton.h
ImplementationFile=ColorButton.cpp
BaseClass=CButton
Filter=W
LastObject=CColorButton
VirtualFilter=BWC

[CLS:CElevationButton]
Type=0
HeaderFile=ElevationButton.h
ImplementationFile=ElevationButton.cpp
BaseClass=CButton
Filter=W
VirtualFilter=BWC
LastObject=CElevationButton

[CLS:CDirectionButton]
Type=0
HeaderFile=DirectionButton.h
ImplementationFile=DirectionButton.cpp
BaseClass=CButton
Filter=W
LastObject=CDirectionButton
VirtualFilter=BWC

[CLS:CWebLinkButton]
Type=0
HeaderFile=WebLinkButton.h
ImplementationFile=WebLinkButton.cpp
BaseClass=CButton
Filter=W
VirtualFilter=BWC
LastObject=CWebLinkButton

[DLG:IDD_DIALOGBAR_TEXTURETOOLS]
Type=1
Class=CTextureToolBar
ControlCount=6
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_STATIC,static,1342308352
Control3=IDC_LIST_TEXTUREBROWSER,SysListView32,1342291980
Control4=IDC_STATIC_CURRENTTEXTURE,static,1350566414
Control5=IDC_COMBO_TERRAINTYPES,combobox,1344339970
Control6=IDC_STATIC,button,1342177287

[CLS:CTextureToolBar]
Type=0
HeaderFile=TextureToolBar.h
ImplementationFile=TextureToolBar.cpp
BaseClass=CDialog
Filter=D
LastObject=CTextureToolBar
VirtualFilter=dWC

[CLS:CImageListCtrl]
Type=0
HeaderFile=ImageListCtrl.h
ImplementationFile=ImageListCtrl.cpp
BaseClass=CListCtrl
Filter=W
LastObject=CImageListCtrl
VirtualFilter=FWC

[CLS:TexToolBar]
Type=0
HeaderFile=TexToolBar.h
ImplementationFile=TexToolBar.cpp
BaseClass=CToolBarCtrl
Filter=W
LastObject=TexToolBar
VirtualFilter=YWC

[CLS:CModelToolBar]
Type=0
HeaderFile=ModelToolBar.h
ImplementationFile=ModelToolBar.cpp
BaseClass=CDialog
Filter=D
LastObject=CModelToolBar
VirtualFilter=dWC

[DLG:IDD_DIALOG_OPTIONS]
Type=1
Class=COptionsDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,button,1342177287
Control4=IDC_SLIDER_RTSSCROLLSPEED,msctls_trackbar32,1342242821
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352

[CLS:COptionsDlg]
Type=0
HeaderFile=OptionsDlg.h
ImplementationFile=OptionsDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=COptionsDlg
VirtualFilter=dWC

[DLG:IDD_DIALOGBAR_ELEVATIONTOOLS]
Type=1
Class=CElevToolsDlgBar
ControlCount=7
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_STATIC,static,1342308352
Control3=IDC_SLIDER_BRUSHEFFECT,msctls_trackbar32,1342242821
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,button,1342177287
Control6=IDC_RADIO_RAISE,button,1342177289
Control7=IDC_RADIO_SMOOTH,button,1342177289

[CLS:CElevToolsDlgBar]
Type=0
HeaderFile=ElevToolsDlgBar.h
ImplementationFile=ElevToolsDlgBar.cpp
BaseClass=CDialog
Filter=D
LastObject=CElevToolsDlgBar

[DLG:IDD_DIALOGBAR_UNITTOOLS]
Type=1
Class=CModelToolBar
ControlCount=4
Control1=IDC_LIST_OBJECTBROWSER,SysListView32,1342291981
Control2=IDC_BUTTON_ADD,button,1342242816
Control3=IDC_BUTTON_EDIT,button,1342242816
Control4=IDC_COMBO_OBJECTTYPES,combobox,1344339970

[DLG:IDD_DIALOGBAR_UNITPROPERTIES]
Type=1
Class=CUnitPropertiesDlg
ControlCount=13
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_NAME,edit,1350631552
Control5=IDC_EDIT_MODEL,edit,1350631552
Control6=IDC_EDIT_TEXTURE,edit,1350631552
Control7=IDC_BUTTON_MODELBROWSE,button,1342242816
Control8=IDC_BUTTON_TEXTUREBROWSE,button,1342242816
Control9=IDC_BUTTON_REFRESH,button,1342242816
Control10=IDC_BUTTON_BACK,button,1342242816
Control11=IDC_STATIC,static,1342308352
Control12=IDC_EDIT_ANIMATION,edit,1350631552
Control13=IDC_BUTTON_ANIMATIONBROWSE,button,1342242816

[DLG:IDD_DIALOG_SIMPLEEDIT]
Type=1
Class=CSimpleEdit
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_EDIT1,edit,1350631552

[CLS:CSimpleEdit]
Type=0
HeaderFile=SimpleEdit.h
ImplementationFile=SimpleEdit.cpp
BaseClass=CDialog
Filter=D
LastObject=CSimpleEdit
VirtualFilter=dWC

[DLG:IDD_DIALOG_MAPSIZE]
Type=1
Class=CMapSizeDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_RADIO_SMALL,button,1342177289
Control4=IDC_RADIO_MEDIUM,button,1342177289
Control5=IDC_RADIO_LARGE,button,1342177289
Control6=IDC_RADIO_HUGE,button,1342177289

[CLS:CMapSizeDlg]
Type=0
HeaderFile=MapSizeDlg.h
ImplementationFile=MapSizeDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CMapSizeDlg

[CLS:CMainFrameDlgBar]
Type=0
HeaderFile=MainFrameDlgBar.h
ImplementationFile=MainFrameDlgBar.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_BUTTON_ELEVATIONTOOLS

[DLG:IDD_DIALOGBAR_BRUSHSHAPEEDITOR]
Type=1
Class=CShadowsPropPage
ControlCount=4
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_COMBO_TERRAINTYPES,combobox,1344339970
Control3=IDC_STATIC,static,1342308352
Control4=IDC_BUTTON_BACK,button,1342242816

[TB:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_CUT
Command5=ID_EDIT_COPY
Command6=ID_EDIT_PASTE
Command7=ID_FILE_PRINT
Command8=ID_APP_ABOUT
CommandCount=8

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=CMainFrame
Command1=ID_FILE_LOADMAP
Command2=ID_FILE_SAVEMAP
Command3=ID_APP_EXIT
Command4=ID_TEST_GO
Command5=ID_TEST_STOP
Command6=ID_EDIT_UNDO
Command7=ID_EDIT_REDO
Command8=ID_VIEW_TERRAIN_SOLID
Command9=ID_VIEW_TERRAIN_GRID
Command10=ID_VIEW_TERRAIN_WIREFRAME
Command11=ID_VIEW_MODEL_SOLID
Command12=ID_VIEW_MODEL_GRID
Command13=ID_VIEW_MODEL_WIREFRAME
Command14=ID_VIEW_RENDERSTATS
Command15=ID_VIEW_SCREENSHOT
Command16=IDR_TEXTURE_TOOLS
Command17=IDR_ELEVATION_TOOLS
Command18=IDR_UNIT_TOOLS
Command19=ID_LIGHTING_SETTINGS
Command20=ID_TERRAIN_LOAD
Command21=IDR_RESIZE_MAP
Command22=ID_TOOLS_OPTIONS
Command23=ID_APP_ABOUT
CommandCount=23

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_FILE_NEW
Command3=ID_FILE_OPEN
Command4=ID_FILE_SAVE
Command5=ID_EDIT_PASTE
Command6=ID_EDIT_UNDO
Command7=ID_EDIT_CUT
Command8=ID_VIEW_RENDERSTATS
Command9=ID_NEXT_PANE
Command10=ID_PREV_PANE
Command11=ID_VIEW_SCREENSHOT
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_EDIT_CUT
Command15=ID_EDIT_REDO
Command16=ID_EDIT_UNDO
CommandCount=16

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=5
Control1=IDC_STATIC,static,1342177294
Control2=IDC_STATIC_VERSION,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889
Control5=IDC_BUTTON_LAUNCHWFG,button,1342275595

[DLG:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=CMainFrameDlgBar
ControlCount=4
Control1=IDC_BUTTON_SELECT,button,1342242944
Control2=IDC_BUTTON_ELEVATIONTOOLS,button,1342242944
Control3=IDC_BUTTON_TEXTURETOOLS,button,1342242944
Control4=IDC_BUTTON_MODELTOOLS,button,1342177408

[DLG:IDD_DIALOG_LIGHTSETTINGS (English (U.S.))]
Type=1
Class=CLightSettingsDlg
ControlCount=19
Control1=IDOK,button,1342242817
Control2=IDC_BUTTON_APPLY,button,1342242816
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_BUTTON_SUNCOLOR,button,1342210059
Control8=IDC_BUTTON_TERRAINAMBIENTCOLOR,button,1342210059
Control9=IDC_STATIC,button,1342177287
Control10=IDC_STATIC,button,1342177287
Control11=IDC_STATIC,static,1342308352
Control12=IDC_BUTTON_DIRECTION,button,1342210059
Control13=IDC_BUTTON_ELEVATION,button,1342210059
Control14=IDC_EDIT_DIRECTION,edit,1350574208
Control15=IDC_SPIN_DIRECTION,msctls_updown32,1342177463
Control16=IDC_STATIC,static,1342308352
Control17=IDC_BUTTON_UNITSAMBIENTCOLOR,button,1342210059
Control18=IDC_EDIT_ELEVATION,edit,1350574208
Control19=IDC_SPIN_ELEVATION,msctls_updown32,1342177462

[DLG:IDD_DIALOGBAR_TEXTURETOOLS (English (U.S.))]
Type=1
Class=CTextureToolBar
ControlCount=6
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_STATIC,static,1342308352
Control3=IDC_LIST_TEXTUREBROWSER,SysListView32,1342291980
Control4=IDC_STATIC_CURRENTTEXTURE,static,1350566414
Control5=IDC_COMBO_TERRAINTYPES,combobox,1344339970
Control6=IDC_STATIC,button,1342177287

[DLG:IDD_DIALOGBAR_UNITTOOLS (English (U.S.))]
Type=1
Class=CModelToolBar
ControlCount=6
Control1=IDC_LIST_OBJECTBROWSER,SysListView32,1342291981
Control2=IDC_BUTTON_ADD,button,1342242816
Control3=IDC_BUTTON_EDIT,button,1342242816
Control4=IDC_COMBO_OBJECTTYPES,combobox,1344339970
Control5=IDC_BUTTON_SELECT,button,1342242944
Control6=IDC_BUTTON_ADDUNIT,button,1342242944

[DLG:IDD_DIALOG_OPTIONS (English (U.S.))]
Type=1
Class=COptionsDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,button,1342177287
Control4=IDC_SLIDER_RTSSCROLLSPEED,msctls_trackbar32,1342242821
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352

[DLG:IDD_DIALOGBAR_ELEVATIONTOOLS (English (U.S.))]
Type=1
Class=CElevToolsDlgBar
ControlCount=7
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_STATIC,static,1342308352
Control3=IDC_SLIDER_BRUSHEFFECT,msctls_trackbar32,1342242821
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,button,1342177287
Control6=IDC_RADIO_RAISE,button,1342177289
Control7=IDC_RADIO_SMOOTH,button,1342177289

[DLG:IDD_DIALOGBAR_UNITPROPERTIES (English (U.S.))]
Type=1
Class=CUnitPropertiesDlg
ControlCount=13
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342308352
Control3=IDC_EDIT_NAME,edit,1350631552
Control4=IDC_EDIT_MODEL,edit,1350631552
Control5=IDC_BUTTON_MODELBROWSE,button,1342242816
Control6=IDC_BUTTON_REFRESH,button,1342242816
Control7=IDC_BUTTON_BACK,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_EDIT_ANIMATION,edit,1350631552
Control10=IDC_BUTTON_ANIMATIONBROWSE,button,1342242816
Control11=IDC_STATIC,static,1342308352
Control12=IDC_EDIT_TEXTURE,edit,1350631552
Control13=IDC_BUTTON_TEXTUREBROWSE,button,1342242816

[DLG:IDD_DIALOG_SIMPLEEDIT (English (U.S.))]
Type=1
Class=CSimpleEdit
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_EDIT1,edit,1350631552

[DLG:IDD_DIALOG_MAPSIZE (English (U.S.))]
Type=1
Class=CMapSizeDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_RADIO_SMALL,button,1342177289
Control4=IDC_RADIO_MEDIUM,button,1342177289
Control5=IDC_RADIO_LARGE,button,1342177289
Control6=IDC_RADIO_HUGE,button,1342177289

[DLG:IDD_DIALOGBAR_BRUSHSHAPEEDITOR (English (U.S.))]
Type=1
Class=CShadowsPropPage
ControlCount=4
Control1=IDC_SLIDER_BRUSHSIZE,msctls_trackbar32,1342242821
Control2=IDC_COMBO_TERRAINTYPES,combobox,1344339970
Control3=IDC_STATIC,static,1342308352
Control4=IDC_BUTTON_BACK,button,1342242816

[DLG:IDD_PROPPAGE_MEDIUM (English (U.S.))]
Type=1
Class=?
ControlCount=1
Control1=IDC_STATIC,static,1342308352

[DLG:IDD_PROPPAGE_NAVIGATION (English (U.S.))]
Type=1
Class=CNavigatePropPage
ControlCount=2
Control1=IDC_STATIC,button,1342177287
Control2=IDC_SLIDER_RTSSCROLLSPEED,msctls_trackbar32,1342242821

[DLG:IDD_PROPPAGE_SHADOWS (English (U.S.))]
Type=1
Class=?
ControlCount=6
Control1=IDC_CHECK_SHADOWS,button,1342242819
Control2=IDC_STATIC,button,1342177287
Control3=IDC_STATIC,static,1342308352
Control4=IDC_BUTTON_SHADOWCOLOR,button,1342210059
Control5=IDC_SLIDER_SHADOWQUALITY,msctls_trackbar32,1342242821
Control6=IDC_STATIC,static,1342308352

[CLS:COptionsPropSheet]
Type=0
HeaderFile=OptionsPropSheet.h
ImplementationFile=OptionsPropSheet.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=COptionsPropSheet
VirtualFilter=hWC

[CLS:CNavigationPropPage]
Type=0
HeaderFile=NavigationPropPage.h
ImplementationFile=NavigationPropPage.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=CNavigationPropPage

[CLS:CShadowsPropPage]
Type=0
HeaderFile=ShadowsPropPage.h
ImplementationFile=ShadowsPropPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CShadowsPropPage
VirtualFilter=idWC

[CLS:CNavigatePropPage]
Type=0
HeaderFile=NavigatePropPage.h
ImplementationFile=NavigatePropPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CNavigatePropPage

[DLG:IDD_UNITPROPERTIES_TEXTURES (English (U.S.))]
Type=1
Class=CUnitPropertiesTexturesTab
ControlCount=3
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_TEXTURE,edit,1350631552
Control3=IDC_BUTTON_TEXTUREBROWSE,button,1342242816

[DLG:IDD_UNITPROPERTIES_ANIMATIONS (English (U.S.))]
Type=1
Class=CUnitPropertiesAnimationsTab
ControlCount=3
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_ANIMATION,edit,1350631552
Control3=IDC_BUTTON_ANIMATIONBROWSE,button,1342242816

[CLS:CUnitPropertiesTabCtrl]
Type=0
HeaderFile=UnitPropertiesTabCtrl.h
ImplementationFile=UnitPropertiesTabCtrl.cpp
BaseClass=CTabCtrl
Filter=W
LastObject=CUnitPropertiesTabCtrl
VirtualFilter=UWC

[CLS:CUnitPropertiesTexturesTab]
Type=0
HeaderFile=UnitPropertiesTexturesTab.h
ImplementationFile=UnitPropertiesTexturesTab.cpp
BaseClass=CDialog
Filter=D

[CLS:CUnitPropertiesAnimationsTab]
Type=0
HeaderFile=UnitPropertiesAnimationsTab.h
ImplementationFile=UnitPropertiesAnimationsTab.cpp
BaseClass=CDialog
Filter=D

