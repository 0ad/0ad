// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__5804C3C3_DA2E_4546_9BE5_886FB4BEF254__INCLUDED_)
#define AFX_MAINFRM_H__5804C3C3_DA2E_4546_9BE5_886FB4BEF254__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MainFrameDlgBar.h"
#include "TexToolsDlgBar.h"
#include "ElevToolsDlgBar.h"
#include "UnitToolsDlgBar.h"
#include "UnitPropertiesDlgBar.h"
#include "BrushShapeEditorDlgBar.h"

class CObjectEntry;

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
	
// Operations
public:
	void OnUnitTools();
	void OnObjectProperties(CObjectEntry* obj);

	void DeselectTools();

	void SetTitle();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CMainFrameDlgBar m_wndDlgBar;
	CTexToolsDlgBar  m_wndTexToolsBar;
	CElevToolsDlgBar m_wndElevToolsBar;
	CUnitToolsDlgBar m_wndUnitToolsBar;
	CUnitPropertiesDlgBar m_wndUnitPropsBar;
	CBrushShapeEditorDlgBar m_wndBrushShapeEditorBar;

// Generated message map functions
protected:
	friend class CMainFrameDlgBar;
	friend class CScEdView;

	void DisableCtrlBars();
	void DisableToolbarButtons();

	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTerrainLoad();
	afx_msg void OnLightingSettings();
	afx_msg void OnViewScreenshot();
	afx_msg void OnTextureTools();
	afx_msg void OnToolsOptions();
	afx_msg void OnToolsBrushShapeEditor();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnElevationTools();
	afx_msg void OnResizeMap();
	afx_msg void OnViewTerrainGrid();
	afx_msg void OnUpdateViewTerrainGrid(CCmdUI* pCmdUI);
	afx_msg void OnViewTerrainSolid();
	afx_msg void OnUpdateViewTerrainSolid(CCmdUI* pCmdUI);
	afx_msg void OnViewTerrainWireframe();
	afx_msg void OnUpdateViewTerrainWireframe(CCmdUI* pCmdUI);
	afx_msg void OnViewModelGrid();
	afx_msg void OnUpdateViewModelGrid(CCmdUI* pCmdUI);
	afx_msg void OnViewModelSolid();
	afx_msg void OnUpdateViewModelSolid(CCmdUI* pCmdUI);
	afx_msg void OnViewModelWireframe();
	afx_msg void OnUpdateViewModelWireframe(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveMap();
	afx_msg void OnFileLoadMap();
	afx_msg void OnViewRenderStats();
	afx_msg void OnUpdateViewRenderStats(CCmdUI* pCmdUI);
	afx_msg LRESULT OnMouseWheel(WPARAM wParam,LPARAM lParam);
	afx_msg void OnTestGo();
	afx_msg void OnUpdateTestGo(CCmdUI* pCmdUI);
	afx_msg void OnTestStop();
	afx_msg void OnUpdateTestStop(CCmdUI* pCmdUI);
	afx_msg void OnRandomMap();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__5804C3C3_DA2E_4546_9BE5_886FB4BEF254__INCLUDED_)
