#include "stdafx.h"
#define _IGNORE_WGL_H_
#include "UserConfig.h"
#include "MainFrm.h"
#include "Model.h"
#include "Unit.h"
#include "UnitManager.h"
#include "ObjectManager.h"
#include "UnitPropertiesDlgBar.h"

#include "EditorData.h"
#include "UIGlobals.h"
#undef _IGNORE_WGL_H_

BEGIN_MESSAGE_MAP(CUnitPropertiesDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CUnitPropertiesDlgBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_BACK, OnButtonBack)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
	ON_BN_CLICKED(IDC_BUTTON_MODELBROWSE, OnButtonModelBrowse)
	ON_BN_CLICKED(IDC_BUTTON_TEXTUREBROWSE, OnButtonTextureBrowse)
	ON_BN_CLICKED(IDC_BUTTON_ANIMATIONBROWSE, OnButtonAnimationBrowse)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////
// MakeRelativeFileName: adjust the given filename to return the filename
// relative to the mods/official directory - also swizzle backslashes
// to forward slashes
// TODO, RC: need to make this work with other root directories
static CStr MakeRelativeFileName(const char* fname)
{
	CStr result;
	const char* ptr=strstr(fname,"mods\\official\\");
	if (!ptr) {
		result=fname;
	} else {
		result=ptr+strlen("mods\\official\\");
	}
	int len=result.Length();
	for (int i=0;i<len;i++) {
		if (result[i]=='\\') result[i]='/';
	}
	return result;
}

CUnitPropertiesDlgBar::CUnitPropertiesDlgBar() : m_Object(0)
{
}

CUnitPropertiesDlgBar::~CUnitPropertiesDlgBar()
{
}

BOOL CUnitPropertiesDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CUnitPropertiesDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}


BOOL CUnitPropertiesDlgBar::OnInitDialog()
{
	 // get the current window size and position
	CRect rect;
	GetWindowRect(rect);

	// now change the size, position, and Z order of the window.
	::SetWindowPos(m_hWnd,HWND_TOPMOST,10,rect.top,rect.Width(),rect.Height(),SWP_HIDEWINDOW);

/*
	CTabCtrl *tabCtrl=(CTabCtrl*) GetDlgItem(IDC_TAB_PAGES);
	tabCtrl->InsertItem(0, "Tab 1", 0);	// add some test pages to the tab
	tabCtrl->InsertItem(1, "Tab 2", 1);
	tabCtrl->InsertItem(2, "Tab 3", 2);
	tabCtrl->InsertItem(3, "Tab 4", 3);
*/
	// initialise data from editor data
	UpdatePropertiesDlg();

	return TRUE;  	              
}

void CUnitPropertiesDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

void CUnitPropertiesDlgBar::OnButtonBack()
{
	// save current object (if we've got one) before going back
	if (m_Object) {

		CStr filename("data/mods/official/art/actors/");
		filename+=g_ObjMan.m_ObjectTypes[m_Object->m_Type].m_Name;
		filename+="/";
		filename+=m_Object->m_Name;
		filename+=".xml";
		m_Object->Save((const char*) filename);

		// and rebuild the model
		UpdateEditorData();
	}

	CMainFrame* mainfrm=(CMainFrame*) AfxGetMainWnd();
	mainfrm->OnUnitTools();
}

void CUnitPropertiesDlgBar::OnButtonRefresh() 
{
	UpdateEditorData();
}

void CUnitPropertiesDlgBar::OnButtonTextureBrowse() 
{
	const char* filter="DDS Files|*.dds|PNG Files|*.png||";
	CFileDialog dlg(TRUE,g_UserCfg.GetOptionString(CFG_TEXTUREEXT),0,OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,filter,0);
	dlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_MODELTEXLOADDIR);

	if (dlg.DoModal()==IDOK) {
		CStr filename=MakeRelativeFileName(dlg.m_ofn.lpstrFile);

		CStr dir(dlg.m_ofn.lpstrFile);
		dir=dir.Left(dlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_MODELTEXLOADDIR,(const char*) dir);

		CWnd* texture=GetDlgItem(IDC_EDIT_TEXTURE);
		texture->SetWindowText(filename);
	}
}

