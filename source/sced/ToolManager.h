#ifndef _TOOLMANAGER_H
#define _TOOLMANAGER_H

#include "Tool.h"

class CToolManager
{
public:
	CToolManager() : m_ActiveTool(0) {}

	// draw the visual representation of active tool, if any
	void OnDraw() {
		if (m_ActiveTool) m_ActiveTool->OnDraw();
	}

	// callback for left button down event 
	void OnLButtonDown(unsigned int flags,int px,int py) {
		if (m_ActiveTool) m_ActiveTool->OnLButtonDown(flags,px,py);
	}

	// callback for left button up event 
	void OnLButtonUp(unsigned int flags,int px,int py) {
		if (m_ActiveTool) m_ActiveTool->OnLButtonUp(flags,px,py);
	}

	// callback for right button down event 
	void OnRButtonDown(unsigned int flags,int px,int py) {
		if (m_ActiveTool) m_ActiveTool->OnRButtonDown(flags,px,py);
	}

	// callback for right button up event 
	void OnRButtonUp(unsigned int flags,int px,int py) {
		if (m_ActiveTool) m_ActiveTool->OnRButtonUp(flags,px,py);
	}

	// callback for mouse move event 
	void OnMouseMove(unsigned int flags,int px,int py) {
		if (m_ActiveTool) m_ActiveTool->OnMouseMove(flags,px,py);
	}

	// set currently active tool, or pass 0 to deactive all tools
	void SetActiveTool(CTool* tool) { m_ActiveTool=tool; }	
	// get currently active tool, if any
	CTool* GetActiveTool() const { return m_ActiveTool; }

private:
	CTool* m_ActiveTool;
};

extern CToolManager g_ToolMan;

#endif
