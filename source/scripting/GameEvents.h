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
		CEventSelectionChanged( bool CausedByPlayer ) : CScriptEvent( L"selectionChanged", EVENT_SELECTION_CHANGED, false )
		{
			AddLocalProperty( L"byPlayer", &m_CausedByPlayer, true );
		}
	};

public:
	void FireSelectionChanged( bool CausedByPlayer )
	{
		CEventSelectionChanged evt( CausedByPlayer );
		DispatchEvent( &evt );
	}
};

#define g_JSGameEvents CGameEvents::GetSingleton()

#endif