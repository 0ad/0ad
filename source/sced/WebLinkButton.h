#if !defined(AFX_WEBLINKBUTTON_H__99561EFB_6294_4249_B339_8364DD26EDB5__INCLUDED_)
#define AFX_WEBLINKBUTTON_H__99561EFB_6294_4249_B339_8364DD26EDB5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WebLinkButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWebLinkButton window

class CWebLinkButton : public CButton
{
// Construction
public:
	CWebLinkButton();

// Attributes
public:
	CString m_Address;
	BOOL m_Clicked;
	HCURSOR m_HandCursor;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWebLinkButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWebLinkButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWebLinkButton)
	afx_msg void OnClicked();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WEBLINKBUTTON_H__99561EFB_6294_4249_B339_8364DD26EDB5__INCLUDED_)
