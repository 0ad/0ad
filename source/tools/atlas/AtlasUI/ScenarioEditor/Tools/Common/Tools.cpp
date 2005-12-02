#include "stdafx.h"

#include "Tools.h"
#include "GameInterface/Messages.h"

class DummyTool : public ITool
{
	void Init(void*) {}
	void Shutdown() {}
	bool OnMouse(wxMouseEvent& WXUNUSED(evt)) { return false; }
	bool OnKey(wxKeyEvent& WXUNUSED(evt), KeyEventType) { return false; }
	void OnTick(float) {}
} dummy;

static ITool* g_CurrentTool = &dummy;

ITool& GetCurrentTool()
{
	return *g_CurrentTool;
}

void SetCurrentTool(const wxString& name, void* initData)
{
	if (g_CurrentTool != &dummy)
	{
		g_CurrentTool->Shutdown();
		delete g_CurrentTool;
	}

	ITool* tool = NULL;
	if (name.Len())
	{
		tool = wxDynamicCast(wxCreateDynamicObject(name), ITool);
		wxASSERT(tool);
		tool->Init(initData);
	}

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
		POST_MESSAGE(RedoCommand());
	else
	{
		POST_MESSAGE(DoCommand(m_Command));
		m_AlreadyDone = true;
	}
	return true;
}

bool WorldCommand::Undo()
{
	POST_MESSAGE(UndoCommand());
	return true;
}

bool WorldCommand::Merge(AtlasWindowCommand* p)
{
	WorldCommand* prev = wxDynamicCast(p, WorldCommand);

	if (! prev)
		return false;

	if (m_Command->GetName() != prev->m_Command->GetName()) // comparing char* pointers, because they're unique-per-class constants
		return false;

	if (! m_Command->IsMergeable())
		return false;

	POST_MESSAGE(MergeCommand());
	return true;
}
