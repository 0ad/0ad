#ifndef _PAINTTEXTURETOOL_H
#define _PAINTTEXTURETOOL_H

#include <set>
#include "res/res.h"
#include "BrushTool.h"

class CTextureEntry;

class CPaintTextureTool : public CBrushTool
{
public:
	enum { MAX_BRUSH_SIZE=8 };

public:
	CPaintTextureTool();

	// tool triggered via left mouse button; paint current selection
	void OnTriggerLeft() { PaintSelection(); }

	// set current painting texture
	void SetSelectedTexture(CTextureEntry* tex) { m_SelectedTexture=tex; }
	// get current painting texture
	CTextureEntry* GetSelectedTexture() { return m_SelectedTexture; }

	// allow multiple triggers by click and drag
	bool SupportDragTrigger() { return true; }

	// get the default paint texture instance
	static CPaintTextureTool* GetTool() { return &m_PaintTextureTool; }

private:
	// apply current texture to current selection
	void PaintSelection();

	// currently selected texture for painting
	CTextureEntry* m_SelectedTexture;

	// default tool instance
	static CPaintTextureTool m_PaintTextureTool;
};

#endif
