// MainFrm.cpp : implementation of the CMainFrame class
//

#include "precompiled.h"
#include "stdafx.h"

#define _IGNORE_WGL_H_
#include "ogl.h"
#include "res/tex.h"
#include "res/mem.h"
#include "res/vfs.h"
#undef _IGNORE_WGL_H_

#include "ScEd.h"
#include "ScEdView.h"
#include "MiniMap.h"
#include "MapReader.h"
#include "MapWriter.h"

#include "UserConfig.h"

#include "Unit.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "TextureManager.h"
#include "ModelDef.h"
#include "UIGlobals.h"
#include "MainFrm.h"
#include "OptionsPropSheet.h"
#include "LightSettingsDlg.h"
#include "MapSizeDlg.h"
#include "EditorData.h"
#include "ToolManager.h"
#include "CommandManager.h"

#include "AlterLightEnvCommand.h"

#include "LightEnv.h"
#include "Terrain.h"

#include "PaintTextureTool.h"
#include "PaintObjectTool.h"
#include "RaiseElevationTool.h"
#include "BrushShapeEditorTool.h"

#include "SelectObjectTool.h"
#include "simulation/Entity.h"

extern CTerrain g_Terrain;
extern CLightEnv g_LightEnv;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame


IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_TERRAIN_LOAD, OnTerrainLoad)
	ON_COMMAND(ID_LIGHTING_SETTINGS, OnLightingSettings)
	ON_COMMAND(ID_VIEW_SCREENSHOT, OnViewScreenshot)
	ON_COMMAND(IDR_TEXTURE_TOOLS, OnTextureTools)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(IDR_ELEVATION_TOOLS, OnElevationTools)
	ON_COMMAND(IDR_RESIZE_MAP, OnResizeMap)
	ON_COMMAND(ID_VIEW_TERRAIN_GRID, OnViewTerrainGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TERRAIN_GRID, OnUpdateViewTerrainGrid)
	ON_COMMAND(ID_VIEW_TERRAIN_SOLID, OnViewTerrainSolid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TERRAIN_SOLID, OnUpdateViewTerrainSolid)
	ON_COMMAND(ID_VIEW_TERRAIN_WIREFRAME, OnViewTerrainWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TERRAIN_WIREFRAME, OnUpdateViewTerrainWireframe)
	ON_COMMAND(ID_VIEW_MODEL_GRID, OnViewModelGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODEL_GRID, OnUpdateViewModelGrid)
	ON_COMMAND(ID_VIEW_MODEL_SOLID, OnViewModelSolid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODEL_SOLID, OnUpdateViewModelSolid)
	ON_COMMAND(ID_VIEW_MODEL_WIREFRAME, OnViewModelWireframe)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODEL_WIREFRAME, OnUpdateViewModelWireframe)
	ON_COMMAND(ID_FILE_SAVEMAP, OnFileSaveMap)
	ON_COMMAND(ID_FILE_LOADMAP, OnFileLoadMap)
	ON_COMMAND(ID_VIEW_RENDERSTATS, OnViewRenderStats)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RENDERSTATS, OnUpdateViewRenderStats)
	ON_MESSAGE(WM_MOUSEWHEEL,OnMouseWheel)
	ON_COMMAND(ID_TEST_GO, OnTestGo)
	ON_UPDATE_COMMAND_UI(ID_TEST_GO, OnUpdateTestGo)
	ON_COMMAND(ID_TEST_STOP, OnTestStop)
	ON_UPDATE_COMMAND_UI(ID_TEST_STOP, OnUpdateTestStop)
	ON_COMMAND(IDR_UNIT_TOOLS, OnUnitTools)
	ON_COMMAND(ID_RANDOM_MAP, OnRandomMap)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_PLAYER_PLAYER0, OnEntityPlayer0)
	ON_COMMAND(ID_PLAYER_PLAYER1, OnEntityPlayer1)
	ON_COMMAND(ID_PLAYER_PLAYER2, OnEntityPlayer2)
	ON_COMMAND(ID_PLAYER_PLAYER3, OnEntityPlayer3)
	ON_COMMAND(ID_PLAYER_PLAYER4, OnEntityPlayer4)
	ON_COMMAND(ID_PLAYER_PLAYER5, OnEntityPlayer5)
	ON_COMMAND(ID_PLAYER_PLAYER6, OnEntityPlayer6)
	ON_COMMAND(ID_PLAYER_PLAYER7, OnEntityPlayer7)
	ON_COMMAND(ID_PLAYER_PLAYER8, OnEntityPlayer8)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::PostNcDestroy()
{
	CFrameWnd::PostNcDestroy();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	
	if (!m_wndDlgBar.Create(this, IDR_MAINFRAME, 
		CBRS_ALIGN_TOP, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndDlgBar.SetWindowText("Toolbar");

	// create status bar
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}


	// create texture tools bar
	if (!m_wndTexToolsBar.Create(this, IDD_DIALOGBAR_TEXTURETOOLS, 
		CBRS_ALIGN_LEFT | CBRS_GRIPPER, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndTexToolsBar.SetWindowText("TexTools");

	// create elevation tools bar
	if (!m_wndElevToolsBar.Create(this, IDD_DIALOGBAR_ELEVATIONTOOLS, 
		CBRS_ALIGN_LEFT | CBRS_GRIPPER, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndElevToolsBar.SetWindowText("ElevTools");

	// create unit tools bar
	if (!m_wndUnitToolsBar.Create(this, IDD_DIALOGBAR_UNITTOOLS, 
		CBRS_ALIGN_LEFT | CBRS_GRIPPER, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndUnitToolsBar.SetWindowText("UnitTools");

	// create unit properties bar
	if (!m_wndUnitPropsBar.Create(this, IDD_DIALOGBAR_UNITPROPERTIES, 
		CBRS_ALIGN_LEFT | CBRS_GRIPPER, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndUnitPropsBar.SetWindowText("UnitProperties");

	// create brush shape editor bar
	if (!m_wndBrushShapeEditorBar.Create(this, IDD_DIALOGBAR_BRUSHSHAPEEDITOR, 
		CBRS_ALIGN_LEFT | CBRS_GRIPPER, AFX_IDW_DIALOGBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndTexToolsBar.SetWindowText("BrushEditor");

	// enable docking on main frame
	EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT | CBRS_ALIGN_BOTTOM);
	
/*
	// initially dock everything
	m_wndTexToolsBar.EnableDocking(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);	
	DockControlBar(&m_wndTexToolsBar);
	m_wndElevToolsBar.EnableDocking(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);	
	DockControlBar(&m_wndElevToolsBar);
	m_wndUnitToolsBar.EnableDocking(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);	
	DockControlBar(&m_wndUnitToolsBar);
	m_wndUnitPropsBar.EnableDocking(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);	
	DockControlBar(&m_wndUnitPropsBar);
*/
	// and start up with all tools deselected
	DeselectTools();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


float rnd1()
{
	return float(rand())/float(RAND_MAX);
}


void CMainFrame::OnTerrainLoad() 
{
	const char* filter="Targa Files|*.tga|RAW files|*.raw||";
	CFileDialog dlg(TRUE,"tga",0,OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,filter,0);
	dlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_TERRAINLOADDIR);

	if (dlg.DoModal()==IDOK) {
		const char* filename=dlg.m_ofn.lpstrFile;

		CStr dir(filename);
		dir=dir.Left(dlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_TERRAINLOADDIR,(const char*) dir);

		g_EditorData.LoadTerrain(filename);
	}
}



void CMainFrame::OnLightingSettings() 
{
	CLightSettingsDlg dlg;
	RGBColorToColorRef(g_LightEnv.m_SunColor,dlg.m_SunColor.m_Color);
	RGBColorToColorRef(g_LightEnv.m_TerrainAmbientColor,dlg.m_TerrainAmbientColor.m_Color);
	RGBColorToColorRef(g_LightEnv.m_UnitsAmbientColor,dlg.m_UnitsAmbientColor.m_Color);
	dlg.m_Elevation=int(RADTODEG(g_LightEnv.m_Elevation)+0.5f);
	dlg.m_Direction=int(RADTODEG(g_LightEnv.m_Rotation)+0.5f);

	if (dlg.DoModal()==IDOK) {
		// have we previously applied a lightenv?
		if (dlg.m_PreviousPreview) {
			// yes - undo it
			g_CmdMan.Undo();
		} 

		// build a lighting environment from the parameters
		CLightEnv env;
		env.m_Elevation=DEGTORAD(dlg.m_ElevationButton.m_Elevation);
		env.m_Rotation=DEGTORAD(dlg.m_DirectionButton.m_Direction);
		ColorRefToRGBColor(dlg.m_SunColor.m_Color,env.m_SunColor);
		ColorRefToRGBColor(dlg.m_TerrainAmbientColor.m_Color,env.m_TerrainAmbientColor);
		ColorRefToRGBColor(dlg.m_TerrainAmbientColor.m_Color,env.m_UnitsAmbientColor);

		// create and execute an AlterLightEnv command
		CAlterLightEnvCommand* cmd=new CAlterLightEnvCommand(env);
		g_CmdMan.Execute(cmd);
	} else {
		if (dlg.m_PreviousPreview) {
			// undo the change
			g_CmdMan.Undo();
		}
	}

	AfxGetMainWnd()->Invalidate();
	AfxGetMainWnd()->UpdateWindow();
}

void CMainFrame::OnViewScreenshot() 
{
	CScEdView* view=(CScEdView*) GetActiveView();
	view->OnScreenShot();
}



void CMainFrame::OnTextureTools() 
{
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);

	// swizzle around control bar visibility
	DisableCtrlBars();
	ShowControlBar(&m_wndTexToolsBar,TRUE,FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_TEXTURETOOLS))->SetState(TRUE);

	// set active tool
	g_ToolMan.SetActiveTool(CPaintTextureTool::GetTool());
}

void CMainFrame::OnElevationTools() 
{
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);

	// swizzle around control bar visibility
	DisableCtrlBars();
	ShowControlBar(&m_wndElevToolsBar,TRUE,FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_ELEVATIONTOOLS))->SetState(TRUE);
	
	// notify window being shown so controls for correct mode (raise/smooth) are drawn,
	// and correct tool is setup
	m_wndElevToolsBar.OnShow();
}

void CMainFrame::OnUnitTools() 
{
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);

	// swizzle around control bar visibility
	DisableCtrlBars();
	ShowControlBar(&m_wndUnitToolsBar,TRUE,FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_MODELTOOLS))->SetState(TRUE);

	// set modeactive tool
	if (m_wndUnitToolsBar.m_Mode==CUnitToolsDlgBar::SELECT_MODE) {
		m_wndUnitToolsBar.SetSelectMode();
	} else {
		m_wndUnitToolsBar.SetAddUnitMode();
	}

	// ensure we're in the right editing mode
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);
}

void CMainFrame::DisableCtrlBars()
{
	ShowControlBar(&m_wndTexToolsBar,FALSE,FALSE);
	ShowControlBar(&m_wndElevToolsBar,FALSE,FALSE);
	ShowControlBar(&m_wndUnitToolsBar,FALSE,FALSE);
	ShowControlBar(&m_wndUnitPropsBar,FALSE,FALSE);
	ShowControlBar(&m_wndBrushShapeEditorBar,FALSE,FALSE);

	// switch off corresponding short cut buttons
	DisableToolbarButtons();
}

void CMainFrame::DisableToolbarButtons()
{
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_SELECT))->SetState(FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_TEXTURETOOLS))->SetState(FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_ELEVATIONTOOLS))->SetState(FALSE);
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_MODELTOOLS))->SetState(FALSE);
}

