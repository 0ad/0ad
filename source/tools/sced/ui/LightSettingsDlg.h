#if !defined(AFX_LIGHTSETTINGSDLG_H__B2A613AF_961F_4365_90A3_82DF9F713401__INCLUDED_)
#define AFX_LIGHTSETTINGSDLG_H__B2A613AF_961F_4365_90A3_82DF9F713401__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LightSettingsDlg.h : header file
//

#include "ColorButton.h"
#include "DirectionButton.h"
#include "ElevationButton.h"

/////////////////////////////////////////////////////////////////////////////
// CLightSettingsDlg dialog

class CLightSettingsDlg : public CDialog
{
// Construction
public:
	CLightSettingsDlg(CWnd* pParent = NULL);   // standard constructor

	bool m_PreviousPreview;
// Dialog Data

	//{{AFX_DATA(CLightSettingsDlg)
	enum { IDD = IDD_DIALOG_LIGHTSETTINGS };
	CColorButton	m_UnitsAmbientColor;
	CDirectionButton	m_DirectionButton;
	CElevationButton	m_ElevationButton;
	CColorButton	m_TerrainAmbientColor;
	CColorButton	m_SunColor;
	int		m_Direction;
	int		m_Elevation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLightSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
	// Generated message map functions
	//{{AFX_MSG(CLightSettingsDlg)
	afx_msg void OnButtonApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnDeltaposSpinDirection(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditDirection();
	afx_msg void OnButtonTerrainambientcolor();
	afx_msg void OnDeltaposSpinElevation(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditElevation();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LIGHTSETTINGSDLG_H__B2A613AF_961F_4365_90A3_82DF9F713401__INCLUDED_)
