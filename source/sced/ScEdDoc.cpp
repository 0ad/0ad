// ScEdDoc.cpp : implementation of the CScEdDoc class
//

#include "stdafx.h"
#include "ScEd.h"

#include "ScEdDoc.h"

/////////////////////////////////////////////////////////////////////////////
// CScEdDoc

IMPLEMENT_DYNCREATE(CScEdDoc, CDocument)

BEGIN_MESSAGE_MAP(CScEdDoc, CDocument)
	//{{AFX_MSG_MAP(CScEdDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScEdDoc construction/destruction

CScEdDoc::CScEdDoc()
{
	// TODO: add one-time construction code here
	m_bAutoDelete = FALSE;
}

CScEdDoc::~CScEdDoc()
{
}

BOOL CScEdDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CScEdDoc serialization

void CScEdDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScEdDoc diagnostics

#ifdef _DEBUG
void CScEdDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CScEdDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScEdDoc commands