void CMainFrame::DeselectTools()
{
	// switch off all control bars
	DisableCtrlBars();
	((CButton*) m_wndDlgBar.GetDlgItem(IDC_BUTTON_SELECT))->SetState(TRUE);
	
	// deselect active tool
	g_ToolMan.SetActiveTool(0);

	// ensure we're in the right editing mode
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);
}

void CMainFrame::OnObjectProperties(CObjectEntry* obj) 
{
	// swizzle around control bar visibility
	DisableCtrlBars();
	ShowControlBar(&m_wndUnitPropsBar,TRUE,FALSE);

	m_wndUnitPropsBar.SetObject(obj);

	// set active tool
	g_ToolMan.SetActiveTool(0);

	// ensure we're in the right editing mode
	g_EditorData.SetMode(CEditorData::UNIT_EDIT);
}

void CMainFrame::OnToolsOptions() 
{
	COptionsPropSheet dlg("Options",this,0);
	dlg.m_NavigatePage.m_ScrollSpeed=g_UserCfg.GetOptionInt(CFG_SCROLLSPEED);
	
	dlg.m_ShadowsPage.m_EnableShadows=g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWS) ? TRUE : FALSE;

	COLORREF c;
	RGBAColorToColorRef(g_Renderer.GetOptionColor(CRenderer::OPT_SHADOWCOLOR),c);
	dlg.m_ShadowsPage.m_ShadowColor.m_Color=c;

	if (dlg.DoModal()==IDOK) {
		g_UserCfg.SetOptionInt(CFG_SCROLLSPEED,dlg.m_NavigatePage.m_ScrollSpeed);
		g_Renderer.SetOptionBool(CRenderer::OPT_SHADOWS,dlg.m_ShadowsPage.m_EnableShadows ? true : false);
		
		RGBAColor c;
		ColorRefToRGBAColor(dlg.m_ShadowsPage.m_ShadowColor.m_Color,0xff,c);
		g_Renderer.SetOptionColor(CRenderer::OPT_SHADOWCOLOR,c);
	}
}

