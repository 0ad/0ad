#ifndef _UNITPROPERTIESDLGBAR_H
#define _UNITPROPERTIESDLGBAR_H

#include "ObjectEntry.h"

class CUnitPropertiesDlgBar : public CDialogBar
{
public:
	CUnitPropertiesDlgBar();
	~CUnitPropertiesDlgBar();

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	void SetObject(CObjectEntry* obj);
	

protected:
	BOOL OnInitDialog();
	
	void UpdateEditorData();
	void UpdatePropertiesDlg();

	// object being edited
	CObjectEntry* m_Object;

	// Generated message map functions
	//{{AFX_MSG(CUnitPropertiesDlgBar)
	afx_msg void OnButtonBack();
	afx_msg void OnButtonRefresh();
	afx_msg void OnButtonTextureBrowse();
	afx_msg void OnButtonModelBrowse();
	afx_msg void OnButtonAnimationBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif
