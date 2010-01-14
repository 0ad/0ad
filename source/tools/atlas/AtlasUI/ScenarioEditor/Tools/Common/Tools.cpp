/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "Tools.h"
#include "GameInterface/Messages.h"
#include "CustomControls/Buttons/ToolButton.h"

class DummyTool : public ITool
{
	void Init(void*, ScenarioEditor*) {}
	void Shutdown() {}
	bool OnMouse(wxMouseEvent& WXUNUSED(evt)) { return false; }
	bool OnKey(wxKeyEvent& WXUNUSED(evt), KeyEventType) { return false; }
	void OnTick(float) {}
} dummy;

struct ToolManagerImpl
{
	ToolManagerImpl() : CurrentTool(&dummy) {}

	ITool* CurrentTool;
	wxString CurrentToolName;

};

ToolManager::ToolManager(ScenarioEditor* scenarioEditor)
	: m(new ToolManagerImpl), m_ScenarioEditor(scenarioEditor)
{
}

ToolManager::~ToolManager()
{
	delete m;
}

ITool& ToolManager::GetCurrentTool()
{
	return *m->CurrentTool;
}

void SetActive(bool active, const wxString& name);

void ToolManager::SetCurrentTool(const wxString& name, void* initData)
{
	if (m->CurrentTool != &dummy)
	{
		m->CurrentTool->Shutdown();
		delete m->CurrentTool;
		m->CurrentTool = &dummy;
	}

	SetActive(false, m->CurrentToolName);

	ITool* tool = NULL;
	if (name.Len())
	{
		tool = wxDynamicCast(wxCreateDynamicObject(name), ITool);
		wxASSERT(tool);
	}

	if (tool)
	{
		m->CurrentTool = tool;
		tool->Init(initData, m_ScenarioEditor);
	}

	m->CurrentToolName = name;
	SetActive(true, m->CurrentToolName);
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
: AtlasWindowCommand(true, wxString::FromAscii(command->GetName())), m_Command(command), m_AlreadyDone(false)
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
