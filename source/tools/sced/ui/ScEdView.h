// ScEdView.h : interface of the CScEdView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCEDVIEW_H__8E15B3D6_0CEA_4B52_95AC_64B15600ADE8__INCLUDED_)
#define AFX_SCEDVIEW_H__8E15B3D6_0CEA_4B52_95AC_64B15600ADE8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// forward declarations
class CScEdDoc;


class CScEdView : public CView
{
protected: // create from serialization only
	CScEdView();
	DECLARE_DYNCREATE(CScEdView)

// Attributes
public:
	CScEdDoc* GetDocument();
	void OnScreenShot();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScEdView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CScEdView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	void IdleTimeProcess();

	// current size of view
	int m_Width,m_Height;

protected:
	void AdjustCameraViaMinimapClick(CPoint point);
	bool AppHasFocus();

	// GL rendering context
	HGLRC m_hGLRC;     
	// last known position of mouse
	CPoint m_LastMousePos;
	// setup pixel format on given DC
	bool SetupPixelFormat(HDC dc);
	// last tick time
	double m_LastTickTime;
	// duration of last frame
	double m_LastFrameDuration;

	void OnMouseLeave(const CPoint& point);
	void OnMouseEnter(const CPoint& point);

// Generated message map functions
protected:
	//{{AFX_MSG(CScEdView)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseWheel(WPARAM wParam,LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ScEdView.cpp
inline CScEdDoc* CScEdView::GetDocument()
   { return (CScEdDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCEDVIEW_H__8E15B3D6_0CEA_4B52_95AC_64B15600ADE8__INCLUDED_)