void CMainFrame::OnEditUndo() 
{
	g_CmdMan.Undo();
}

void CMainFrame::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	const char* cmdName=g_CmdMan.GetUndoName();
	if (!cmdName) {
		const char* undoText="&Undo";
		pCmdUI->SetText(undoText);
		pCmdUI->Enable(FALSE);
	} else {
		const char* undoText="&Undo                                    Ctrl+Z";
		char buf[64];
		strcpy(buf,undoText);
		size_t len=strlen(cmdName);
		if (len>32) len=32;
		buf[6]='\"';
		strncpy(buf+7,cmdName,len);
		buf[6+len+1]='\"';
		pCmdUI->SetText(buf);
		pCmdUI->Enable(TRUE);
	}
}

void CMainFrame::OnEditRedo() 
{
	g_CmdMan.Redo();
}

void CMainFrame::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	const char* cmdName=g_CmdMan.GetRedoName();
	if (!cmdName) {
		const char* redoText="&Redo";
		pCmdUI->SetText(redoText);
		pCmdUI->Enable(FALSE);
	} else {
		const char* redoText="&Redo                                    Ctrl+Y";
		char buf[64];
		strcpy(buf,redoText);
		size_t len=strlen(cmdName);
		if (len>32) len=32;
		buf[6]='\"';
		strncpy(buf+7,cmdName,len);
		buf[6+len+1]='\"';
		pCmdUI->SetText(buf);
		pCmdUI->Enable(TRUE);
	}
}



