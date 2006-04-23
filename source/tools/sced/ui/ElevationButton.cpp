// ElevationButton.cpp : implementation file
//

#include "precompiled.h"
#include <math.h>
#include "stdafx.h"
#include "ScEd.h"
#include "ElevationButton.h"

#include "MathUtil.h"

/////////////////////////////////////////////////////////////////////////////
// CElevationButton

CElevationButton::CElevationButton() : m_Elevation(45)
{
}

CElevationButton::~CElevationButton()
{
}


BEGIN_MESSAGE_MAP(CElevationButton, CButton)
	//{{AFX_MSG_MAP(CElevationButton)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CElevationButton message handlers

void CElevationButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC* pDC   = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;

    // draw button edges 
	pDC->DrawFrameControl(rect, DFC_BUTTON,DFCS_BUTTONPUSH | DFCS_FLAT );

    // deflate the drawing rect by the size of the button's edges
    rect.DeflateRect( CSize(GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE)));
    
    // fill the interior color 
	pDC->FillSolidRect(rect,RGB(0,0,0)); 	

	// shrink rect before drawing anything else 
    rect.DeflateRect(15,5);

	// create a hollow brush
	CBrush brush;
	brush.CreateStockObject(HOLLOW_BRUSH);
	CBrush* oldbrush=pDC->SelectObject(&brush);

	// draw horizon
	pDC->SetROP2(R2_WHITE);
    pDC->MoveTo(rect.left,rect.bottom);
    pDC->LineTo(rect.right,rect.bottom);

	// draw direction arrow
	pDC->SetROP2(R2_COPYPEN);
	CPen pen;
	pen.CreatePen(PS_SOLID,1,RGB(255,0,0));
	CPen* oldpen=pDC->SelectObject(&pen);

	float dy=float(rect.bottom-rect.top)/2.0f;
	float dx=float(rect.right-rect.left)/2.0f;
	float r=(float) sqrt(2*dx*dx);

	float cx=float(rect.left+rect.right)/2.0f;
	float cy=float(rect.bottom);

	float dirx=cos(DEGTORAD(m_Elevation));
	float diry=-sin(DEGTORAD(m_Elevation));

	CPoint m_One(int(cx+dirx*r),int(cy+diry*r));
	CPoint m_Two(int(cx+dirx*(r-14)),int(cy+diry*(r-14)));

	double slopy , cosy , siny;
	 double Par = 6.5;  //length of Arrow (>)
	slopy = atan2( float( m_One.y - m_Two.y ),float( m_One.x - m_Two.x ) );
  cosy = cos( slopy );
  siny = sin( slopy ); //need math.h for these functions

  //draw a line between the 2 endpoint
  pDC->MoveTo( m_One );
  pDC->LineTo( m_Two );
  
  pDC->MoveTo( m_Two );
  pDC->LineTo( m_Two.x + int( Par * cosy - ( Par / 2.0 * siny ) ),
    m_Two.y + int( Par * siny + ( Par / 2.0 * cosy ) ) );
  pDC->LineTo( m_Two.x + int( Par * cosy + Par / 2.0 * siny ),
    m_Two.y - int( Par / 2.0 * cosy - Par * siny ) );
  pDC->LineTo( m_Two );


	pDC->SelectObject(oldpen);
	pDC->SelectObject(oldbrush);
}
