#include "stdafx.h"

#include "Tools.h"
#include "GameInterface/Messages.h"

class DummyTool : public ITool
{
	void OnMouse(wxMouseEvent& evt) { evt.Skip(); }
	void OnKey(wxKeyEvent& evt, KeyEventType) { evt.Skip(); }
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
: AtlasWindowCommand(true, wxString::FromAscii(command->GetType())), m_Command(command), m_AlreadyDone(false)
{
}

WorldCommand::~WorldCommand()
{
	delete m_Command;
}

bool WorldCommand::Do()
{
	if (m_AlreadyDone)
		POST_COMMAND(RedoCommand());
	else
	{
		POST_COMMAND(DoCommand(m_Command));
		m_AlreadyDone = true;
	}
	return true;
}

bool WorldCommand::Undo()
{
	POST_COMMAND(UndoCommand());
	return true;
}

bool WorldCommand::Merge(AtlasWindowCommand* p)
{
	WorldCommand* prev = wxDynamicCast(p, WorldCommand);

	if (! prev)
		return false;

	if (m_Command->GetType() != prev->m_Command->GetType()) // comparing char* pointers, because they're unique-per-class constants
		return false;

	if (! m_Command->IsMergeable())
		return false;

	POST_COMMAND(MergeCommand());
	return true;
}
