#if !defined(AFX_DIRECTIONBUTTON_H__C66DCA94_4F09_41D8_8CAA_C2AF5EDA38D8__INCLUDED_)
#define AFX_DIRECTIONBUTTON_H__C66DCA94_4F09_41D8_8CAA_C2AF5EDA38D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DirectionButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDirectionButton window

class CDirectionButton : public CButton
{
// Construction
public:
	CDirectionButton();

// Attributes
public:
	float m_Direction;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDirectionButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDirectionButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDirectionButton)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIRECTIONBUTTON_H__C66DCA94_4F09_41D8_8CAA_C2AF5EDA38D8__INCLUDED_)
