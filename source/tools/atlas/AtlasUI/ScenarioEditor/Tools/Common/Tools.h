#ifndef TOOLS_H__
#define TOOLS_H__

#include "ScenarioEditor/ScenarioEditor.h"
#include "general/AtlasWindowCommand.h"

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

namespace AtlasMessage { struct mWorldCommand; }
class WorldCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(WorldCommand);

public:
	WorldCommand(AtlasMessage::mWorldCommand* command);
	~WorldCommand();
	bool Do();
	bool Undo();
	bool Redo();

private:
	AtlasMessage::mWorldCommand* m_Command;
};

#define ADD_WORLDCOMMAND(type, data) ScenarioEditor::GetCommandProc().Submit(new WorldCommand(new AtlasMessage::m##type(AtlasMessage::d##type data)))


#endif // TOOLS_H__
