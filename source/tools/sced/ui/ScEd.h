// ScEd.h : main header file for the SCED application
//

#if !defined(AFX_SCED_H__7CE13472_02D4_450E_8F31_B08FBE733ED6__INCLUDED_)
#define AFX_SCED_H__7CE13472_02D4_450E_8F31_B08FBE733ED6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CScEdApp:
// See ScEd.cpp for the implementation of this class
//

class CScEdApp : public CWinApp
{
public:
	CScEdApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScEdApp)
	public:
	virtual BOOL InitInstance();
	virtual int Run();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CScEdApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCED_H__7CE13472_02D4_450E_8F31_B08FBE733ED6__INCLUDED_)
