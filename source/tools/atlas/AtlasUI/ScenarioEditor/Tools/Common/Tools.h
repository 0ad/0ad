#ifndef TOOLS_H__
#define TOOLS_H__

class wxMouseEvent;
class wxKeyEvent;

class ITool
{
public:
	enum { KEY_DOWN, KEY_UP, KEY_CHAR };

	virtual void OnMouse(wxMouseEvent& evt) = 0;
	virtual void OnKey(wxKeyEvent& evt, int dir) = 0;
	virtual void OnTick(float dt) = 0;

	virtual ~ITool() {};
};

#define DECLARE_TOOL(name) ITool* CreateTool_##name() { return new name(); }
#define USE_TOOL(name) { extern ITool* CreateTool_##name(); SetCurrentTool(CreateTool_##name()); }

extern ITool* g_CurrentTool;
extern void SetCurrentTool(ITool*); // for internal use only

#endif // TOOLS_H__
