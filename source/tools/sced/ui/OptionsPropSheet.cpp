// OptionsPropSheet.cpp : implementation file
//

#include "precompiled.h"
#include "stdafx.h"
#include "ScEd.h"
#include "OptionsPropSheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsPropSheet

IMPLEMENT_DYNAMIC(COptionsPropSheet, CPropertySheet)

COptionsPropSheet::COptionsPropSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{	
	AddPage(&m_NavigatePage);
	AddPage(&m_ShadowsPage);
}

COptionsPropSheet::COptionsPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_NavigatePage);
	AddPage(&m_ShadowsPage);
}

COptionsPropSheet::~COptionsPropSheet()
{
}


BEGIN_MESSAGE_MAP(COptionsPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(COptionsPropSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsPropSheet message handlers

BOOL COptionsPropSheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	// NAVIGATION CONTROLS:
	// setup scroll speed slider
	CSliderCtrl* sliderctrl=(CSliderCtrl*) m_NavigatePage.GetDlgItem(IDC_SLIDER_RTSSCROLLSPEED);
	sliderctrl->SetRange(0,10,TRUE);
	sliderctrl->SetPos(m_NavigatePage.m_ScrollSpeed);

	// SHADOW CONTROLS:
	// setup checkbox
//	CSliderCtrl* sliderctrl=(CSliderCtrl*) m_NavigatePage.GetDlgItem(IDC_SLIDER_RTSSCROLLSPEED);
//	sliderctrl->SetRange(0,10,TRUE);
//	sliderctrl->SetPos(m_NavigatePage.m_ScrollSpeed);

	// setup shadow colour

	// setup quality slider
	return bResult;
}
