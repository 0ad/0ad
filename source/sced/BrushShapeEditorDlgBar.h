#ifndef _BRUSHSHAPEEDITORDLGBAR_H
#define _BRUSHSHAPEEDITORDLGBAR_H

class CBrushShapeEditorDlgBar : public CDialogBar
{
//	DECLARE_DYNAMIC(CInitDialogBar)
public:
	CBrushShapeEditorDlgBar();
	~CBrushShapeEditorDlgBar();

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

protected:

	BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CBrushShapeEditorDlgBar)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonBack();
	afx_msg void OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangeBrush();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif 

