// NavigatePropPage.cpp : implementation file
//

#include "precompiled.h"
#include "stdafx.h"
#include "ScEd.h"
#include "NavigatePropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNavigatePropPage property page

IMPLEMENT_DYNCREATE(CNavigatePropPage, CPropertyPage)

CNavigatePropPage::CNavigatePropPage() : CPropertyPage(CNavigatePropPage::IDD)
{
	//{{AFX_DATA_INIT(CNavigatePropPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CNavigatePropPage::~CNavigatePropPage()
{
}

void CNavigatePropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNavigatePropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNavigatePropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CNavigatePropPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNavigatePropPage message handlers