void CMainFrame::OnResizeMap() 
{
	CMapSizeDlg dlg;
	if (dlg.DoModal()==IDOK) {
		// resize terrain to selected size
		g_Terrain.Resize(dlg.m_MapSize);	

		// reinitialise minimap to cope with terrain of different size
		g_MiniMap.Initialise();
	}
}

void CMainFrame::OnViewModelGrid() 
{
	g_Renderer.SetModelRenderMode(EDGED_FACES);
}

void CMainFrame::OnUpdateViewModelGrid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetModelRenderMode()==EDGED_FACES);
}

void CMainFrame::OnViewModelSolid() 
{
	g_Renderer.SetModelRenderMode(SOLID);
}

void CMainFrame::OnUpdateViewModelSolid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetModelRenderMode()==SOLID);	
}

void CMainFrame::OnViewModelWireframe() 
{
	g_Renderer.SetModelRenderMode(WIREFRAME);
}

void CMainFrame::OnUpdateViewModelWireframe(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetModelRenderMode()==WIREFRAME);
}

void CMainFrame::OnViewTerrainGrid() 
{
	g_Renderer.SetTerrainRenderMode(EDGED_FACES);
}

void CMainFrame::OnUpdateViewTerrainGrid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetTerrainRenderMode()==EDGED_FACES);
}

