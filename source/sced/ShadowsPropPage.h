#if !defined(AFX_SHADOWSPROPPAGE_H__49CB83C3_EFFA_4D6B_8EAC_A728D4F85963__INCLUDED_)
#define AFX_SHADOWSPROPPAGE_H__49CB83C3_EFFA_4D6B_8EAC_A728D4F85963__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ShadowsPropPage.h : header file
//

#include "ColorButton.h"

/////////////////////////////////////////////////////////////////////////////
// CShadowsPropPage dialog

class CShadowsPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CShadowsPropPage)

// Construction
public:
	CShadowsPropPage();
	~CShadowsPropPage();

// Dialog Data
	//{{AFX_DATA(CShadowsPropPage)
	enum { IDD = IDD_PROPPAGE_SHADOWS };
	CColorButton	m_ShadowColor;
	BOOL	m_EnableShadows;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CShadowsPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CShadowsPropPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHADOWSPROPPAGE_H__49CB83C3_EFFA_4D6B_8EAC_A728D4F85963__INCLUDED_)
