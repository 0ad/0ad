#if !defined(AFX_MAPSIZEDLG_H__F43F9B80_73CB_4DEB_8630_F0690664D510__INCLUDED_)
#define AFX_MAPSIZEDLG_H__F43F9B80_73CB_4DEB_8630_F0690664D510__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapSizeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapSizeDlg dialog

class CMapSizeDlg : public CDialog
{
// Construction
public:
	CMapSizeDlg(CWnd* pParent = NULL);   // standard constructor

	int m_MapSize;

// Dialog Data
	//{{AFX_DATA(CMapSizeDlg)
	enum { IDD = IDD_DIALOG_MAPSIZE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapSizeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapSizeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioHuge();
	afx_msg void OnRadioLarge();
	afx_msg void OnRadioMedium();
	afx_msg void OnRadioSmall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPSIZEDLG_H__F43F9B80_73CB_4DEB_8630_F0690664D510__INCLUDED_)
