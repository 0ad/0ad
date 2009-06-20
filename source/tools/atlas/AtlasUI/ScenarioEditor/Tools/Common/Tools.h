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

#ifndef INCLUDED_TOOLS
#define INCLUDED_TOOLS

#include "General/AtlasWindowCommand.h"

class wxMouseEvent;
class wxKeyEvent;
class ScenarioEditor;

class ITool : public wxObject
{
public:
	enum KeyEventType { KEY_DOWN, KEY_UP, KEY_CHAR };

	virtual void Init(void* initData, ScenarioEditor* scenarioEditor) = 0;
	virtual void Shutdown() = 0;
	virtual bool OnMouse(wxMouseEvent& evt) = 0; // return true if handled
	virtual bool OnKey(wxKeyEvent& evt, KeyEventType dir) = 0; // return true if handled
	virtual void OnTick(float dt) = 0; // dt in seconds

	virtual ~ITool() {};
};

struct ToolManagerImpl;
class ToolManager
{
public:
	ToolManager(ScenarioEditor* scenarioEditor);
	~ToolManager();
	ITool& GetCurrentTool();
	void SetCurrentTool(const wxString& name, void* initData = NULL);
private:
	ToolManagerImpl* m;
	ScenarioEditor* m_ScenarioEditor;
};

class ToolButton;
extern void RegisterToolButton(ToolButton* button, const wxString& toolName);
extern void RegisterToolBarButton(wxToolBar* toolbar, int buttonId, const wxString& toolName);

//////////////////////////////////////////////////////////////////////////


namespace AtlasMessage { struct mWorldCommand; }
class WorldCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(WorldCommand);

	bool m_AlreadyDone;
public:
	WorldCommand(AtlasMessage::mWorldCommand* command);
	~WorldCommand();
	bool Do();
	bool Undo();
	bool Merge(AtlasWindowCommand* previousCommand);

private:
	AtlasMessage::mWorldCommand* m_Command;
};

#define POST_COMMAND(type, data) ScenarioEditor::GetCommandProc().Submit(new WorldCommand(new AtlasMessage::m##type(AtlasMessage::d##type data)))

//////////////////////////////////////////////////////////////////////////

#define SET_STATE(s) obj->SetState(&obj->s)

template <typename T>
class StateDrivenTool : public ITool
{
public:
	StateDrivenTool()
		: m_CurrentState(&Disabled), m_ScenarioEditor(NULL)
	{
	}

	virtual void Init(void* WXUNUSED(initData), ScenarioEditor* scenarioEditor)
	{
		m_ScenarioEditor = scenarioEditor;
	}

	virtual void Shutdown()
	{
		// This can't be done in the destructor, because ~StateDrivenTool
		// is not called until after the subclass has been destroyed and its
		// vtable (containing OnDisable) has been removed.
		SetState(&Disabled);
	}

protected:
	// Called when the tool is enabled/disabled; always called in zero or
	// more enable-->disable pairs per object instance.
	virtual void OnEnable() {}
	virtual void OnDisable() {}

	struct State
	{
		virtual void OnEnter(T* WXUNUSED(obj)) {}
		virtual void OnLeave(T* WXUNUSED(obj)) {}
		virtual void OnTick (T* WXUNUSED(obj), float WXUNUSED(dt)) {}

		// Should return true if the event has been handled (else the event will
		// be passed to a lower-priority level)
		virtual bool OnMouse(T* WXUNUSED(obj), wxMouseEvent& WXUNUSED(evt)) { return false; }
		virtual bool OnKey(T* WXUNUSED(obj), wxKeyEvent& WXUNUSED(evt), KeyEventType WXUNUSED(type)) { return false; }
	};


	struct sDisabled : public State
	{
		void OnEnter(T* obj) { obj->OnDisable(); }
		void OnLeave(T* obj) { obj->OnEnable(); }
	}
	Disabled;

	void SetState(State* state)
	{
		m_CurrentState->OnLeave(static_cast<T*>(this));
		// this cast is safe as long as the class is used as in
		// "class Something : public StateDrivenTool<Something> { ... }"
		m_CurrentState = state;
		m_CurrentState->OnEnter(static_cast<T*>(this));
	}

	ScenarioEditor& GetScenarioEditor() { wxASSERT(m_ScenarioEditor); return *m_ScenarioEditor; }

private:
	State* m_CurrentState;

	ScenarioEditor* m_ScenarioEditor; // not NULL, except before Init has been called

	virtual bool OnMouse(wxMouseEvent& evt)
	{
		return m_CurrentState->OnMouse(static_cast<T*>(this), evt);
	}

	virtual bool OnKey(wxKeyEvent& evt, KeyEventType dir)
	{
		return m_CurrentState->OnKey(static_cast<T*>(this), evt, dir);
	}

	virtual void OnTick(float dt)
	{
		m_CurrentState->OnTick(static_cast<T*>(this), dt);
	}
};


#endif // INCLUDED_TOOLS
