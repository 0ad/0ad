// UnitPropertiesAnimationsTab.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "UnitPropertiesAnimationsTab.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesAnimationsTab dialog


CUnitPropertiesAnimationsTab::CUnitPropertiesAnimationsTab(CWnd* pParent /*=NULL*/)
	: CDialog(CUnitPropertiesAnimationsTab::IDD, pParent)
{
	//{{AFX_DATA_INIT(CUnitPropertiesAnimationsTab)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CUnitPropertiesAnimationsTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUnitPropertiesAnimationsTab)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUnitPropertiesAnimationsTab, CDialog)
	//{{AFX_MSG_MAP(CUnitPropertiesAnimationsTab)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesAnimationsTab message handlers
