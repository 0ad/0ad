#if !defined(AFX_MAINFRAMEDLGBAR_H__B1EBAF6A_9AB1_4797_B859_3D47D8B011E0__INCLUDED_)
#define AFX_MAINFRAMEDLGBAR_H__B1EBAF6A_9AB1_4797_B859_3D47D8B011E0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainFrameDlgBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMainFrameDlgBar dialog

class CMainFrameDlgBar : public CDialogBar
{
// Construction
public:
	CMainFrameDlgBar();   // standard constructor

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

// Dialog Data
	//{{AFX_DATA(CMainFrameDlgBar)
	enum { IDD = IDR_MAINFRAME };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrameDlgBar)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CMainFrameDlgBar)
	afx_msg void OnButtonSelect();
	afx_msg void OnButtonTextureTools(); 
	afx_msg void OnButtonElevationTools();
	afx_msg void OnButtonModelTools(); 
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRAMEDLGBAR_H__B1EBAF6A_9AB1_4797_B859_3D47D8B011E0__INCLUDED_)
