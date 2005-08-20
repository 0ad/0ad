#include "stdafx.h"

#include "Tools.h"
#include "GameInterface/Messages.h"

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

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(WorldCommand, AtlasWindowCommand);

WorldCommand::WorldCommand(AtlasMessage::mWorldCommand* command)
: AtlasWindowCommand(true, _("???")), m_Command(command)
{
}

WorldCommand::~WorldCommand()
{
	delete m_Command;
}

bool WorldCommand::Do()
{
	ADD_COMMAND(DoCommand(m_Command));
	return true;
}

bool WorldCommand::Undo()
{
	ADD_COMMAND(UndoCommand());
	return true;
}

bool WorldCommand::Redo()
{
	ADD_COMMAND(RedoCommand());
	return true;
}
