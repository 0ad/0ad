// SimpleEdit.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "SimpleEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CSimpleEdit dialog


CSimpleEdit::CSimpleEdit(const char* title,CWnd* pParent /*=NULL*/)
	: m_Title(title), CDialog(CSimpleEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSimpleEdit)
	m_Text = _T("");
	//}}AFX_DATA_INIT
}


void CSimpleEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimpleEdit)
	DDX_Text(pDX, IDC_EDIT1, m_Text);
	DDV_MaxChars(pDX, m_Text, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSimpleEdit, CDialog)
	//{{AFX_MSG_MAP(CSimpleEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimpleEdit message handlers

BOOL CSimpleEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWindowText((const char*) m_Title);
	
	CWnd* edit=GetDlgItem(IDC_EDIT1);
	if (edit) {
		edit->SetFocus();
		return FALSE;
	}

	return TRUE;  
}