void CMainFrame::OnViewTerrainSolid() 
{
	g_Renderer.SetTerrainRenderMode(SOLID);
}

void CMainFrame::OnUpdateViewTerrainSolid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetTerrainRenderMode()==SOLID);	
}

void CMainFrame::OnViewTerrainWireframe() 
{
	g_Renderer.SetTerrainRenderMode(WIREFRAME);
}

void CMainFrame::OnUpdateViewTerrainWireframe(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_Renderer.GetTerrainRenderMode()==WIREFRAME);
}

void CMainFrame::OnFileSaveMap() 
{
	const char* filter="PMP Files|*.pmp||";
	CFileDialog savedlg(FALSE,"pmp","Untitled.pmp",OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR,filter,0);
	savedlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_MAPSAVEDIR);
	if (savedlg.DoModal()==IDOK) {
		const char* savename=savedlg.m_ofn.lpstrFile;

		CStr dir(savename);
		dir=dir.Left(savedlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_MAPSAVEDIR,(const char*) dir);

		CMapWriter writer;
		try {
			writer.SaveMap(savename, &g_Terrain, &g_LightEnv, &g_UnitMan);

			CStr filetitle=savedlg.m_ofn.lpstrFileTitle;
			int index=filetitle.ReverseFind(CStr("."));
			CStr doctitle=(index==-1) ? filetitle : filetitle.GetSubstring(0,index);
			g_EditorData.SetScenarioName((const char*) doctitle);
			SetTitle();
		} catch (CFilePacker::CFileOpenError) {
			char buf[256];
			sprintf(buf,"Failed to open \"%s\" for writing",savename);
			MessageBox(buf,"Error",MB_OK);
		} catch (CFilePacker::CFileWriteError) {
			char buf[256];
			sprintf(buf,"Error trying to write to \"%s\"",savename);
			MessageBox(buf,"Error",MB_OK);
#ifdef NDEBUG
		} catch (...) {
			char buf[256];
			sprintf(buf,"Error saving file \"%s\"",savename);
			MessageBox(buf,"Error",MB_OK);
#endif
		}
	}
}

void CMainFrame::OnFileLoadMap() 
{
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);

	const char* filter="PMP Files|*.pmp||";
	CFileDialog loaddlg(TRUE,"pmp",0,OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,filter,0);
	loaddlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_MAPLOADDIR);

	if (loaddlg.DoModal()==IDOK) {
		const char* loadname=loaddlg.m_ofn.lpstrFile;

		CStr dir(loadname);
		dir=dir.Left(loaddlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_MAPLOADDIR,(const char*) dir);
		
		CMapReader reader;
		try {
			reader.LoadMap(loadname, &g_Terrain, &g_UnitMan, &g_LightEnv);

			CStr filetitle=loaddlg.m_ofn.lpstrFileTitle;
			int index=filetitle.ReverseFind(CStr("."));
			CStr doctitle=(index==-1) ? filetitle : filetitle.GetSubstring(0,index);
			g_EditorData.SetScenarioName((const char*) doctitle);
			SetTitle();

			// reinitialise minimap 
			g_MiniMap.Initialise();
			// render everything force asset load
			g_EditorData.RenderNoCull();

			// start everything idling
			const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
			for (uint i=0;i<units.size();++i) {
				if (units[i]->GetObject()->m_IdleAnim) {
					units[i]->GetModel()->SetAnimation(units[i]->GetObject()->m_IdleAnim);
				}
			}
		} catch (CFileUnpacker::CFileOpenError) {
			char buf[256];
			sprintf(buf,"Failed to open \"%s\" for reading",loadname);
			MessageBox(buf,"Error",MB_OK);
		} catch (CFileUnpacker::CFileReadError) {
			char buf[256];
			sprintf(buf,"Error trying to read from \"%s\"",loadname);
			MessageBox(buf,"Error",MB_OK);
		} catch (CFileUnpacker::CFileEOFError) {
			char buf[256];
			sprintf(buf,"Premature end of file error reading from \"%s\"",loadname);
			MessageBox(buf,"Error",MB_OK);
		} catch (CFileUnpacker::CFileVersionError) {
			char buf[256];
			sprintf(buf,"Error reading from \"%s\" - version mismatch",loadname);
			MessageBox(buf,"Error",MB_OK);
		} catch (CFileUnpacker::CFileTypeError) {
			char buf[256];
			sprintf(buf,"Error reading \"%s\" - doesn't seem to a PMP file",loadname);
			MessageBox(buf,"Error",MB_OK);
#ifdef NDEBUG
		} catch (...) {
			char buf[256];
			sprintf(buf,"Error loading file \"%s\"",loadname);
			MessageBox(buf,"Error",MB_OK);
#endif
		}
	}	
}

