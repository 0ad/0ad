#if !defined(AFX_NAVIGATEPROPPAGE_H__BD8DC549_74F0_425E_9970_77BAE8F735E5__INCLUDED_)
#define AFX_NAVIGATEPROPPAGE_H__BD8DC549_74F0_425E_9970_77BAE8F735E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NavigatePropPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNavigatePropPage dialog

class CNavigatePropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CNavigatePropPage)

// Construction
public:
	CNavigatePropPage();
	~CNavigatePropPage();

// Dialog Data
	//{{AFX_DATA(CNavigatePropPage)
	enum { IDD = IDD_PROPPAGE_NAVIGATION };
	int		m_ScrollSpeed;
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNavigatePropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNavigatePropPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NAVIGATEPROPPAGE_H__BD8DC549_74F0_425E_9970_77BAE8F735E5__INCLUDED_)
