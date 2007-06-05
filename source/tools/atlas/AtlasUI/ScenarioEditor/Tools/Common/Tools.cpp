#include "precompiled.h"

#include "Tools.h"
#include "GameInterface/Messages.h"
#include "CustomControls/Buttons/ToolButton.h"

class DummyTool : public ITool
{
	void Init(void*) {}
	void Shutdown() {}
	bool OnMouse(wxMouseEvent& WXUNUSED(evt)) { return false; }
	bool OnKey(wxKeyEvent& WXUNUSED(evt), KeyEventType) { return false; }
	void OnTick(float) {}
} dummy;

static ITool* g_CurrentTool = &dummy;
static wxString g_CurrentToolName;

void SetActive(bool active, const wxString& name);

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
		g_CurrentTool = &dummy;
	}

	SetActive(false, g_CurrentToolName);

	ITool* tool = NULL;
	if (name.Len())
	{
		tool = wxDynamicCast(wxCreateDynamicObject(name), ITool);
		wxASSERT(tool);
	}

	if (tool)
	{
		g_CurrentTool = tool;
		tool->Init(initData);
	}

	g_CurrentToolName = name;
	SetActive(true, g_CurrentToolName);
}

//////////////////////////////////////////////////////////////////////////

struct toolbarButton
{
	wxString name;
	wxToolBar* toolbar;
	int id;
};
struct toolButton
{
	wxString name;
	ToolButton* button;
};

typedef std::vector<toolbarButton> toolbarButtons_t;
typedef std::vector<toolButton> toolButtons_t;

static toolbarButtons_t toolbarButtons;
static toolButtons_t toolButtons;

void SetActive(bool active, const wxString& name)
{
	for (toolbarButtons_t::iterator it = toolbarButtons.begin(); it != toolbarButtons.end(); ++it)
		if (it->name == name)
			it->toolbar->ToggleTool(it->id, active);

	for (toolButtons_t::iterator it = toolButtons.begin(); it != toolButtons.end(); ++it)
		if (it->name == name)
			it->button->SetSelectedAppearance(active);
}

void RegisterToolButton(ToolButton* button, const wxString& toolName)
{
	toolButton b;
	b.name = toolName;
	b.button = button;
	toolButtons.push_back(b);
}

void RegisterToolBarButton(wxToolBar* toolbar, int buttonId, const wxString& toolName)
{
	toolbarButton b;
	b.name = toolName;
	b.toolbar = toolbar;
	b.id = buttonId;
	toolbarButtons.push_back(b);
}


//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(WorldCommand, AtlasWindowCommand);

WorldCommand::WorldCommand(AtlasMessage::mWorldCommand* command)
: AtlasWindowCommand(true, wxString::FromAscii(command->GetType())), m_Command(command), m_AlreadyDone(false)
{
}

WorldCommand::~WorldCommand()
{
	// m_Command was allocated by POST_COMMAND
	delete m_Command;
}

bool WorldCommand::Do()
{
	if (m_AlreadyDone)
		POST_MESSAGE(RedoCommand, ());
	else
	{
		// The DoCommand message clones the data from m_Command, and posts that
		// (passing ownership to the game), so we're free to delete m_Command
		// at any time
		POST_MESSAGE(DoCommand, (m_Command));
		m_AlreadyDone = true;
	}
	return true;
}

bool WorldCommand::Undo()
{
	POST_MESSAGE(UndoCommand, ());
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

	POST_MESSAGE(MergeCommand, ());
	return true;
}
