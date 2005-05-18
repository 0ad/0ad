// GameEvents.h

// A class that exists to let scripts know when important things happen
// in the game.

#ifndef GAME_EVENTS_INCLUDED
#define GAME_EVENTS_INCLUDED

#include "DOMEvent.h"

class CGameEvents : public IEventTarget, public Singleton<CGameEvents>
{
	// Game events don't really run on an object
	JSObject* GetScriptExecContext( IEventTarget* target ) { return( g_ScriptingHost.GetGlobalObject() ); }

	// Some events
	class CEventSelectionChanged : public CScriptEvent
	{
		bool m_CausedByPlayer;
	public:
		CEventSelectionChanged(bool CausedByPlayer):
			CScriptEvent( L"selectionChanged", EVENT_SELECTION_CHANGED, false ),
			m_CausedByPlayer(CausedByPlayer)
		{
			AddLocalProperty( L"byPlayer", &m_CausedByPlayer, true );
		}
	};
	
	class CEventWorldClick: public CScriptEvent
	{
		int m_Button;
		int m_Clicks;
		int m_Command;
		int m_SecondaryCommand;
		CEntity *m_Entity;
		uint m_X, m_Y;
	public:
		CEventWorldClick(int button, int clicks, int command, int secCommand, CEntity *ent, uint x, uint y):
			CScriptEvent(L"worldClick", EVENT_WORLD_CLICK, false),
			m_Button(button),
			m_Clicks(clicks),
			m_Command(command),
			m_SecondaryCommand(secCommand),
			m_Entity(ent),
			m_X(x),
			m_Y(y)
		{
			AddLocalProperty(L"button", &m_Button);
			AddLocalProperty(L"clicks", &m_Clicks);
			AddLocalProperty(L"command", &m_Command);
			AddLocalProperty(L"secondaryCommand", &m_SecondaryCommand);
			if (ent)
				AddLocalProperty(L"entity", &m_Entity);
			else
				AddProperty(L"entity", JSVAL_NULL);
			AddLocalProperty(L"x", &m_X);
			AddLocalProperty(L"y", &m_Y);
		}
	};

public:
	void FireSelectionChanged( bool CausedByPlayer )
	{
		CEventSelectionChanged evt( CausedByPlayer );
		DispatchEvent( &evt );
	}
	
	void FireWorldClick(int button, int clicks, int command, int secCommand, CEntity *ent, uint x, uint y)
	{
		CEventWorldClick evt(button, clicks, command, secCommand, ent, x, y);
		DispatchEvent(&evt);
	}
};

#define g_JSGameEvents CGameEvents::GetSingleton()

#endif
