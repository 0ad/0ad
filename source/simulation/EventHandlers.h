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

// List of event handlers (for entities) the engine will call in to.
// Using integer tags should be ever-so-slightly faster than the hashmap lookup
// Also allows events to be renamed without affecting other code.

#ifndef INCLUDED_EVENTHANDLERS
#define INCLUDED_EVENTHANDLERS

#include "scripting/DOMEvent.h"
#include "maths/Vector3D.h"
#include "EntityOrders.h"

class CEventInitialize : public CScriptEvent
{
public:
	CEventInitialize() : CScriptEvent( L"initialize", EVENT_INITIALIZE, true ) {}
};

class CEventDeath : public CScriptEvent
{
public:
	CEventDeath() : CScriptEvent( L"death", EVENT_DEATH, false ) {}
};

/*
class CEventTick : public CScriptEvent
{
public:
	CEventTick() : CScriptEvent( L"tick", EVENT_TICK, false ) {}
};
*/

class CEventContactAction : public CScriptEvent
{
	CEntity* m_target;
	int m_action;
public:
	CEventContactAction( CEntity* target, int action );
};

class CEventTargetExhausted : public CScriptEvent
{
	CEntity* m_target;
	int m_action;
public:
	CEventTargetExhausted( CEntity* target, int action );
};

class CEventStartConstruction : public CScriptEvent
{
	CEntity* m_target;
public:
	CEventStartConstruction( CEntity* target );
};

class CEventStartProduction : public CScriptEvent
{
	int m_productionType;
	CStrW m_name;
	float m_time;
public:
	CEventStartProduction( int productionType, const CStrW& name );
	inline float GetTime() { return m_time; }
};

class CEventFinishProduction : public CScriptEvent
{
	int m_productionType;
	CStrW m_name;
public:
	CEventFinishProduction( int productionType, const CStrW& name );
};

class CEventCancelProduction : public CScriptEvent
{
	int m_productionType;
	CStrW m_name;
public:
	CEventCancelProduction( int productionType, const CStrW& name );
};

class CEventTargetChanged : public CScriptEvent
{
	CEntity* m_target;
public:
	int m_defaultOrder;
	int m_defaultAction;
	CStrW m_defaultCursor;
	CStrW m_secondaryCursor;
	int m_secondaryOrder;
	int m_secondaryAction;
	CEventTargetChanged( CEntity* target );
};

class CEventPrepareOrder : public CScriptEvent
{
public:
	CEntity* m_target;
	int m_orderType;
	int m_action;
	CStrW m_name;
	CEntity* m_notifySource;
	int m_notifyType;
	CEventPrepareOrder( CEntity* target, int orderType, int action, const CStrW& name );
};

class CEventOrderTransition : public CScriptEvent
{
	int m_orderPrevious;
	int m_orderCurrent;
	HEntity m_target;
	CVector3D m_worldPosition;
public:
	CEventOrderTransition( int orderPrevious, int orderCurrent, CEntity* target, CVector3D& worldPosition );
};
class CEventNotification : public CScriptEvent
{
	//Same as CEntityOrder data for support of all orders
	CEntity* m_target;
	int m_action;	//u64 is unsupported...will this work?
	int m_notifyType;
	CVector3D m_location;	//No real use for y, but CVector2D unsupported
public:
	CEventNotification( const CEntityOrder& order, int notifyType );
};
class CFormationEvent : public CScriptEvent
{
	 int m_formationEvent;

public:
	CFormationEvent( int type );

	enum FormationEventType
	{
		FORMATION_ENTER,
		FORMATION_LEAVE,
		FORMATION_DAMAGE,
		FORMATION_ATTACK,

		FORMATION_LAST
	};
};
class CIdleEvent : public CScriptEvent
{
	int m_notifyType;	//previous order in notification code form
	int m_orderType;
	int m_action;	//previous order in terms of generic order action
	CVector3D m_location;
	CEntity* m_target;
public:
	CIdleEvent( const CEntityOrder& order, int notifyType );
};
#endif
