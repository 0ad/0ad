#ifndef _TEXTOOLSDLGBAR_H
#define _TEXTOOLSDLGBAR_H

#include "TextureEntry.h"

class CTexToolsDlgBar : public CDialogBar
{
//	DECLARE_DYNAMIC(CInitDialogBar)
public:
	CTexToolsDlgBar();
	~CTexToolsDlgBar();

	BOOL Create(CWnd * pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID);
	BOOL Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName, UINT nStyle, UINT nID);
	void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

protected:
	CImageList m_ImageList;

	CTerrainTypeGroup *GetCurrentTerrainType();

	void Select(CTextureEntry* entry);
	BOOL BuildImageListIcon(CTextureEntry* texentry);
	BOOL AddImageListIcon(CTextureEntry* entry);
	BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CTexToolsDlgBar)
	afx_msg void OnClickListTextureBrowser(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnReleasedCaptureSliderBrushSize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangeTerrainTypes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif 

