#include <assert.h>
#include "stdafx.h"
#include "ToolManager.h"
#include "ElevToolsDlgBar.h"
#include "RaiseElevationTool.h"
#include "SmoothElevationTool.h"
#include "UIGlobals.h"

BEGIN_MESSAGE_MAP(CElevToolsDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CElevToolsDlgBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_RADIO_RAISE, OnRadioRaise)
	ON_BN_CLICKED(IDC_RADIO_SMOOTH, OnRadioSmooth)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BRUSHEFFECT, OnReleasedCaptureSliderBrushEffect)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_BRUSHSIZE, OnReleasedCaptureSliderBrushSize)
END_MESSAGE_MAP()


CElevToolsDlgBar::CElevToolsDlgBar() : m_Mode(RAISELOWER_MODE)
{
}

CElevToolsDlgBar::~CElevToolsDlgBar()
{
}

BOOL CElevToolsDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CElevToolsDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}

void CElevToolsDlgBar::SetRaiseControls()
{
	// set up brush size slider
	CSliderCtrl* sizectrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sizectrl->SetRange(0,CRaiseElevationTool::MAX_BRUSH_SIZE,TRUE);
	sizectrl->SetPos(CRaiseElevationTool::GetTool()->GetBrushSize());

	// set up brush effect slider
	CSliderCtrl* effectctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHEFFECT);
	effectctrl->SetRange(1,CRaiseElevationTool::MAX_SPEED/16,TRUE);
	effectctrl->SetPos(CRaiseElevationTool::GetTool()->GetSpeed()/16);

	// setup radio buttons
	CheckDlgButton(IDC_RADIO_RAISE,TRUE);
	CheckDlgButton(IDC_RADIO_SMOOTH,FALSE);
}

void CElevToolsDlgBar::SetSmoothControls()
{
	// set up brush size slider
	CSliderCtrl* sizectrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sizectrl->SetRange(0,CSmoothElevationTool::MAX_BRUSH_SIZE,TRUE);
	sizectrl->SetPos(CSmoothElevationTool::GetTool()->GetBrushSize());

	// set up brush effect slider
	CSliderCtrl* effectctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHEFFECT);
	effectctrl->SetRange(1,int(CSmoothElevationTool::MAX_SMOOTH_POWER),TRUE);
	effectctrl->SetPos(int(CSmoothElevationTool::GetTool()->GetSmoothPower()));

	// setup radio buttons
	CheckDlgButton(IDC_RADIO_RAISE,FALSE);
	CheckDlgButton(IDC_RADIO_SMOOTH,TRUE);
}

BOOL CElevToolsDlgBar::OnInitDialog()
{
	 // get the current window size and position
	CRect rect;
	GetWindowRect(rect);

	// now change the size, position, and Z order of the window.
	::SetWindowPos(m_hWnd,HWND_TOPMOST,10,rect.top,rect.Width(),rect.Height(),SWP_HIDEWINDOW);

	// initially show raise controls
	SetRaiseControls();

	return TRUE;  	              
}

void CElevToolsDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

void CElevToolsDlgBar::OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	if (IsDlgButtonChecked(IDC_RADIO_RAISE)) {
		CRaiseElevationTool::GetTool()->SetBrushSize(sliderctrl->GetPos());
	} else {
		CSmoothElevationTool::GetTool()->SetBrushSize(sliderctrl->GetPos());
	}

	*pResult = 0;
}

void CElevToolsDlgBar::OnReleasedCaptureSliderBrushEffect(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_BRUSHEFFECT);
	if (IsDlgButtonChecked(IDC_RADIO_RAISE)) {
		CRaiseElevationTool::GetTool()->SetSpeed(sliderctrl->GetPos()*16);
	} else {
		CSmoothElevationTool::GetTool()->SetSmoothPower(float(sliderctrl->GetPos()));
	}

	*pResult = 0;
}

void CElevToolsDlgBar::OnShow()
{
	switch (m_Mode) {
		case RAISELOWER_MODE:
			SetRaiseControls();
			g_ToolMan.SetActiveTool(CRaiseElevationTool::GetTool());
			break;

		case SMOOTH_MODE:
			SetSmoothControls();
			g_ToolMan.SetActiveTool(CSmoothElevationTool::GetTool());
			break;
			
		default:
			assert(0);
	}
}

void CElevToolsDlgBar::OnRadioRaise()
{
	// set UI elements for raise/lower
	SetRaiseControls();

	// set current tool
	g_ToolMan.SetActiveTool(CRaiseElevationTool::GetTool());

	// redraw sliders
	CWnd* sizeslider=GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sizeslider->Invalidate();
	sizeslider->UpdateWindow();

	CWnd* effectslider=GetDlgItem(IDC_SLIDER_BRUSHEFFECT);
	effectslider->Invalidate();
	effectslider->UpdateWindow();

	// store mode
	m_Mode=RAISELOWER_MODE;
}

void CElevToolsDlgBar::OnRadioSmooth()
{
	// set UI elements for smooth
	SetSmoothControls();

	// set current tool
	g_ToolMan.SetActiveTool(CSmoothElevationTool::GetTool());

	// redraw sliders
	CWnd* sizeslider=GetDlgItem(IDC_SLIDER_BRUSHSIZE);
	sizeslider->Invalidate();
	sizeslider->UpdateWindow();

	CWnd* effectslider=GetDlgItem(IDC_SLIDER_BRUSHEFFECT);
	effectslider->Invalidate();
	effectslider->UpdateWindow();

	// store mode
	m_Mode=SMOOTH_MODE;
}

