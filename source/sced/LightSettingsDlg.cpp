// LightSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "LightSettingsDlg.h"

#include <assert.h>

#include "Terrain.h"
#include "LightEnv.h"

#include "CommandManager.h"
#include "AlterLightEnvCommand.h"


extern CTerrain g_Terrain;


/////////////////////////////////////////////////////////////////////////////
// CLightSettingsDlg dialog


CLightSettingsDlg::CLightSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLightSettingsDlg::IDD, pParent), m_PreviousPreview(false)
{
	//{{AFX_DATA_INIT(CLightSettingsDlg)
	m_Direction = 270;
	m_Elevation = 45;
	//}}AFX_DATA_INIT
}


void CLightSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLightSettingsDlg)
	DDX_Control(pDX, IDC_BUTTON_UNITSAMBIENTCOLOR, m_UnitsAmbientColor);
	DDX_Control(pDX, IDC_BUTTON_DIRECTION, m_DirectionButton);
	DDX_Control(pDX, IDC_BUTTON_ELEVATION, m_ElevationButton);
	DDX_Control(pDX, IDC_BUTTON_TERRAINAMBIENTCOLOR, m_TerrainAmbientColor);
	DDX_Control(pDX, IDC_BUTTON_SUNCOLOR, m_SunColor);
	DDX_Text(pDX, IDC_EDIT_DIRECTION, m_Direction);
	DDX_Text(pDX, IDC_EDIT_ELEVATION, m_Elevation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLightSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CLightSettingsDlg)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, OnButtonApply)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_DIRECTION, OnDeltaposSpinDirection)
	ON_EN_CHANGE(IDC_EDIT_DIRECTION, OnChangeEditDirection)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ELEVATION, OnDeltaposSpinElevation)
	ON_EN_CHANGE(IDC_EDIT_ELEVATION, OnChangeEditElevation)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLightSettingsDlg message handlers


void CLightSettingsDlg::OnButtonApply() 
{	
	UpdateData(TRUE);

	// have we previously applied a lightenv?
	if (m_PreviousPreview) {
		// yes - undo it
		g_CmdMan.Undo();
	} 

	// build a lighting environment from the parameters
	CLightEnv env;
	env.m_Elevation=DEGTORAD(m_ElevationButton.m_Elevation);
	env.m_Rotation=DEGTORAD(m_DirectionButton.m_Direction);
	ColorRefToRGBColor(m_SunColor.m_Color,env.m_SunColor);
	ColorRefToRGBColor(m_TerrainAmbientColor.m_Color,env.m_TerrainAmbientColor);
	ColorRefToRGBColor(m_TerrainAmbientColor.m_Color,env.m_UnitsAmbientColor);

	// create and execute an AlterLightEnv command
	CAlterLightEnvCommand* cmd=new CAlterLightEnvCommand(env);
	g_CmdMan.Execute(cmd);
	
	AfxGetMainWnd()->Invalidate();
	AfxGetMainWnd()->UpdateWindow();

	m_PreviousPreview=true;
}

BOOL CLightSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CSpinButtonCtrl* dirspin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_DIRECTION);
	assert(dirspin);
	dirspin->SetRange32(0,359);
	dirspin->SetPos(m_Direction);
	m_DirectionButton.m_Direction=float(m_Direction);

	CSpinButtonCtrl* espin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_ELEVATION);
	assert(espin);
	espin->SetRange32(0,90);
	espin->SetPos(m_Elevation);
	m_ElevationButton.m_Elevation=float(m_Elevation);

	return TRUE;  	              
}

void CLightSettingsDlg::OnDeltaposSpinDirection(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	// update direction button
	if (!UpdateData(TRUE)) return;
	CSpinButtonCtrl* dirspin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_DIRECTION);
	m_Direction=dirspin->GetPos()+pNMUpDown->iDelta;
	m_DirectionButton.m_Direction=float(m_Direction);
	m_DirectionButton.Invalidate();
	m_DirectionButton.UpdateWindow();

	UpdateData(FALSE);

	*pResult = 0;
}

void CLightSettingsDlg::OnChangeEditDirection() 
{
	if (IsWindow(m_DirectionButton.m_hWnd)) {
		if (!UpdateData(TRUE)) return;
		m_DirectionButton.m_Direction=float(m_Direction);
		m_DirectionButton.Invalidate();
		m_DirectionButton.UpdateWindow();	

		CSpinButtonCtrl* dirspin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_DIRECTION);
		if (dirspin) dirspin->SetPos(m_Direction);
	}
}

void CLightSettingsDlg::OnDeltaposSpinElevation(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	// update elevation button
	if (!UpdateData(TRUE)) return;
	CSpinButtonCtrl* espin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_ELEVATION);
	m_Elevation=espin->GetPos()+pNMUpDown->iDelta;
	m_ElevationButton.m_Elevation=float(m_Elevation);
	m_ElevationButton.Invalidate();
	m_ElevationButton.UpdateWindow();

	UpdateData(FALSE);
	
	*pResult = 0;
}

void CLightSettingsDlg::OnChangeEditElevation() 
{
	if (IsWindow(m_ElevationButton.m_hWnd)) {
		if (!UpdateData(TRUE)) return;
		
		m_ElevationButton.m_Elevation=float(m_Elevation);
		m_ElevationButton.Invalidate();
		m_ElevationButton.UpdateWindow();	

		CSpinButtonCtrl* espin=(CSpinButtonCtrl*) GetDlgItem(IDC_SPIN_ELEVATION);
		if (espin) espin->SetPos(m_Elevation);
	}
}

