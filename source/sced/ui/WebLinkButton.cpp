// WebLinkButton.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "WebLinkButton.h"

/////////////////////////////////////////////////////////////////////////////
// CWebLinkButton

CWebLinkButton::CWebLinkButton() : m_Clicked(FALSE), m_HandCursor(0)
{
}

CWebLinkButton::~CWebLinkButton()
{
}


BEGIN_MESSAGE_MAP(CWebLinkButton, CButton)
	//{{AFX_MSG_MAP(CWebLinkButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWebLinkButton message handlers

void CWebLinkButton::OnClicked() 
{
	m_Clicked=TRUE;

	CString url("http://");
	url+=m_Address;
	ShellExecute(NULL,"open",(LPCTSTR) url,NULL,NULL,SW_SHOW);
}

void CWebLinkButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC* pDC   = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;

	if (m_Address.GetLength()>0) {
		
		// get a LOGFONT for current font, but underlined
		CFont* font=GetFont();
		LOGFONT logFont;
		font->GetLogFont(&logFont);
		logFont.lfUnderline=TRUE;
	
		// build font from this, select into DC
		CFont newfont;
		newfont.CreateFontIndirect(&logFont);
		CFont* oldfont=pDC->SelectObject(&newfont);

		// draw text
		pDC->SetTextColor(m_Clicked ? RGB(0,0,128) : RGB(0,0,255));
		pDC->TextOut(rect.left,(rect.bottom+rect.top)/2,m_Address);

		// restore old font
		pDC->SelectObject(&oldfont);
	}
}


void CWebLinkButton::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_HandCursor){//!m_MouseHovering) {
		m_HandCursor=AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32649));
		if (!m_HandCursor) {
			// ack - couldn't get the hand : must be running on Win95/Win98 (unsupported?) .. just
			// use any difference cursor for the minute
			m_HandCursor=AfxGetApp()->LoadStandardCursor(IDC_UPARROW);
		}
	}
	::SetCursor(m_HandCursor);
}
