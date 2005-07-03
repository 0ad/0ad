#include "stdafx.h"

#include "Tools.h"

class DummyTool : public ITool
{
	void OnMouse(wxMouseEvent& evt) { evt.Skip(); }
	void OnKey(wxKeyEvent& evt, int) { evt.Skip(); }
	void OnTick(float) {}
} dummy;

ITool* g_CurrentTool = &dummy;

void SetCurrentTool(ITool* tool)
{
	if (g_CurrentTool != &dummy)
		delete g_CurrentTool;

	if (tool == NULL)
		g_CurrentTool = &dummy;
	else
		g_CurrentTool = tool;
}
