#ifndef TOOLS_H__
#define TOOLS_H__

#include "ScenarioEditor/ScenarioEditor.h"
#include "General/AtlasWindowCommand.h"

class wxMouseEvent;
class wxKeyEvent;

class ITool : public wxObject
{
public:
	enum KeyEventType { KEY_DOWN, KEY_UP, KEY_CHAR };

	virtual void Init(void* initData) = 0;
	virtual void Shutdown() = 0;
	virtual bool OnMouse(wxMouseEvent& evt) = 0; // return true if handled
	virtual bool OnKey(wxKeyEvent& evt, KeyEventType dir) = 0; // return true if handled
	virtual void OnTick(float dt) = 0; // dt in seconds

	virtual ~ITool() {};
};

extern ITool& GetCurrentTool();
extern void SetCurrentTool(const wxString& name, void* initData = NULL);

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
		: m_CurrentState(&Disabled)
	{
	}

	virtual void Init(void* WXUNUSED(initData))
	{
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

private:
	State* m_CurrentState;

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


#endif // TOOLS_H__
