// GameEvents.h
// Defines a singleton class, g_JSGameEvents that fires certain events on
// request (Fire*). This serves to notify scripts of important game events.
// The CScriptEvent-derived events are declared here as well,
// with their type set to one of EventTypes.h's EEventType.

#ifndef INCLUDED_GAMEEVENTS
#define INCLUDED_GAMEEVENTS

#include "DOMEvent.h"
#include "EventTypes.h"
#include "ps/Singleton.h"

class CGameEvents : public IEventTarget, public Singleton<CGameEvents>
{
	// Game events don't really run on an object
	JSObject* GetScriptExecContext( IEventTarget* UNUSED(target) ) { return( g_ScriptingHost.GetGlobalObject() ); }

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
		int m_Order;
		int m_Action;
		int m_SecondaryOrder;
		int m_SecondaryAction;
		CEntity *m_Entity;
		int m_X, m_Y;
	public:
		CEventWorldClick(int button, int clicks, int order, int action,
				int secOrder, int secAction, CEntity *ent, int x, int y):
			CScriptEvent(L"worldClick", EVENT_WORLD_CLICK, false),
			m_Button(button),
			m_Clicks(clicks),
			m_Order(order),
			m_Action(action),
			m_SecondaryOrder(secOrder),
			m_SecondaryAction(secAction),
			m_Entity(ent),
			m_X(x),
			m_Y(y)
		{
			AddLocalProperty(L"button", &m_Button);
			AddLocalProperty(L"clicks", &m_Clicks);
			AddLocalProperty(L"order", &m_Order);
			AddLocalProperty(L"action", &m_Action);
			AddLocalProperty(L"secondaryOrder", &m_SecondaryOrder);
			AddLocalProperty(L"secondaryAction", &m_SecondaryAction);
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
	
	void FireWorldClick(int button, int clicks, int order, int action,
		int secOrder, int secAction, CEntity *ent, int x, int y)
	{
		CEventWorldClick evt(button, clicks, order, action, secOrder, secAction, ent, x, y);
		DispatchEvent(&evt);
	}
};

#define g_JSGameEvents CGameEvents::GetSingleton()

#endif
