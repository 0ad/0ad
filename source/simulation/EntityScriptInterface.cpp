#include "precompiled.h"

#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/Sprite.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "ps/Game.h"
#include "ps/Interact.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scripting/ScriptableComplex.inl"
#include "ps/scripting/JSCollection.h"
#include "network/NetMessage.h"

#include "Aura.h"
#include "Collision.h"
#include "Entity.h"
#include "EntityFormation.h"
#include "EntityManager.h"
#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "EventHandlers.h"
#include "Formation.h"
#include "FormationManager.h"
#include "PathfindEngine.h"
#include "ProductionQueue.h"
#include "TechnologyCollection.h"
#include "TerritoryManager.h"
#include "Simulation.h"
#include "Stance.h"

#include <algorithm>

extern int g_xres, g_yres;

/*

 Scripting interface

*/

// Scripting initialization

void CEntity::ScriptingInit()
{
	// TODO: lots of these return jsval when they should return proper types
	// and make use of automatic type conversion
	AddMethod<jsval_t, &CEntity::ToString>( "toString", 0 );
	AddMethod<bool, &CEntity::OrderSingle>( "order", 1 );
	AddMethod<bool, &CEntity::OrderQueued>( "orderQueued", 1 );
	AddMethod<bool, &CEntity::OrderFromTriggers>( "orderFromTriggers", 1 );
	AddMethod<jsval_t, &CEntity::TerminateOrder>( "terminateOrder", 1 );
	AddMethod<bool, &CEntity::Kill>( "kill", 0 );
	AddMethod<bool, &CEntity::IsIdle>( "isIdle", 0 );
	AddMethod<bool, &CEntity::IsDestroyed>( "isDestroyed", 0 );
	AddMethod<bool, &CEntity::HasClass>( "hasClass", 1 );
	AddMethod<jsval_t, &CEntity::GetSpawnPoint>( "getSpawnPoint", 1 );
	AddMethod<jsval_t, &CEntity::AddAura>( "addAura", 3 );
	AddMethod<jsval_t, &CEntity::RemoveAura>( "removeAura", 1 );
	AddMethod<jsval_t, &CEntity::SetActionParams>( "setActionParams", 5 );
	AddMethod<int, &CEntity::GetCurrentRequest>( "getCurrentRequest", 0 );
	AddMethod<bool, &CEntity::ForceCheckListeners>( "forceCheckListeners", 2 );
	AddMethod<bool, &CEntity::RequestNotification>( "requestNotification", 4 );
	AddMethod<jsval_t, &CEntity::DestroyNotifier>( "destroyNotifier", 1 );
	AddMethod<jsval_t, &CEntity::DestroyAllNotifiers>( "destroyAllNotifiers", 0 );
	AddMethod<jsval_t, &CEntity::TriggerRun>( "triggerRun", 1 );
	AddMethod<jsval_t, &CEntity::SetRun>( "setRun", 1 );
	AddMethod<jsval_t, &CEntity::GetRunState>( "getRunState", 0 );
	AddMethod<bool, &CEntity::IsInFormation>( "isInFormation", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationBonus>( "getFormationBonus", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationBonusType>( "getFormationBonusType", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationBonusVal>( "getFormationBonusVal", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationPenalty>( "getFormationPenalty", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationPenaltyType>( "getFormationPenaltyType", 0 );
	AddMethod<jsval_t, &CEntity::GetFormationPenaltyVal>( "getFormationPenaltyVal", 0 );
	AddMethod<jsval_t, &CEntity::RegisterDamage>( "registerDamage", 0 );
	AddMethod<jsval_t, &CEntity::RegisterOrderChange>( "registerOrderChange", 0 );
	AddMethod<jsval_t, &CEntity::GetAttackDirections>( "getAttackDirections", 0 );
	AddMethod<jsval_t, &CEntity::FindSector>( "findSector", 4);
	AddMethod<jsval_t, &CEntity::GetHeight>( "getHeight", 0 );
	AddMethod<jsval_t, &CEntity::HasRallyPoint>( "hasRallyPoint", 0 );
	AddMethod<jsval_t, &CEntity::SetRallyPointAtCursor>( "setRallyPointAtCursor", 0 );
	AddMethod<jsval_t, &CEntity::GetRallyPoint>( "getRallyPoint", 0 );
	AddMethod<jsval_t, &CEntity::OnDamaged>( "onDamaged", 1 );
	AddMethod<jsval_t, &CEntity::GetVisibleEntities>( "getVisibleEntities", 0 );
	AddMethod<float, &CEntity::GetDistance>( "getDistance", 1 );
	AddMethod<jsval_t, &CEntity::FlattenTerrain>( "flattenTerrain", 0 );

	AddClassProperty( L"traits.id.classes", static_cast<GetFn>(&CEntity::GetClassSet), static_cast<SetFn>(&CEntity::SetClassSet) );
	AddClassProperty( L"template", static_cast<CEntityTemplate* CEntity::*>(&CEntity::m_base), false, static_cast<NotifyFn>(&CEntity::LoadBase) );

	/* Any inherited property MUST be added to EntityTemplate.cpp as well */

	AddClassProperty( L"actions.move.speed", &CEntity::m_speed );
	AddClassProperty( L"actions.move.run.speed", &CEntity::m_runSpeed );
	AddClassProperty( L"actions.move.run.rangeMin", &CEntity::m_runMinRange );
	AddClassProperty( L"actions.move.run.range", &CEntity::m_runMaxRange );
	AddClassProperty( L"actions.move.run.regenRate", &CEntity::m_runRegenRate );
	AddClassProperty( L"actions.move.run.decayRate", &CEntity::m_runDecayRate );
	AddClassProperty( L"selected", &CEntity::m_selected, false, static_cast<NotifyFn>(&CEntity::CheckSelection) );
	AddClassProperty( L"group", &CEntity::m_grouped, false, static_cast<NotifyFn>(&CEntity::CheckGroup) );
	AddClassProperty( L"traits.extant", &CEntity::m_extant );
	AddClassProperty( L"actions.move.turningRadius", &CEntity::m_turningRadius );
	AddClassProperty( L"position", &CEntity::m_position, false, static_cast<NotifyFn>(&CEntity::Teleport) );
	AddClassProperty( L"orientation", &CEntity::m_orientation, false, static_cast<NotifyFn>(&CEntity::Reorient) );
	AddClassProperty( L"player", static_cast<GetFn>(&CEntity::JSI_GetPlayer), static_cast<SetFn>(&CEntity::JSI_SetPlayer) );
	AddClassProperty( L"traits.health.curr", &CEntity::m_healthCurr );
	AddClassProperty( L"traits.health.max", &CEntity::m_healthMax );
	AddClassProperty( L"traits.health.regenRate", &CEntity::m_healthRegenRate );
	AddClassProperty( L"traits.health.regenStart", &CEntity::m_healthRegenStart );
	AddClassProperty( L"traits.health.decayRate", &CEntity::m_healthDecayRate );
	AddClassProperty( L"traits.stamina.curr", &CEntity::m_staminaCurr );
	AddClassProperty( L"traits.stamina.max", &CEntity::m_staminaMax );
	AddClassProperty( L"traits.rank.name", &CEntity::m_rankName );
	AddClassProperty( L"traits.vision.los", &CEntity::m_los );
	AddClassProperty( L"traits.ai.stance.curr", &CEntity::m_stanceName, false, static_cast<NotifyFn>(&CEntity::StanceChanged) );
	AddClassProperty( L"lastCombatTime", &CEntity::m_lastCombatTime );
	AddClassProperty( L"lastRunTime", &CEntity::m_lastRunTime );
	AddClassProperty( L"building", &CEntity::m_building );
	AddClassProperty( L"visible", &CEntity::m_visible );
	AddClassProperty( L"productionQueue", &CEntity::m_productionQueue );
	AddClassProperty( L"traits.creation.buildingLimitCategory", &CEntity::m_buildingLimitCategory );


	CJSComplex<CEntity>::ScriptingInit( "Entity", Construct, 2 );
}

// Script constructor

JSBool CEntity::Construct( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 2 );

	CVector3D position;
	float orientation = g_Game->GetSimulation()->RandFloat() * 2 * PI;

	JSObject* jsEntityTemplate = JSVAL_TO_OBJECT( argv[0] );
	CStrW templateName;

	CPlayer* player = g_Game->GetPlayer( 0 );

	CEntityTemplate* baseEntity = NULL;
	if( JSVAL_IS_OBJECT( argv[0] ) ) // only set baseEntity if jsEntityTemplate is a valid object
		baseEntity = ToNative<CEntityTemplate>( cx, jsEntityTemplate );

	if( !baseEntity )
	{
		try
		{
			templateName = g_ScriptingHost.ValueToUCString( argv[0] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			*rval = JSVAL_NULL;
			JS_ReportError( cx, "Invalid template identifier" );
			return( JS_TRUE );
		}
		baseEntity = g_EntityTemplateCollection.GetTemplate( templateName );
	}

	if( !baseEntity )
	{
		*rval = JSVAL_NULL;
		JS_ReportError( cx, "No such template: %s", CStr8(templateName).c_str() );
		return( JS_TRUE );
	}

	JSI_Vector3D::Vector3D_Info* jsVector3D = NULL;
	if( JSVAL_IS_OBJECT( argv[1] ) )
		jsVector3D = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( argv[1] ), &JSI_Vector3D::JSI_class, NULL );

	if( jsVector3D )
	{
		position = *( jsVector3D->vector );
	}

	if( argc >= 3 )
	{
		try
		{
			orientation = ToPrimitive<float>( argv[2] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			// TODO: Net-safe random for this parameter.
			orientation = 0.0f;
		}
	}

	if( argc >= 4 )
	{
		try
		{
			player = ToPrimitive<CPlayer*>( argv[3] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			player = g_Game->GetPlayer( 0 );
		}
	}

	std::set<CStr8> selections; // TODO: let scripts specify selections?
	HEntity handle = g_EntityManager.Create( baseEntity, position, orientation, selections );
	handle->m_actor->SetPlayerID( player->GetPlayerID() );
	handle->Initialize();

	*rval = ToJSVal<CEntity>( *handle );
	return( JS_TRUE );
}

// Script-bound methods

jsval_t CEntity::ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	CStrW name( L"[object Entity: " + m_base->m_Tag + L"]" );
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, name.utf16().c_str() ) ) );
}

jsval CEntity::JSI_GetPlayer()
{
	return ToJSVal<CPlayer>( m_player );
}

void CEntity::JSI_SetPlayer( jsval val )
{
	CPlayer* newPlayer = 0;

	try 
	{
		newPlayer = ToPrimitive<CPlayer*>( val );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( g_ScriptingHost.getContext(), "Invalid value given to entity.player - should be a Player object." );
		return;
	}

	// Cancel all production to refund the old player
	m_productionQueue->CancelAll();

	// Exit all our auras so we can re-enter them as the new player
	ExitAuras();
	
	if( m_actor )
		m_actor->SetPlayerID( newPlayer->GetPlayerID() ); // calls this->SetPlayer
	else
		SetPlayer(newPlayer);
}

bool CEntity::Order( JSContext* cx, uintN argc, jsval* argv, CEntityOrder::EOrderSource source, bool Queued )
{
	// This needs to be sorted (uses Scheduler rather than network messaging)

	int orderCode;
	debug_assert(argc >= 1);
	try
	{
		orderCode = ToPrimitive<int>( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid order type" );
		return( false );
	}

	CEntityOrder newOrder;

	newOrder.m_source = source;

	CEntity* target;

	newOrder.m_type = (CEntityOrder::EOrderType)orderCode;

	switch( orderCode )
	{
		case CEntityOrder::ORDER_GOTO:
		case CEntityOrder::ORDER_RUN:
		case CEntityOrder::ORDER_PATROL:
			JSU_REQUIRE_PARAMS_CPP(3);
			try
			{
				newOrder.m_target_location.x = ToPrimitive<float>( argv[1] );
				newOrder.m_target_location.y = ToPrimitive<float>( argv[2] );
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid location" );
				return( false );
			}
			//It's not a notification order
			if ( argc == 3 )
			{
				if ( entf_get(ENTF_DESTROY_NOTIFIERS) )
				{
					m_currentRequest=0;
					DestroyAllNotifiers();
				}
			}
			break;
		case CEntityOrder::ORDER_CONTACT_ACTION:
			JSU_REQUIRE_PARAMS_CPP(4);
			target = ToNative<CEntity>( argv[1] );
			if( !target )
			{
				JS_ReportError( cx, "Invalid target" );
				return( false );
			}
			newOrder.m_target_entity = target->me;
			try
			{
				newOrder.m_action = ToPrimitive<int>( argv[2] );
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid generic order type parameter - expected int" );
				return( false );
			}
			try
			{
				newOrder.m_run = ToPrimitive<bool>( argv[3] );
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid generic order run parameter - expected bool" );
				return( false );
			}
			//It's not a notification order
			if ( argc == 4 )
			{
				if ( entf_get(ENTF_DESTROY_NOTIFIERS) )
				{
					m_currentRequest=0;
					DestroyAllNotifiers();
				}
			}
			break;
		case CEntityOrder::ORDER_PRODUCE:
			JSU_REQUIRE_PARAMS_CPP(3);
			try {
				newOrder.m_name = ToPrimitive<CStrW>(argv[2]);
				newOrder.m_produce_type = ToPrimitive<int>(argv[1]);
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid parameter types" );
				return( false );
			}
			break;
		default:
			JS_ReportError( cx, "Invalid order type" );
			return( false );
	}

	if( !Queued )
		ClearOrders();
	PushOrder( newOrder );

	return( true );
}

bool CEntity::Kill( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	Kill(true);

	return( true );
}

jsval_t CEntity::GetSpawnPoint( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	float spawn_clearance = 2.0f;
	if( argc >= 1 )
	{
		CEntityTemplate* be = ToNative<CEntityTemplate>( argv[0] );
		if( be )
		{
			switch( be->m_bound_type )
			{
				case CBoundingObject::BOUND_CIRCLE:
				spawn_clearance = be->m_bound_circle->m_radius;
				break;
				case CBoundingObject::BOUND_OABB:
				spawn_clearance = be->m_bound_box->m_radius;
				break;
				default:
				debug_warn("No bounding information for spawned object!" );
			}
		}
		else
			spawn_clearance = ToPrimitive<float>( argv[0] );
	}
	else
		debug_warn("No arguments to Entity::GetSpawnPoint()" );

	// TODO: Make netsafe.
	CBoundingCircle spawn( 0.0f, 0.0f, spawn_clearance, 0.0f );

	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
	{
		CBoundingBox* oabb = (CBoundingBox*)m_bounds;

		// Pick a start point

		int edge = g_Game->GetSimulation()->RandInt( 4 );
		int point;

		double max_w = oabb->m_w + spawn_clearance + 2.0;
		double max_d = oabb->m_d + spawn_clearance + 2.0;
		int w_count = (int)( max_w * 2 );
		int d_count = (int)( max_d * 2 );

		CVector2D w_step = oabb->m_v * (float)( max_w / w_count );
		CVector2D d_step = oabb->m_u * (float)( max_d / d_count );
		CVector2D pos( m_position );
		if( edge & 1 )
		{
			point = g_Game->GetSimulation()->RandInt( 2 * d_count ) - d_count;
			pos += ( oabb->m_v * (float)max_w + d_step * (float)point ) * ( ( edge & 2 ) ? -1.0f : 1.0f );
		}
		else
		{
			point = g_Game->GetSimulation()->RandInt( 2 * w_count ) - w_count;
			pos += ( oabb->m_u * (float)max_d + w_step * (float)point ) * ( ( edge & 2 ) ? -1.0f : 1.0f );
		}

		int start_edge = edge;
		int start_point = point;

		spawn.m_pos = pos;

		// Then step around the edge (clockwise) until a free space is found, or
		// we've gone all the way around.
		while( GetCollisionObject( &spawn ) )
		{
			switch( edge )
			{
				case 0:
				point++;
				pos += w_step;
				if( point >= w_count )
				{
					edge = 1;
					point = -d_count;
				}
				break;
				case 1:
				point++;
				pos -= d_step;
				if( point >= d_count )
				{
					edge = 2;
					point = w_count;
				}
				break;
				case 2:
				point--;
				pos -= w_step;
				if( point <= -w_count )
				{
					edge = 3;
					point = d_count;
				}
				break;
				case 3:
				point--;
				pos += d_step;
				if( point <= -d_count )
				{
					edge = 0;
					point = -w_count;
				}
				break;
			}
			if( ( point == start_point ) && ( edge == start_edge ) )
				return( JSVAL_NULL );
			spawn.m_pos = pos;
		}
		CVector3D rval( pos.x, GetAnchorLevel( pos.x, pos.y ), pos.y );
		return( ToJSVal( rval ) );
	}
	else if( m_bounds->m_type == CBoundingObject::BOUND_CIRCLE )
	{
		float ang;
		ang = g_Game->GetSimulation()->RandFloat() * 2 * PI;
		float radius = m_bounds->m_radius + 1.0f + spawn_clearance;
		float d_ang = spawn_clearance / ( 2.0f * radius );
		float ang_end = ang + 2.0f * PI;
		float x = 0.0f, y = 0.0f; // make sure they're initialized
		for( ; ang < ang_end; ang += d_ang )
		{
			x = m_position.X + radius * cos( ang );
			y = m_position.Z + radius * sin( ang );
			spawn.SetPosition( x, y );
			if( !GetCollisionObject( &spawn ) )
				break;
		}
		if( ang < ang_end )
		{
			// Found a satisfactory position...
			CVector3D pos( x, 0, y );
			pos.Y = GetAnchorLevel( x, y );
			return( ToJSVal( pos ) );
		}
		else
			return( JSVAL_NULL );
	}
	return( JSVAL_NULL );
}

jsval_t CEntity::AddAura( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc >= 8 );
	debug_assert( JSVAL_IS_OBJECT(argv[7]) );

	CStrW name = ToPrimitive<CStrW>( argv[0] );
	float radius = ToPrimitive<float>( argv[1] );
	int tickRate = std::max( 0, ToPrimitive<int>( argv[2] ) );	// since it's a size_t we don't want it to be negative
	float r = ToPrimitive<float>( argv[3] );
	float g = ToPrimitive<float>( argv[4] );
	float b = ToPrimitive<float>( argv[5] );
	float a = ToPrimitive<float>( argv[6] );
	CVector4D color(r, g, b, a);
	JSObject* handler = JSVAL_TO_OBJECT( argv[7] );

	if( m_auras[name] )
	{
		delete m_auras[name];
	}
	m_auras[name] = new CAura( cx, this, name, radius, tickRate, color, handler );

	initAuraData();

	return JSVAL_VOID;
}

jsval_t CEntity::RemoveAura( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	debug_assert( argc >= 1 );
	CStrW name = ToPrimitive<CStrW>( argv[0] );
	if( m_auras[name] )
	{
		delete m_auras[name];
		m_auras.erase(name);
	}

	initAuraData();

	return JSVAL_VOID;
}

jsval_t CEntity::SetActionParams( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	debug_assert( argc == 5 );

	int id = ToPrimitive<int>( argv[0] );
	float minRange = ToPrimitive<int>( argv[1] );
	float maxRange = ToPrimitive<int>( argv[2] );
	int speed = ToPrimitive<int>( argv[3] );
	CStr8 animation = ToPrimitive<CStr8>( argv[4] );

	m_actions[id] = SEntityAction( id, minRange, maxRange, speed, animation );

	return JSVAL_VOID;
}

bool CEntity::RequestNotification( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(4);

	CEntityListener notify;
	CEntity *target = ToNative<CEntity>( argv[0] );
	notify.m_type = (CEntityListener::EType)ToPrimitive<int>( argv[1] );
	bool tmpDestroyNotifiers = ToPrimitive<bool>( argv[2] );
	entf_set_to(ENTF_DESTROY_NOTIFIERS, !ToPrimitive<bool>( argv[3] ));

	if (target == this)
		return false;

	notify.m_sender = this;

	//Clean up old requests
	if ( tmpDestroyNotifiers )
		DestroyAllNotifiers();
	//If new request is not the same and we're destroy notifiers, reset
	else if ( !(notify.m_type & m_currentRequest) && entf_get(ENTF_DESTROY_NOTIFIERS))
		DestroyAllNotifiers();

	m_currentRequest = notify.m_type;
	m_notifiers.push_back( target );
	int result = target->m_currentNotification & notify.m_type;

	//If our target isn't stationary and it's doing something we want to follow, send notification
	if ( result && !target->m_orderQueue.empty() )
	{
		CEntityOrder order = target->m_orderQueue.front();
		switch( result )
		{
			 case CEntityListener::NOTIFY_GOTO:
			 case CEntityListener::NOTIFY_RUN:
				DispatchNotification( order, result );
				break;

			 case CEntityListener::NOTIFY_HEAL:
			 case CEntityListener::NOTIFY_ATTACK:
			 case CEntityListener::NOTIFY_GATHER:
			 case CEntityListener::NOTIFY_DAMAGE:
				 DispatchNotification( order, result );
				 break;
			 default:
				 JS_ReportError( cx, "Invalid order type" );
				 break;
		}
		target->m_listeners.push_back( notify );
		return true;
	}

	target->m_listeners.push_back( notify );
	return false;
}
int CEntity::GetCurrentRequest( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return m_currentRequest;
}
bool CEntity::ForceCheckListeners( JSContext *cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(2);
	int type = ToPrimitive<int>( argv[0] );	   //notify code
	m_currentNotification = type;

	CEntity *target = ToNative<CEntity>( argv[1] );
	if ( target->m_orderQueue.empty() )
		return false;

	CEntityOrder order = target->m_orderQueue.front();
	for (size_t i=0; i<m_listeners.size(); i++)
	{
		int result = m_listeners[i].m_type & type;
		if ( result )
		{
			 switch( result )
			 {
				 case CEntityListener::NOTIFY_GOTO:
				 case CEntityListener::NOTIFY_RUN:
				 case CEntityListener::NOTIFY_HEAL:
				 case CEntityListener::NOTIFY_ATTACK:
				 case CEntityListener::NOTIFY_GATHER:
				 case CEntityListener::NOTIFY_DAMAGE:
				 case CEntityListener::NOTIFY_IDLE:	//target should be 'this'
					m_listeners[i].m_sender->DispatchNotification( order, result );
					break;

				default:
					JS_ReportError( cx, "Invalid order type" );
					break;
			 }
		 }
	}
	return true;
}
void CEntity::CheckListeners( int type, CEntity *target)
{
	m_currentNotification = type;

	debug_assert(target);
	if ( target->m_orderQueue.empty() )
		return;

	CEntityOrder order = target->m_orderQueue.front();
	for (size_t i=0; i<m_listeners.size(); i++)
	{
		int result = m_listeners[i].m_type & type;
		if ( result )
		{
			 switch( result )
			 {
				 case CEntityListener::NOTIFY_GOTO:
				 case CEntityListener::NOTIFY_RUN:
				 case CEntityListener::NOTIFY_HEAL:
				 case CEntityListener::NOTIFY_ATTACK:
				 case CEntityListener::NOTIFY_GATHER:
				 case CEntityListener::NOTIFY_DAMAGE:
					 m_listeners[i].m_sender->DispatchNotification( order, result );
					 break;
				 default:
					debug_warn("Invalid notification: CheckListeners()");
					continue;
			 }
		}
	}
}
jsval_t CEntity::DestroyAllNotifiers( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	DestroyAllNotifiers();
	return JS_TRUE;
}
jsval_t CEntity::DestroyNotifier( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	DestroyNotifier( ToNative<CEntity>( argv[0] ) );
	return JS_TRUE;
}

jsval_t CEntity::TriggerRun( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	entf_set(ENTF_TRIGGER_RUN);
	return JSVAL_VOID;
}

jsval_t CEntity::SetRun( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	bool should_run = ToPrimitive<bool> ( argv[0] );
	entf_set_to(ENTF_SHOULD_RUN, should_run);
	entf_set_to(ENTF_IS_RUNNING, should_run);
	return JSVAL_VOID;
}
jsval_t CEntity::GetRunState( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return BOOLEAN_TO_JSVAL( entf_get(ENTF_SHOULD_RUN) );
}
jsval_t CEntity::GetFormationPenalty( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenalty() );
}
jsval_t CEntity::GetFormationPenaltyBase( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenaltyBase() );
}
jsval_t CEntity::GetFormationPenaltyType( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenaltyType() );
}
jsval_t CEntity::GetFormationPenaltyVal( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenaltyVal() );
}
jsval_t CEntity::GetFormationBonus( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonus() );
}
jsval_t CEntity::GetFormationBonusBase( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonusBase() );
}
jsval_t CEntity::GetFormationBonusType( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonusType() );
}
jsval_t CEntity::GetFormationBonusVal( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonusVal() );
}

