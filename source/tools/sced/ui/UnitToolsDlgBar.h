#ifndef _UNITTOOLSDLGBAR_H
#define _UNITTOOLSDLGBAR_H

class CUnitToolsDlgBar : public CDialogBar
{
public:
	CUnitToolsDlgBar();
	~CUnitToolsDlgBar();

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	enum { SELECT_MODE, ADDUNIT_MODE } m_Mode;

	void SetSelectMode();
	void SetAddUnitMode();

protected:
	BOOL OnInitDialog();
	int GetCurrentObjectType();

	std::vector<CStr> m_ObjectNames[2];

	// Generated message map functions
	//{{AFX_MSG(CUnitToolsDlgBar)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonAddUnit();
	afx_msg void OnButtonSelect();
	afx_msg void OnClickListObjectBrowser(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void CUnitToolsDlgBar::OnSelChangeObjectTypes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
