#if !defined(AFX_UNITPROPERTIESTEXTURESTAB_H__5F56FF68_6305_4E2E_BC75_6CF208ED2DBB__INCLUDED_)
#define AFX_UNITPROPERTIESTEXTURESTAB_H__5F56FF68_6305_4E2E_BC75_6CF208ED2DBB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UnitPropertiesTexturesTab.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTexturesTab dialog

class CUnitPropertiesTexturesTab : public CDialog
{
// Construction
public:
	CUnitPropertiesTexturesTab(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUnitPropertiesTexturesTab)
	enum { IDD = IDD_UNITPROPERTIES_TEXTURES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUnitPropertiesTexturesTab)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUnitPropertiesTexturesTab)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UNITPROPERTIESTEXTURESTAB_H__5F56FF68_6305_4E2E_BC75_6CF208ED2DBB__INCLUDED_)