jsval_t CEntity::RegisterDamage( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CEntity* inflictor = ToNative<CEntity>( argv[0] );
	CVector2D up(1.0f, 0.0f);
	CVector2D pos = CVector2D( inflictor->m_position.X, inflictor->m_position.Z );
	CVector2D posDelta = (pos - m_position).Normalize();

	float angle = acosf( up.Dot(posDelta) );
	//Find what section it is between and "activate" it
	int sector = FindSector(m_base->m_sectorDivs, angle, DEGTORAD(360.0f))-1;
	m_sectorValues[sector]=true;
	return JS_TRUE;
}
jsval_t CEntity::RegisterOrderChange( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CEntity* idleEntity = ToNative<CEntity>( argv[0] );

	CVector2D up(1.0f, 0.0f);
	CVector2D pos = CVector2D( idleEntity->m_position.X, idleEntity->m_position.Z );
	CVector2D posDelta = (pos - m_position).Normalize();

	float angle = acosf( up.Dot(posDelta) );
	//Find what section it is between and "deactivate" it
	int sector = std::max(0, FindSector(m_base->m_sectorDivs, angle, DEGTORAD(360.0f)));
	m_sectorValues[sector]=false;
	return JS_TRUE;
}
jsval_t CEntity::GetAttackDirections( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	int directions=0;

	for ( std::vector<bool>::iterator it=m_sectorValues.begin(); it != m_sectorValues.end(); it++ )
	{
		if ( *it )
			++directions;
	}
	return ToJSVal( directions );
}
jsval_t CEntity::FindSector( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(4);

	try
	{
		int divs = ToPrimitive<int>( argv[0] );
		float angle = ToPrimitive<float>( argv[1] );
		float maxAngle = ToPrimitive<float>( argv[2] );
		bool negative = ToPrimitive<bool>( argv[3] );

		return ToJSVal( FindSector(divs, angle, maxAngle, negative) );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid parameters for FindSector" );
		return 0;
	}
}
jsval_t CEntity::HasRallyPoint( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( entf_get(ENTF_HAS_RALLY_POINT) );
}
jsval_t CEntity::GetRallyPoint( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	CVector3D v3d( m_rallyPoint.x, m_rallyPoint.y, 0 );
	return ToJSVal( v3d );
}
jsval_t CEntity::SetRallyPointAtCursor( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	// Queue an order over the network to set a rally point
	CSetRallyPointMessage* msg = new CSetRallyPointMessage;
	msg->m_Entities = CEntityList(me);
	msg->m_IsQueued = true;
	CVector3D point = g_Game->GetView()->GetCamera()->GetWorldCoordinates(true);
	msg->m_TargetX = (int) point.X;
	msg->m_TargetY = (int) point.Z;
	g_Game->GetSimulation()->QueueLocalCommand(msg);
	return JS_TRUE;
}

