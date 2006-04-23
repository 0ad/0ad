#include "precompiled.h"
#include "stdafx.h"
#include "MainFrm.h"
#include "SimpleEdit.h"
#include "ObjectManager.h"
#include "UnitToolsDlgBar.h"
#include "PaintObjectTool.h"
#include "SelectObjectTool.h"
#include "ToolManager.h"

#include "BaseEntityCollection.h"

#undef CRect // because it was redefined to PS_Rect in Overlay.h

BEGIN_MESSAGE_MAP(CUnitToolsDlgBar, CDialogBar)
	//{{AFX_MSG_MAP(CUnitToolsDlgBar)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_ADDUNIT, OnButtonAddUnit)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_NOTIFY(NM_CLICK, IDC_LIST_OBJECTBROWSER, OnClickListObjectBrowser)
	ON_CBN_SELCHANGE(IDC_COMBO_OBJECTTYPES, OnSelChangeObjectTypes)
END_MESSAGE_MAP()


CUnitToolsDlgBar::CUnitToolsDlgBar()
{
}

CUnitToolsDlgBar::~CUnitToolsDlgBar()
{
}

BOOL CUnitToolsDlgBar::Create(CWnd * pParentWnd, LPCTSTR lpszTemplateName,UINT nStyle, UINT nID)
{
	if (!CDialogBar::Create(pParentWnd, lpszTemplateName, nStyle, nID)) {
		return FALSE;
	}

	if (!OnInitDialog()) {
		return FALSE;
	}

	return TRUE;
}

BOOL CUnitToolsDlgBar::Create(CWnd * pParentWnd, UINT nIDTemplate,UINT nStyle, UINT nID)
{
	if (!Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID)) {
		return FALSE;
	}

	return TRUE;
}


BOOL CUnitToolsDlgBar::OnInitDialog()
{
	 // get the current window size and position
	CRect rect;
	GetWindowRect(rect);

	// now change the size, position, and Z order of the window.
	::SetWindowPos(m_hWnd,HWND_TOPMOST,10,rect.top,rect.Width(),rect.Height(),SWP_HIDEWINDOW);

	// setup the list ctrl
	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_OBJECTBROWSER);
	listctrl->GetClientRect(&rect);
	listctrl->InsertColumn(0,_T("Item Name"),LVCFMT_LEFT,rect.Width());
	

	// build combo box for object types
	CComboBox* objecttypes=(CComboBox*) GetDlgItem(IDC_COMBO_OBJECTTYPES);

	objecttypes->AddString("Entities");
	objecttypes->AddString("Actors");

	objecttypes->SetCurSel(0);

	std::vector<CStrW> names;
	g_EntityTemplateCollection.getBaseEntityNames(names);
	for (size_t i = 0; i < names.size(); ++i)
		m_ObjectNames[0].push_back((CStr)names[i]);

	g_ObjMan.GetAllObjectNames(m_ObjectNames[1]);


	CButton* addunit=(CButton*) GetDlgItem(IDC_BUTTON_ADDUNIT);
	addunit->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_ADDUNIT)));

	CButton* selectunit=(CButton*) GetDlgItem(IDC_BUTTON_SELECT);
	selectunit->SetBitmap(::LoadBitmap(::GetModuleHandle(0),MAKEINTRESOURCE(IDB_BITMAP_SELECTUNIT)));

	OnButtonSelect();

	OnSelChangeObjectTypes();

	return TRUE;  	              
}

void CUnitToolsDlgBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	bDisableIfNoHndler = FALSE;
	CDialogBar::OnUpdateCmdUI(pTarget,bDisableIfNoHndler);
}