void CUnitPropertiesDlgBar::OnButtonAnimationBrowse() 
{
	const char* filter="PSA Files|*.psa||";
	CFileDialog dlg(TRUE,"psa",0,OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,filter,0);
	dlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_MODELANIMATIONDIR);

	if (dlg.DoModal()==IDOK) {
		CStr filename=MakeRelativeFileName(dlg.m_ofn.lpstrFile);

		CStr dir(dlg.m_ofn.lpstrFile);
		dir=dir.Left(dlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_MODELANIMATIONDIR,(const char*) dir);

		CWnd* animation=GetDlgItem(IDC_EDIT_ANIMATION);
		animation->SetWindowText(filename);
	}
}

void CUnitPropertiesDlgBar::OnButtonModelBrowse() 
{
	const char* filter="PMD Files|*.pmd|0ADM Files|*.0adm||";
	CFileDialog dlg(TRUE,"pmd",0,OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,filter,0);
	dlg.m_ofn.lpstrInitialDir=g_UserCfg.GetOptionString(CFG_MODELLOADDIR);

	if (dlg.DoModal()==IDOK) {
		CStr filename=MakeRelativeFileName(dlg.m_ofn.lpstrFile);
		
		CStr dir(dlg.m_ofn.lpstrFile);
		dir=dir.Left(dlg.m_ofn.nFileOffset-1);
		g_UserCfg.SetOptionString(CFG_MODELLOADDIR,(const char*) dir);

		CWnd* texture=GetDlgItem(IDC_EDIT_MODEL);
		texture->SetWindowText(filename);
	}
}

void CUnitPropertiesDlgBar::UpdateEditorData()
{
	if (!m_Object) {
		g_ObjMan.SetSelectedObject(0);
		return;
	}

	CString str;
	
	CWnd* name=GetDlgItem(IDC_EDIT_NAME);
	name->GetWindowText(str);
	m_Object->m_Name=str;
	
	CWnd* model=GetDlgItem(IDC_EDIT_MODEL);
	model->GetWindowText(str);
	m_Object->m_ModelName=str;

	CWnd* texture=GetDlgItem(IDC_EDIT_TEXTURE);
	texture->GetWindowText(str);
	m_Object->m_TextureName=str;

	CWnd* animation=GetDlgItem(IDC_EDIT_ANIMATION);
	animation->GetWindowText(str);
	if (m_Object->m_Animations.size()==0) {
		m_Object->m_Animations.resize(1);
		m_Object->m_Animations[0].m_AnimName="Idle";
	}
	m_Object->m_Animations[0].m_FileName=str;

	std::vector<CUnit*> animupdatelist;
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		if (units[i]->GetModel()->GetModelDef()==m_Object->m_Model->GetModelDef()) {
			animupdatelist.push_back(units[i]);
		}
	}
	if (m_Object->BuildModel()) {
		g_ObjMan.SetSelectedObject(m_Object);
		CSkeletonAnim* anim=m_Object->m_Model->GetAnimation();
		if (anim) {
			for (uint i=0;i<animupdatelist.size();++i) {
				animupdatelist[i]->GetModel()->SetAnimation(anim);
			}
		}
	} else {
		g_ObjMan.SetSelectedObject(0);
	}
}

void CUnitPropertiesDlgBar::UpdatePropertiesDlg()
{
	if (!m_Object) return;

	CWnd* name=GetDlgItem(IDC_EDIT_NAME);
	if (name) {
		name->SetWindowText(m_Object->m_Name);
	}
	
	CWnd* model=GetDlgItem(IDC_EDIT_MODEL);
	if (model) {
		model->SetWindowText(m_Object->m_ModelName);
	}

	CWnd* texture=GetDlgItem(IDC_EDIT_TEXTURE);
	if (texture) {
		texture->SetWindowText(m_Object->m_TextureName);
	}

	CWnd* animation=GetDlgItem(IDC_EDIT_ANIMATION);
	if (animation) {
		if (m_Object->m_Animations.size()>0) {
			animation->SetWindowText(m_Object->m_Animations[0].m_FileName);
		}
	}
}


void CUnitPropertiesDlgBar::SetObject(CObjectEntry* obj) 
{ 
	m_Object=obj; 
	if (m_Object) {
		if (m_Object->BuildModel()) {
			g_ObjMan.SetSelectedObject(m_Object);
		} else {
			g_ObjMan.SetSelectedObject(0);
		}
	}
	UpdatePropertiesDlg();
}
