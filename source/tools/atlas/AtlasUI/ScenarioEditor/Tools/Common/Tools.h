#ifndef TOOLS_H__
#define TOOLS_H__

#include "ScenarioEditor/ScenarioEditor.h"
#include "general/AtlasWindowCommand.h"

class wxMouseEvent;
class wxKeyEvent;

class ITool
{
public:
	enum KeyEventType { KEY_DOWN, KEY_UP, KEY_CHAR };

	virtual void OnMouse(wxMouseEvent& evt) = 0;
	virtual void OnKey(wxKeyEvent& evt, KeyEventType dir) = 0;
	virtual void OnTick(float dt) = 0; // dt in seconds

	virtual ~ITool() {};
};

#define DECLARE_TOOL(name) ITool* CreateTool_##name() { return new name(); }


#define USE_TOOL(name) { extern ITool* CreateTool_##name(); SetCurrentTool(CreateTool_##name()); }

extern ITool* g_CurrentTool;
extern void SetCurrentTool(ITool*); // for internal use only

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

#define ADD_WORLDCOMMAND(type, data) ScenarioEditor::GetCommandProc().Submit(new WorldCommand(new AtlasMessage::m##type(AtlasMessage::d##type data)))

//////////////////////////////////////////////////////////////////////////

#define SET_STATE(s) obj->SetState(&obj->s)

template <typename T>
class StateDrivenTool : public ITool
{
protected:

	struct State
	{
		virtual void enter(T* WXUNUSED(obj)) {}
		virtual void leave(T* WXUNUSED(obj)) {}
		virtual void tick (T* WXUNUSED(obj), float WXUNUSED(dt)) {}

		virtual bool mouse(T* WXUNUSED(obj), wxMouseEvent& WXUNUSED(evt))
		{
			return false;
		}
		virtual bool key(T* WXUNUSED(obj), wxKeyEvent& WXUNUSED(evt), KeyEventType WXUNUSED(type))
		{
			return false;
		}
	};

	struct Disabled : public State
	{
	}
	Disabled;

	State* m_CurrentState;
	void SetState(State* state)
	{
		m_CurrentState->leave(static_cast<T*>(this));
			// this cast is safe as long as the class is used as in
			// "class Something : public StateDrivenTool<Something> { ... }"
		m_CurrentState = state;
		m_CurrentState->enter(static_cast<T*>(this));
	}
	StateDrivenTool()
		: m_CurrentState(&Disabled)
	{
	}

	~StateDrivenTool()
	{
		SetState(&Disabled);
	}

private:
	virtual void OnMouse(wxMouseEvent& evt)
	{
		m_CurrentState->mouse(static_cast<T*>(this), evt);
	}

	virtual void OnKey(wxKeyEvent& evt, KeyEventType dir)
	{
		m_CurrentState->key(static_cast<T*>(this), evt, dir);
	}

	virtual void OnTick(float dt)
	{
		m_CurrentState->tick(static_cast<T*>(this), dt);
	}
};


#endif // TOOLS_H__
