#ifndef _BRUSHSHAPEEDITORTOOL_H
#define _BRUSHSHAPEEDITORTOOL_H

#include "res/res.h"
#include "Tool.h"

class CPatch;
class CMiniPatch;

class CBrushShapeEditorTool : public CTool
{
public:
	enum { MAX_BRUSH_SIZE=8 };

public:
	CBrushShapeEditorTool();

	// draw the visual representation of this tool
	void OnDraw();
	// callback for left button down event 
	void OnLButtonDown(unsigned int flags,int px,int py);
	// callback for right button down event 
	void OnRButtonDown(unsigned int flags,int px,int py);
	// callback for mouse move event 
	void OnMouseMove(unsigned int flags,int px,int py);

	// set current brush size
	void SetBrushSize(int size);
	// get current brush size
	int GetBrushSize() { return m_BrushSize; }

	// get the default brush shape editor instance
	static CBrushShapeEditorTool* GetTool() { return &m_BrushShapeEditorTool; }

protected:
	// current tool brush size
	int m_BrushSize;
	// on/off state of each bit in the brush
	bool* m_BrushData;
	// default tool instance
	static CBrushShapeEditorTool m_BrushShapeEditorTool;
};

#endif
