// ColorButton.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "ColorButton.h"


/////////////////////////////////////////////////////////////////////////////
// CColorButton

CColorButton::CColorButton() : m_Color(0)
{
}

CColorButton::~CColorButton()
{
}


BEGIN_MESSAGE_MAP(CColorButton, CButton)
	//{{AFX_MSG_MAP(CColorButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorButton message handlers

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC* pDC   = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;

    // draw button edges 
	pDC->DrawFrameControl(rect, DFC_BUTTON,DFCS_BUTTONPUSH | DFCS_FLAT );

    // deflate the drawing rect by the size of the button's edges
    rect.DeflateRect( CSize(GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE)));
    
    // fill the interior color 
	pDC->FillSolidRect(rect,m_Color); 
}


void CColorButton::OnClicked() 
{
	CColorDialog dlg;
	if (dlg.DoModal()==IDOK) {
		// store color in button
		m_Color=dlg.m_cc.rgbResult;
		// force redraw
		Invalidate();
		UpdateWindow();
	}	
}
