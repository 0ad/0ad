#include "precompiled.h"
#include "stdafx.h"
#define _IGNORE_WGL_H_
#include "MainFrm.h"
#include "EditorData.h"
#include "BrushShapeEditorTool.h"
#include "BrushShapeEditorDlgBar.h"
#undef _IGNORE_WGL_H_

#undef CRect // because it was redefined to PS_Rect in Overlay.h

BEGIN_MESSAGE_MAP(CBrushShapeEditorDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CBrushShapeEditorDlgBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_BACK, OnButtonBack)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_CBN_SELCHANGE(IDC_COMBO_TERRAINTYPES, OnSelChangeBrush)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BRUSHSIZE, OnReleasedCaptureSliderBrushSize)
END_MESSAGE_MAP()


CBrushShapeEditorDlgBar::CBrushShapeEditorDlgBar()
{
}

CBrushShapeEditorDlgBar::~CBrushShapeEditorDlgBar()
{
}

BOOL CBrushShapeEditorDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CBrushShapeEditorDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}

void CBrushShapeEditorDlgBar::OnButtonAdd() 
{
}

BOOL CBrushShapeEditorDlgBar::OnInitDialog()
{
	 // get the current window size and position
	CRect rect;
	GetWindowRect(rect);

	// now change the size, position, and Z order of the window.
	::SetWindowPos(m_hWnd,HWND_TOPMOST,10,rect.top,rect.Width(),rect.Height(),SWP_HIDEWINDOW);

	// set up brush size slider
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sliderctrl->SetRange(0,CBrushShapeEditorTool::MAX_BRUSH_SIZE);
	sliderctrl->SetPos(CBrushShapeEditorTool::GetTool()->GetBrushSize());

	return TRUE;  	              
}

void CBrushShapeEditorDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

void CBrushShapeEditorDlgBar::OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	CBrushShapeEditorTool::GetTool()->SetBrushSize(sliderctrl->GetPos());
	*pResult = 0;
}


void CBrushShapeEditorDlgBar::OnSelChangeBrush()
{
}

void CBrushShapeEditorDlgBar::OnButtonBack()
{
	CMainFrame* mainfrm=(CMainFrame*) AfxGetMainWnd();
	mainfrm->DeselectTools();
}