void CMainFrame::SetTitle()
{
	// set document title
	if (GetActiveView()) {
		GetActiveView()->GetDocument()->SetTitle(g_EditorData.GetScenarioName());
	}
}

void CMainFrame::OnViewRenderStats() 
{
	CInfoBox& infobox=g_EditorData.GetInfoBox();
	infobox.SetVisible(!infobox.GetVisible());	
}

void CMainFrame::OnUpdateViewRenderStats(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(g_EditorData.GetInfoBox().GetVisible()==true);
}

void CMainFrame::OnToolsBrushShapeEditor() 
{
	DeselectTools();
	ShowControlBar(&m_wndBrushShapeEditorBar,TRUE,FALSE);

	// set active tool
	g_ToolMan.SetActiveTool(CBrushShapeEditorTool::GetTool());

	// ensure we're in the right editing mode
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);
}

LRESULT CMainFrame::OnMouseWheel(WPARAM wParam,LPARAM lParam) 
{
	// Windows sucks: why the main frame is getting mouse wheel messages
	// when mouse move messages are still going to the view is beyond
	// my comprehension.  Just duplicate the work CScEdView does.
	int xPos = LOWORD(lParam); 
	int yPos = HIWORD(lParam); 
	SHORT fwKeys = LOWORD(wParam);
	SHORT zDelta = HIWORD(wParam);

	g_NaviCam.OnMouseWheelScroll(0,xPos,yPos,float(zDelta)/120.0f);

	return 0;
}

void CMainFrame::OnTestGo() 
{
	g_EditorData.SetMode(CEditorData::TEST_MODE);	
}

void CMainFrame::OnUpdateTestGo(CCmdUI* pCmdUI) 
{
	if (g_EditorData.GetMode()==CEditorData::TEST_MODE || g_EditorData.GetMode()!=CEditorData::SCENARIO_EDIT)
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(TRUE);
}

void CMainFrame::OnTestStop() 
{
	g_EditorData.SetMode(CEditorData::SCENARIO_EDIT);
}

