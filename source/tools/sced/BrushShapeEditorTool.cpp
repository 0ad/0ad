#include "precompiled.h"

#include <stdlib.h>
#include <string.h>
#include "ogl.h"
#include "Renderer.h"
#include "BrushShapeEditorTool.h"


// default tool instance
CBrushShapeEditorTool CBrushShapeEditorTool::m_BrushShapeEditorTool;


CBrushShapeEditorTool::CBrushShapeEditorTool() : m_BrushSize(5), m_BrushData(0)
{
	m_BrushData=new bool[m_BrushSize*m_BrushSize];
	memset(m_BrushData,0,sizeof(bool)*m_BrushSize*m_BrushSize);
}

void CBrushShapeEditorTool::OnDraw()
{
	g_Renderer.SetTexture(0,0);

	glDepthMask(0);

	// draw grid

	// draw state of each bit in the brush
	int r=m_BrushSize;
	// iterate through selected patches
	for (int j=0;j<r;j++) {
		for (int i=0;i<r;i++) {
			if (m_BrushData[j*m_BrushSize+i]) {
				// draw filled quad here
			}
		}
	}

	glDepthMask(1);
}
	
void CBrushShapeEditorTool::OnLButtonDown(unsigned int flags,int px,int py) 
{
	// check for bit under cursor
	if (0) {
		// switch if on
	}
}

void CBrushShapeEditorTool::OnRButtonDown(unsigned int flags,int px,int py) 
{
	// check for bit under cursor
	if (0) {
		// switch it off
	}
}

void CBrushShapeEditorTool::OnMouseMove(unsigned int flags,int px,int py) 
{
/*
	if (flags & TOOL_MOUSEFLAG_LBUTTONDOWN) {
		OnLButtonDown(flags,px,py);
	} else if (flags & TOOL_MOUSEFLAG_RBUTTONDOWN) {
		OnRButtonDown(flags,px,py);
	}
*/
}

void CBrushShapeEditorTool::SetBrushSize(int size)
{ 
	// allocate new data
	bool* newdata=new bool[size*size];
	memset(newdata,0,sizeof(bool)*size*size);

	// copy data from old to new
  	bool* src=m_BrushData;
	bool* dst=newdata;
	u32 copysize=size>m_BrushSize ? m_BrushSize : size;
	for (u32 j=0;j<copysize;j++) {
		memcpy(dst,src,copysize*sizeof(bool));
		dst+=copysize;
		src+=m_BrushSize;
	}

	// clean up and assign new data as current
	delete[] m_BrushData;
	m_BrushData=newdata;
}