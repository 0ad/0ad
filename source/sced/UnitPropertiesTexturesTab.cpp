// UnitPropertiesTexturesTab.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "UnitPropertiesTexturesTab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTexturesTab dialog


CUnitPropertiesTexturesTab::CUnitPropertiesTexturesTab(CWnd* pParent /*=NULL*/)
	: CDialog(CUnitPropertiesTexturesTab::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUnitPropertiesTexturesTab)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CUnitPropertiesTexturesTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUnitPropertiesTexturesTab)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUnitPropertiesTexturesTab, CDialog)
	//{{AFX_MSG_MAP(CUnitPropertiesTexturesTab)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTexturesTab message handlers