void CUnitToolsDlgBar::OnButtonAdd()
{
//	bool foundname=false;
//	CSimpleEdit dlg("Enter Object Name");
//	while (!foundname && dlg.DoModal()==IDOK) {
//		// get object name
//		CString& name=dlg.m_Text;
//		if (name.GetLength()==0) {
//			MessageBox("Bad name","Error");
//		} else if (g_ObjMan.FindObject(name)!=0) {
//			MessageBox("Object with that name already exists","Error");
//		} else {
//			// add to list ctrl
//			CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_OBJECTBROWSER);
//			int index=listctrl->GetItemCount();
//			listctrl->InsertItem(index,(const char*) name,index);
//			
//			// deselect current selection in list ctrl, if any
//			POSITION pos=listctrl->GetFirstSelectedItemPosition();
//			if (pos) {
//				int oldindex=listctrl->GetNextSelectedItem(pos);
//				listctrl->SetItemState(oldindex, 0, LVIS_SELECTED);
//			}
//
//			// select new entry
//			listctrl->SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
//
//			// now enter edit mode
//			CObjectEntry* obj=new CObjectEntry(GetCurrentObjectType());
//			obj->m_Name=(const char*)name;
//			g_ObjMan.AddObject(obj,GetCurrentObjectType());
//
//			CMainFrame* mainfrm=(CMainFrame*) AfxGetMainWnd();
//			mainfrm->OnObjectProperties(obj);
//			foundname=true;
//		}
//	}

}
 
void CUnitToolsDlgBar::OnButtonEdit()
{
//	// get current selection, if any
//	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_OBJECTBROWSER);
//	POSITION pos=listctrl->GetFirstSelectedItemPosition();
//	if (!pos) {
//		// nothing selected, nothing to do
//		return;
//	}
//	
//	// get object at position
//	const std::vector<CObjectEntry*>& objects=g_ObjMan.m_ObjectTypes[GetCurrentObjectType()].m_Objects;
//	CObjectEntry* obj=objects[(intptr_t)pos-1];
//	CMainFrame* mainfrm=(CMainFrame*) AfxGetMainWnd();
//	mainfrm->OnObjectProperties(obj);
}

	
void CUnitToolsDlgBar::OnClickListObjectBrowser(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_OBJECTBROWSER);

	// deselect current selection in list ctrl, if any
	POSITION pos=listctrl->GetFirstSelectedItemPosition();
	if (pos) {
		CStrW name (CStr(listctrl->GetItemText((int)(intptr_t)pos-1, 0)));
		if (GetCurrentObjectType() == 0)
			g_ObjMan.SetSelectedEntity(g_EntityTemplateCollection.getTemplate(name));
		else
			g_ObjMan.SetSelectedObject(g_ObjMan.FindObject((CStr)name));
	}

	// shift to add mode
	SetAddUnitMode();

	*pResult = 0;
}

int CUnitToolsDlgBar::GetCurrentObjectType()
{
	CComboBox* objecttypes=(CComboBox*) GetDlgItem(IDC_COMBO_OBJECTTYPES);
	return objecttypes->GetCurSel();
}

void CUnitToolsDlgBar::OnSelChangeObjectTypes()
{
	// clear out the listctrl
	CListCtrl* listctrl=(CListCtrl*) GetDlgItem(IDC_LIST_OBJECTBROWSER);
	listctrl->DeleteAllItems();
	
	// add new items back to listbox
	std::vector<CStr>& names=m_ObjectNames[GetCurrentObjectType()];
	for (uint i=0;i<names.size();i++) {
		// add to list ctrl
		listctrl->InsertItem(i, names[i], i);
	}

	g_ObjMan.SetSelectedObject(0);
}

void CUnitToolsDlgBar::OnButtonSelect()
{
	SetSelectMode();
}

void CUnitToolsDlgBar::SetSelectMode()
{
	((CButton*) GetDlgItem(IDC_BUTTON_SELECT))->SetState(TRUE);
	((CButton*) GetDlgItem(IDC_BUTTON_ADDUNIT))->SetState(FALSE);
	m_Mode=SELECT_MODE;
	g_ToolMan.SetActiveTool(CSelectObjectTool::GetTool());
}

void CUnitToolsDlgBar::OnButtonAddUnit()
{
	SetAddUnitMode();
}

void CUnitToolsDlgBar::SetAddUnitMode()
{
	((CButton*) GetDlgItem(IDC_BUTTON_SELECT))->SetState(FALSE);
	((CButton*) GetDlgItem(IDC_BUTTON_ADDUNIT))->SetState(TRUE);
	m_Mode=ADDUNIT_MODE;
	g_ToolMan.SetActiveTool(CPaintObjectTool::GetTool());
}