// Called by the script when the entity is damaged, to let it retaliate
jsval_t CEntity::OnDamaged( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CEntity* damageSource = ToNative<CEntity>( argv[0] );
	m_stance->OnDamaged( damageSource );
	return JSVAL_VOID;
}

jsval_t CEntity::GetVisibleEntities( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	std::vector<CEntity*> pointers;
	g_EntityManager.GetInLOS( this, pointers );
	std::vector<HEntity> handles( pointers.size() );
	for( size_t i=0; i<pointers.size(); i++ )
		handles[i] = pointers[i]->me;
	return OBJECT_TO_JSVAL( EntityCollection::Create( handles ) );
}

float CEntity::GetDistance( JSContext* cx, uintN argc, jsval* argv )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CEntity* target = ToNative<CEntity>( argv[0] );
	if( !target )
		return -1.0f;
	return this->Distance2D( target );
}

/*

Methods that provide an interface from C++ to JavaScript functions.

*/

int CEntity::GetAttackAction( HEntity target )
{
	jsval attackFunc;
	if( GetProperty( g_ScriptingHost.GetContext(), L"getAttackAction", &attackFunc ) )
	{
		jsval arg = ToJSVal( target );
		jsval rval;
		if( JS_CallFunctionValue( g_ScriptingHost.GetContext(), GetScript(), attackFunc, 1, &arg, &rval ) == JS_TRUE )
		{
			return ToPrimitive<int>( rval );
		}
	}

	// Default return value is an invalid action ID
	return 0;
}

