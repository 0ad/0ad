#if !defined(AFX_UNITPROPERTIESTABCTRL_H__65018BF5_3893_4430_AA1D_743F79904748__INCLUDED_)
#define AFX_UNITPROPERTIESTABCTRL_H__65018BF5_3893_4430_AA1D_743F79904748__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UnitPropertiesTabCtrl.h : header file
//

#include "UnitPropertiesTexturesTab.h"
#include "UnitPropertiesAnimationsTab.h"

/////////////////////////////////////////////////////////////////////////////
// CUnitPropertiesTabCtrl window

class CUnitPropertiesTabCtrl : public CTabCtrl
{
// Construction
public:
	CUnitPropertiesTabCtrl();

	CDialog *m_tabPages[2];
	int m_tabCurrent;
	int m_nNumberOfPages;

	void Init();

// Attributes
public:
	CUnitPropertiesTexturesTab* m_TexturesTab;
	CUnitPropertiesAnimationsTab* m_AnimationsTab;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUnitPropertiesTabCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUnitPropertiesTabCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CUnitPropertiesTabCtrl)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UNITPROPERTIESTABCTRL_H__65018BF5_3893_4430_AA1D_743F79904748__INCLUDED_)
