#if !defined(AFX_UNITPROPERTIESANIMATIONSTAB_H__EFA556CF_E800_419B_8438_4CCB00217B1A__INCLUDED_)
#define AFX_UNITPROPERTIESANIMATIONSTAB_H__EFA556CF_E800_419B_8438_4CCB00217B1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UnitPropertiesAnimationsTab.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesAnimationsTab dialog

class CUnitPropertiesAnimationsTab : public CDialog
{
// Construction
public:
	CUnitPropertiesAnimationsTab(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUnitPropertiesAnimationsTab)
	enum { IDD = IDD_UNITPROPERTIES_ANIMATIONS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUnitPropertiesAnimationsTab)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUnitPropertiesAnimationsTab)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UNITPROPERTIESANIMATIONSTAB_H__EFA556CF_E800_419B_8438_4CCB00217B1A__INCLUDED_)
