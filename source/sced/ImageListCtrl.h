#if !defined(AFX_IMAGELISTCTRL_H__EAFCDB8A_5A00_4F1E_966A_3036315B92E0__INCLUDED_)
#define AFX_IMAGELISTCTRL_H__EAFCDB8A_5A00_4F1E_966A_3036315B92E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ImageListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImageListCtrl window

class CImageListCtrl : public CListCtrl
{
// Construction
public:
	CImageListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImageListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CImageListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CImageListCtrl)
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGELISTCTRL_H__EAFCDB8A_5A00_4F1E_966A_3036315B92E0__INCLUDED_)
