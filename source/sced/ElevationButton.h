#if !defined(AFX_ELEVATIONBUTTON_H__7CD052F1_193A_46B3_9EC6_81A52F9A2399__INCLUDED_)
#define AFX_ELEVATIONBUTTON_H__7CD052F1_193A_46B3_9EC6_81A52F9A2399__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ElevationButton.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CElevationButton window

class CElevationButton : public CButton
{
// Construction
public:
	CElevationButton();

// Attributes
public:
	float m_Elevation;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CElevationButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CElevationButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CElevationButton)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ELEVATIONBUTTON_H__7CD052F1_193A_46B3_9EC6_81A52F9A2399__INCLUDED_)
