#if !defined(AFX_OPTIONSPROPSHEET_H__11CE8789_3E13_4D06_966E_6A6DA9C36E13__INCLUDED_)
#define AFX_OPTIONSPROPSHEET_H__11CE8789_3E13_4D06_966E_6A6DA9C36E13__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsPropSheet.h : header file
//

#include "NavigatePropPage.h"
#include "ShadowsPropPage.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsPropSheet

class COptionsPropSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(COptionsPropSheet)

// Construction
public:
	COptionsPropSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	COptionsPropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
	CNavigatePropPage m_NavigatePage;
	CShadowsPropPage m_ShadowsPage;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsPropSheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COptionsPropSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(COptionsPropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSPROPSHEET_H__11CE8789_3E13_4D06_966E_6A6DA9C36E13__INCLUDED_)
