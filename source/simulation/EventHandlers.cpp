#include "precompiled.h"
#include "EventHandlers.h"
#include "Entity.h"

CEventAttack::CEventAttack( CEntity* target ) : CScriptEvent( L"attack", true, EVENT_ATTACK )
{
	m_target = target;
	AddProperty( L"target", &m_target );
}

CEventDamage::CEventDamage( CEntity* inflictor, CDamageType* damage ) : CScriptEvent( L"takesDamage", true, EVENT_DAMAGE )
{
	m_inflictor = inflictor;
	m_damage = damage;
	AddReadOnlyProperty( L"inflictor", &m_inflictor );
	AddProperty( L"damage", &m_damage );
}

CEventTargetChanged::CEventTargetChanged( CEntity* target ) : CScriptEvent( L"targetChanged", false, EVENT_TARGET_CHANGED )
{
	m_target = target;
	m_defaultAction = -1;
	AddReadOnlyProperty( L"target", &m_target );
	AddProperty( L"defaultAction", &m_defaultAction );
}

CEventPrepareOrder::CEventPrepareOrder( CEntity* target, int orderType ) : CScriptEvent( L"prepareOrder", true, EVENT_PREPARE_ORDER )
{
	m_target = target;
	m_orderType = orderType;
	AddReadOnlyProperty( L"target", &m_target );
	AddReadOnlyProperty( L"orderType", &m_orderType );
}

CEventOrderTransition::CEventOrderTransition( int orderPrevious, int orderCurrent, CEntity*& target, CVector3D& worldPosition ) : CScriptEvent( L"orderTransition", true, EVENT_ORDER_TRANSITION )
{
	m_orderPrevious = orderPrevious;
	m_orderCurrent = orderCurrent;
	m_target = &target;
	m_worldPosition = &worldPosition;
	AddReadOnlyProperty( L"orderPrevious", &m_orderPrevious );
	AddProperty( L"orderCurrent", &m_orderCurrent );
	AddProperty( L"target", m_target );
	AddProperty( L"position", m_worldPosition );
}