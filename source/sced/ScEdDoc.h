// ScEdDoc.h : interface of the CScEdDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCEDDOC_H__6242BDFF_C79F_4830_8E45_7433106317DB__INCLUDED_)
#define AFX_SCEDDOC_H__6242BDFF_C79F_4830_8E45_7433106317DB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CScEdDoc : public CDocument
{
protected: // create from serialization only
	CScEdDoc();
	DECLARE_DYNCREATE(CScEdDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScEdDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CScEdDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CScEdDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCEDDOC_H__6242BDFF_C79F_4830_8E45_7433106317DB__INCLUDED_)
