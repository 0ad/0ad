// MapSizeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "MapSizeDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMapSizeDlg dialog


CMapSizeDlg::CMapSizeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMapSizeDlg::IDD, pParent), m_MapSize(11)
{
	//{{AFX_DATA_INIT(CMapSizeDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMapSizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapSizeDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapSizeDlg, CDialog)
	//{{AFX_MSG_MAP(CMapSizeDlg)
	ON_BN_CLICKED(IDC_RADIO_HUGE, OnRadioHuge)
	ON_BN_CLICKED(IDC_RADIO_LARGE, OnRadioLarge)
	ON_BN_CLICKED(IDC_RADIO_MEDIUM, OnRadioMedium)
	ON_BN_CLICKED(IDC_RADIO_SMALL, OnRadioSmall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapSizeDlg message handlers

BOOL CMapSizeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	OnRadioMedium();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMapSizeDlg::OnRadioHuge() 
{
	CheckDlgButton(IDC_RADIO_SMALL,FALSE);
	CheckDlgButton(IDC_RADIO_MEDIUM,FALSE);
	CheckDlgButton(IDC_RADIO_LARGE,FALSE);
	CheckDlgButton(IDC_RADIO_HUGE,TRUE);
	m_MapSize=17;
}

void CMapSizeDlg::OnRadioLarge() 
{
	CheckDlgButton(IDC_RADIO_SMALL,FALSE);
	CheckDlgButton(IDC_RADIO_MEDIUM,FALSE);
	CheckDlgButton(IDC_RADIO_LARGE,TRUE);
	CheckDlgButton(IDC_RADIO_HUGE,FALSE);
	m_MapSize=13;
}

void CMapSizeDlg::OnRadioMedium() 
{
	CheckDlgButton(IDC_RADIO_SMALL,FALSE);
	CheckDlgButton(IDC_RADIO_MEDIUM,TRUE);
	CheckDlgButton(IDC_RADIO_LARGE,FALSE);
	CheckDlgButton(IDC_RADIO_HUGE,FALSE);
	m_MapSize=11;
}

void CMapSizeDlg::OnRadioSmall() 
{
	CheckDlgButton(IDC_RADIO_SMALL,TRUE);
	CheckDlgButton(IDC_RADIO_MEDIUM,FALSE);
	CheckDlgButton(IDC_RADIO_LARGE,FALSE);
	CheckDlgButton(IDC_RADIO_HUGE,FALSE);
	m_MapSize=9;
}