void CMainFrame::OnUpdateTestStop(CCmdUI* pCmdUI) 
{
	if (g_EditorData.GetMode()==CEditorData::TEST_MODE)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

static float getExactGroundLevel( float x, float y )
{
	// TODO MT: If OK with Rich, move to terrain core. Once this works, that is.

	x /= 4.0f;
	y /= 4.0f;

	int xi = (int)floor( x );
	int yi = (int)floor( y );
	float xf = x - (float)xi;
	float yf = y - (float)yi;

	u16* heightmap = g_Terrain.GetHeightMap();
	unsigned long mapsize = g_Terrain.GetVerticesPerSide();

	float h00 = heightmap[yi*mapsize + xi];
	float h01 = heightmap[yi*mapsize + xi + mapsize];
	float h10 = heightmap[yi*mapsize + xi + 1];
	float h11 = heightmap[yi*mapsize + xi + mapsize + 1];

	/*
	if( xf < ( 1.0f - yf ) )
	{
		return( HEIGHT_SCALE * ( ( 1 - xf - yf ) * h00 + xf * h10 + yf * h01 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( xf + yf - 1 ) * h11 + ( 1 - xf ) * h01 + ( 1 - yf ) * h10 ) );
	*/

	/*
	if( xf > yf ) 
	{
		return( HEIGHT_SCALE * ( ( 1 - xf ) * h00 + ( xf - yf ) * h10 + yf * h11 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( 1 - yf ) * h00 + ( yf - xf ) * h01 + xf * h11 ) );
	*/

	return( HEIGHT_SCALE * ( ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ) );

}

static CObjectEntry* GetRandomActorTemplate()
{
	if (g_ObjMan.m_ObjectTypes.size()==0) return 0;

	CObjectEntry* found=0;
	int checkloop=250;
	do {
		u32 type=rand()%(u32)g_ObjMan.m_ObjectTypes.size();
		u32 actorsoftype=(u32)g_ObjMan.m_ObjectTypes[type].m_Objects.size();
		if (actorsoftype>0) {
			found=g_ObjMan.m_ObjectTypes[type].m_Objects[rand()%actorsoftype];
			if (found && found->m_Model && found->m_Model->GetModelDef()->GetNumBones()>0) {
			} else {
				found=0;
			}
		}
	} while (--checkloop && !found);
	
	return found;
}

static CTextureEntry* GetRandomTexture()
{
	if (g_TexMan.m_TerrainTextures.size()==0) return 0;

	CTextureEntry* found=0;
	do {
		u32 type=rand()%(u32)g_TexMan.m_TerrainTextures.size();
		u32 texturesoftype=(u32)g_TexMan.m_TerrainTextures[type].m_Textures.size();
		if (texturesoftype>0) {
			found=g_TexMan.m_TerrainTextures[type].m_Textures[rand()%texturesoftype];
		}
	} while (!found);
	
	return found;
}

void CMainFrame::OnRandomMap() 
{
	const u32 count=5000;	
	const u32 unitsPerDir=u32(sqrt(float(count)));	
	
	u32 i,j;
	u32 vsize=g_Terrain.GetVerticesPerSide()-1;

	for (i=0;i<unitsPerDir;i++) {
		for (j=0;j<unitsPerDir;j++) {
//			float x=CELL_SIZE*vsize*float(i+1)/float(unitsPerDir+1);
//			float z=CELL_SIZE*vsize*float(j+1)/float(unitsPerDir+1);
			float dx=float(rand())/float(RAND_MAX);
			float x=CELL_SIZE*vsize*dx;
			float dz=float(rand())/float(RAND_MAX);
			float z=CELL_SIZE*vsize*dz;
			float y=getExactGroundLevel(x,z);
			CObjectEntry* actortemplate=GetRandomActorTemplate();
			if (actortemplate && actortemplate->m_Model) {
				CUnit* unit=new CUnit(actortemplate,actortemplate->m_Model->Clone());
				g_UnitMan.AddUnit(unit);
				
				CMatrix3D trans;
				trans.SetIdentity();	
				trans.RotateY(2*PI*float(rand())/float(RAND_MAX));
				trans.Translate(x,y,z);
				unit->GetModel()->SetTransform(trans);
			}
		}
	}

/*
	for (i=0;i<vsize;i++) {
		for (j=0;j<vsize;j++) {
			CTextureEntry* tex=GetRandomTexture();
			CMiniPatch* mp=g_Terrain.GetTile(i,j);
			mp->Tex1=tex->GetHandle();
			mp->Tex1Priority=tex->GetType();
			mp->m_Parent->SetDirty(RENDERDATA_UPDATE_VERTICES | RENDERDATA_UPDATE_INDICES);
		}
	}
	g_MiniMap.Rebuild();
*/
}

#define P(n) void CMainFrame::OnEntityPlayer##n() { return OnEntityPlayerX(n); }
P(0); P(1); P(2); P(3); P(4); P(5); P(6); P(7); P(8);
#undef P

void CMainFrame::OnEntityPlayerX(int x)
{
	CEntity* entity = CSelectObjectTool::GetTool()->GetFirstEntity();
	if (entity)
		entity->m_player = (CPlayer*)(intptr_t)x; // HACK: ScEd doesn't have a g_Game, so we can't use its CPlayers
}
