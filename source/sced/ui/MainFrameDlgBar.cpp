// MainFrameDlgBar.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "MainFrm.h"
#include "MainFrameDlgBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CMainFrameDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CMainFrameDlgBar)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_TEXTURETOOLS, OnButtonTextureTools)
	ON_BN_CLICKED(IDC_BUTTON_ELEVATIONTOOLS, OnButtonElevationTools)
	ON_BN_CLICKED(IDC_BUTTON_MODELTOOLS, OnButtonModelTools)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrameDlgBar dialog


CMainFrameDlgBar::CMainFrameDlgBar()
{
	//{{AFX_DATA_INIT(CMainFrameDlgBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BOOL CMainFrameDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrameDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}

void CMainFrameDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrameDlgBar message handlers

void CMainFrameDlgBar::OnButtonSelect() 
{
	CMainFrame* parent=(CMainFrame*) GetParent();
	parent->DeselectTools();
}

void CMainFrameDlgBar::OnButtonTextureTools() 
{
	CMainFrame* parent=(CMainFrame*) GetParent();
	parent->OnTextureTools();
}

void CMainFrameDlgBar::OnButtonElevationTools() 
{
	CMainFrame* parent=(CMainFrame*) GetParent();
	parent->OnElevationTools();
}

void CMainFrameDlgBar::OnButtonModelTools() 
{
	CMainFrame* parent=(CMainFrame*) GetParent();
	parent->OnUnitTools();
}

BOOL CMainFrameDlgBar::OnInitDialog() 
{
	CButton* btnSelect=(CButton*) GetDlgItem(IDC_BUTTON_SELECT);
	btnSelect->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_SELECT)));
	
	CButton* btnTexTools=(CButton*) GetDlgItem(IDC_BUTTON_TEXTURETOOLS);
	btnTexTools->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_TEXTURETOOLS)));

	CButton* btnElevTools=(CButton*) GetDlgItem(IDC_BUTTON_ELEVATIONTOOLS);
	btnElevTools->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_ELEVATIONTOOLS)));

	CButton* btnMdlTools=(CButton*) GetDlgItem(IDC_BUTTON_MODELTOOLS);
	btnMdlTools->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_MODELTOOLS)));

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
