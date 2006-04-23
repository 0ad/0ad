// OptionsDlg.cpp : implementation file
//

#include "precompiled.h"
#include "stdafx.h"
#include "ScEd.h"
#include "OptionsDlg.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog


COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_ScrollSpeed = 0;
	//}}AFX_DATA_INIT
}


void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Slider(pDX, IDC_SLIDER_RTSSCROLLSPEED, m_ScrollSpeed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers

BOOL COptionsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// set up scroll speed slider
	CSliderCtrl* sliderctrl=(CSliderCtrl*) GetDlgItem(IDC_SLIDER_RTSSCROLLSPEED);
	sliderctrl->SetRange(0,10,TRUE);
	sliderctrl->SetPos(m_ScrollSpeed);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
