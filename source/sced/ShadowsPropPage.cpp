// ShadowsPropPage.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "ShadowsPropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShadowsPropPage property page

IMPLEMENT_DYNCREATE(CShadowsPropPage, CPropertyPage)

CShadowsPropPage::CShadowsPropPage() : CPropertyPage(CShadowsPropPage::IDD)
{
	//{{AFX_DATA_INIT(CShadowsPropPage)
	m_EnableShadows = FALSE;
	//}}AFX_DATA_INIT
}

CShadowsPropPage::~CShadowsPropPage()
{
}

void CShadowsPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShadowsPropPage)
	DDX_Control(pDX, IDC_BUTTON_SHADOWCOLOR, m_ShadowColor);
	DDX_Check(pDX, IDC_CHECK_SHADOWS, m_EnableShadows);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CShadowsPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CShadowsPropPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShadowsPropPage message handlers
