#ifndef _ELEVTOOLSDLGBAR_H
#define _ELEVTOOLSDLGBAR_H

class CElevToolsDlgBar : public CDialogBar
{
public:
	enum Mode { RAISELOWER_MODE, SMOOTH_MODE };

	CElevToolsDlgBar();
	~CElevToolsDlgBar();

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	void OnShow();

protected:
	BOOL OnInitDialog();

	void SetRaiseControls();
	void SetSmoothControls();

	// current operating mode
	Mode m_Mode;

	// Generated message map functions
	//{{AFX_MSG(CElevToolsDlgBar)
	afx_msg void OnRadioRaise();
	afx_msg void OnRadioSmooth();
	afx_msg void OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedCaptureSliderBrushEffect(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