jsval_t CEntity::FlattenTerrain( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	float xDiff, yDiff;
	CVector3D pos = m_position;
	CVector2D pos2D(m_position.X, m_position.Z);

	if ( m_bounds->m_type == CBoundingObject::BOUND_CIRCLE )
		xDiff = yDiff = m_bounds->m_radius;
	else
	{
		CBoundingBox* box = static_cast<CBoundingBox*>( m_bounds );
		xDiff = box->m_w;
		yDiff = box->m_d;
	}
	//If we ever need to take rotated bounding boxes into account...
	//CVector2D points[4] = { CVector2D(xDiff, yDiff), CVector2D(-xDiff, -yDiff),
		//CVector2D(-xDiff, yDiff), CVector2D(xDiff, yDiff) };
	//CVector3D circle = points[0].FindCircumCircle(points[1], points[2]);
	//float cos = cosf(m_graphics_orientation.Y), sin = sinf(m_graphics_orientation.Y);

	//for ( size_t i = 0; i<4; ++i )
	//{
	//	points[i].x *= cos*circle.Z;
	//	points[i].y *= sin*circle.Z;
	//}
	g_Game->GetWorld()->GetTerrain()->FlattenArea(pos.X-xDiff, pos.X+xDiff, pos.Z-yDiff, pos.Z+yDiff);
	SnapToGround();
	return JS_TRUE;
}
