// ImageListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "ScEd.h"
#include "ImageListCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CImageListCtrl

CImageListCtrl::CImageListCtrl()
{
}

CImageListCtrl::~CImageListCtrl()
{
}


BEGIN_MESSAGE_MAP(CImageListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CImageListCtrl)
	ON_WM_DRAWITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CImageListCtrl message handlers

void CImageListCtrl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{	
}
