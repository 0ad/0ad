#if !defined(AFX_COLORBUTTON_H__DBA27321_CEB1_4D75_9225_AFF2B02FCD82__INCLUDED_)
#define AFX_COLORBUTTON_H__DBA27321_CEB1_4D75_9225_AFF2B02FCD82__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ColorButton.h : header file
//

#include "Color.h"

inline void ColorRefToRGBColor(COLORREF c,RGBColor& result)
{
	result.X=(c & 0xff)/255.0f;
	result.Y=((c>>8) & 0xff)/255.0f;
	result.Z=((c>>16) & 0xff)/255.0f;
}

inline void ColorRefToRGBAColor(COLORREF c,BYTE alpha,RGBAColor& result)
{
	result[0]=(c & 0xff)/255.0f;
	result[1]=((c>>8) & 0xff)/255.0f;
	result[2]=((c>>16) & 0xff)/255.0f;
	result[3]=alpha/255.0f;
}

inline void RGBColorToColorRef(const RGBColor& c,COLORREF& result)
{
	int r=int(c.X*255);
	if (r<0) r=0;
	if (r>255) r=255;
	
	int g=int(c.Y*255);
	if (g<0) g=0;
	if (g>255) g=255;

	int b=int(c.Z*255);
	if (b<0) b=0;
	if (b>255) b=255;

	result=r | (g<<8) | (b<<16);
}

inline void RGBAColorToColorRef(const RGBAColor& c,COLORREF& result)
{
	int r=int(c[0]*255);
	if (r<0) r=0;
	if (r>255) r=255;
	
	int g=int(c[1]*255);
	if (g<0) g=0;
	if (g>255) g=255;

	int b=int(c[2]*255);
	if (b<0) b=0;
	if (b>255) b=255;

	int a=int(c[3]*255);
	if (a<0) a=0;
	if (a>255) a=255;

	result=r | (g<<8) | (b<<16) | (a<<24);
}

/////////////////////////////////////////////////////////////////////////////
// CColorButton window

class CColorButton : public CButton
{
// Construction
public:
	CColorButton();

// Attributes
public:
	COLORREF m_Color;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorButton)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORBUTTON_H__DBA27321_CEB1_4D75_9225_AFF2B02FCD82__INCLUDED_)
