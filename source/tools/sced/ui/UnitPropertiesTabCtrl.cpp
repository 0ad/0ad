// UnitPropertiesTabCtrl.cpp : implementation file
//

#include "precompiled.h"
#include "stdafx.h"
#include "ScEd.h"
#include "UnitPropertiesTabCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTabCtrl

CUnitPropertiesTabCtrl::CUnitPropertiesTabCtrl()
{
	m_TexturesTab=new CUnitPropertiesTexturesTab;
	m_AnimationsTab=new CUnitPropertiesAnimationsTab;
	m_tabPages[0]=m_TexturesTab;
	m_tabPages[1]=m_AnimationsTab;
	m_nNumberOfPages=2;
}

CUnitPropertiesTabCtrl::~CUnitPropertiesTabCtrl()
{
	delete m_TexturesTab;
	delete m_AnimationsTab;
}


BEGIN_MESSAGE_MAP(CUnitPropertiesTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CUnitPropertiesTabCtrl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTabCtrl message handlers

void CUnitPropertiesTabCtrl::Init() 
{
	m_tabCurrent=0;

	PSTR pszTabItems[] = { 
		"Textures", 
		"Animations", 
		NULL 
	}; 

	TC_ITEM tcItem; 
	for(INT i = 0; pszTabItems[i] != NULL; i++) 
	{ 
		tcItem.mask = TCIF_TEXT; 
		tcItem.pszText = pszTabItems[i]; 
		tcItem.cchTextMax = strlen(pszTabItems[i]); 
		InsertItem(i,&tcItem); 
	} 

	m_TexturesTab->Create(IDD_UNITPROPERTIES_TEXTURES,this);
	m_AnimationsTab->Create(IDD_UNITPROPERTIES_ANIMATIONS,this);

	m_TexturesTab->ShowWindow(SW_SHOW);
	m_AnimationsTab->ShowWindow(SW_HIDE);
 	
	
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX=itemRect.left;
	nY=itemRect.bottom+1;
	nXc=tabRect.right-itemRect.left-1;
	nYc=tabRect.bottom-nY-1;

	m_tabPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for(int nCount=1; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
	}
}
