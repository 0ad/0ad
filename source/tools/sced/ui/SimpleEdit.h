#if !defined(AFX_SIMPLEEDIT_H__190C3373_D31B_4037_9851_2C3255DFEA53__INCLUDED_)
#define AFX_SIMPLEEDIT_H__190C3373_D31B_4037_9851_2C3255DFEA53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SimpleEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSimpleEdit dialog

class CSimpleEdit : public CDialog
{
// Construction
public:
	CSimpleEdit(const char* title,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSimpleEdit)
	enum { IDD = IDD_DIALOG_SIMPLEEDIT };
	CString	m_Text;
	CString	m_Title;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimpleEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSimpleEdit)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMPLEEDIT_H__190C3373_D31B_4037_9851_2C3255DFEA53__INCLUDED_)
