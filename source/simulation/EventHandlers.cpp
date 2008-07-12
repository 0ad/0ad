#include "precompiled.h"
#include "EventHandlers.h"
#include "Entity.h"

CEventGeneric::CEventGeneric( CEntity* target, int action ) : CScriptEvent( L"generic", EVENT_GENERIC, true)
{
	m_target = target;
	m_action = action;
	AddLocalProperty( L"target", &m_target );
	AddLocalProperty( L"action", &m_action );
}

CEventTargetExhausted::CEventTargetExhausted( CEntity* target, int action ) : CScriptEvent( L"TargetExhausted", EVENT_TARGET_EXHAUSTED, true)
{
	m_target = target;
	m_action = action;
	AddLocalProperty( L"target", &m_target );
	AddLocalProperty( L"action", &m_action );
}

CEventStartConstruction::CEventStartConstruction( CEntity* target ) : CScriptEvent( L"startConstruction", EVENT_START_CONSTRUCTION, true)
{
	m_target = target;
	AddLocalProperty( L"target", &m_target );
}

CEventStartProduction::CEventStartProduction( int productionType, const CStrW& name )
	: CScriptEvent( L"startProduction", EVENT_START_PRODUCTION, true)
{
	m_productionType = productionType;
	m_name = name;
	m_time = -1;
	AddLocalProperty( L"productionType", &m_productionType );
	AddLocalProperty( L"name", &m_name );
	AddLocalProperty( L"time", &m_time );
}

CEventFinishProduction::CEventFinishProduction( int productionType, const CStrW& name )
	: CScriptEvent( L"finishProduction", EVENT_FINISH_PRODUCTION, true)
{
	m_productionType = productionType;
	m_name = name;
	AddLocalProperty( L"productionType", &m_productionType );
	AddLocalProperty( L"name", &m_name );
}

CEventCancelProduction::CEventCancelProduction( int productionType, const CStrW& name )
	: CScriptEvent( L"cancelProduction", EVENT_CANCEL_PRODUCTION, true)
{
	m_productionType = productionType;
	m_name = name;
	AddLocalProperty( L"productionType", &m_productionType );
	AddLocalProperty( L"name", &m_name );
}

CEventTargetChanged::CEventTargetChanged( CEntity* target ) : CScriptEvent( L"targetChanged", EVENT_TARGET_CHANGED, false )
{
	m_target = target;
	m_defaultOrder = -1;
	m_defaultAction = 0;
	m_defaultCursor = L"arrow-default";
	
	m_secondaryOrder = -1;
	m_secondaryAction = 0;
	m_secondaryCursor = L"arrow-default";
	
	AddLocalProperty( L"target", &m_target, true );
	AddLocalProperty( L"defaultOrder", &m_defaultOrder );
	AddLocalProperty( L"defaultAction", &m_defaultAction );
	AddLocalProperty( L"defaultCursor", &m_defaultCursor );
	AddLocalProperty( L"secondaryOrder", &m_secondaryOrder );
	AddLocalProperty( L"secondaryCursor", &m_secondaryCursor );
	AddLocalProperty( L"secondaryAction", &m_secondaryAction );
}

CEventPrepareOrder::CEventPrepareOrder( CEntity* target, int orderType, int action, const CStrW& name) 
	: CScriptEvent( L"prepareOrder", EVENT_PREPARE_ORDER, true )
{
	m_target = target;
	m_orderType = orderType;
	m_action = action;
	m_name = name;
	AddLocalProperty( L"target", &m_target, true );
	AddLocalProperty( L"orderType", &m_orderType, true );
	AddLocalProperty( L"action", &m_action );
	AddLocalProperty( L"name", &m_name );
	AddLocalProperty( L"notifyType", &m_notifyType );
	AddLocalProperty( L"notifySource", &m_notifySource );
}

CEventOrderTransition::CEventOrderTransition( int orderPrevious, int orderCurrent, CEntity* target, CVector3D& worldPosition ) 
	: CScriptEvent( L"orderTransition", EVENT_ORDER_TRANSITION, true )
{
	m_orderPrevious = orderPrevious;
	m_orderCurrent = orderCurrent;

	if(target) {
		m_target = target->me;
	}
	else {
		m_target = HEntity();
	}

	m_worldPosition = worldPosition;

	AddLocalProperty( L"orderPrevious", &m_orderPrevious, true );
	AddLocalProperty( L"orderCurrent", &m_orderCurrent );
	AddLocalProperty( L"target", &m_target );
	AddLocalProperty( L"position", &m_worldPosition );
}
CEventNotification::CEventNotification( const CEntityOrder& order, int notifyType ) : CScriptEvent( L"notification", EVENT_NOTIFICATION, true )
{
	m_notifyType = notifyType;
	m_target = order.m_target_entity;
	CVector3D convert( order.m_target_location.x, 0.0f, order.m_target_location.y );
	m_location = convert;
	
	AddLocalProperty( L"notifyType", &m_notifyType );
	AddLocalProperty( L"target", &m_target );
	AddLocalProperty( L"location", &m_location );
}
CFormationEvent::CFormationEvent( int type ) : CScriptEvent( L"formationEvent", EVENT_FORMATION, true )
{
	(int&) m_formationEvent = type;
	AddLocalProperty( L"formationEvent", &m_formationEvent );
}
CIdleEvent::CIdleEvent( const CEntityOrder& order, int notifyType ) : CScriptEvent( L"idleEvent", EVENT_IDLE, false )
{
	m_notifyType = notifyType;
	m_orderType = order.m_type;
	m_target = order.m_target_entity;
	CVector3D convert( order.m_target_location.x, 0.0f, order.m_target_location.y );
	m_location = convert;
	
	AddLocalProperty( L"notifyType", &m_notifyType );
	AddLocalProperty( L"orderType", &m_orderType );
	AddLocalProperty( L"target", &m_target );
	AddLocalProperty( L"location", &m_location );
}
