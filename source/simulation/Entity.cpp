// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "precompiled.h"

#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "Unit.h"

#include "Renderer.h"
#include "Model.h"
#include "Terrain.h"
#include "Interact.h"

#include "Collision.h"
#include "PathfindEngine.h"

#include "Game.h"

#include "scripting/JSInterface_Vector3D.h"

CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation )
{
	m_position = position;
	m_orientation = orientation;
	m_ahead.x = sin( m_orientation );
	m_ahead.y = cos( m_orientation );
	
	AddProperty( L"template", (CBaseEntity**)&m_base, false, (NotifyFn)&CEntity::loadBase );
	AddProperty( L"actions.move.speed", &m_speed );
	AddProperty( L"selected", &m_selected, false, (NotifyFn)&CEntity::checkSelection );
	AddProperty( L"group", &m_grouped, false, (NotifyFn)&CEntity::checkGroup );
	AddProperty( L"traits.extant", &m_extant );
	AddProperty( L"traits.corpse", &m_corpse );
	AddProperty( L"actions.move.turningradius", &m_turningRadius );
	AddProperty( L"actions.attack.range", &m_meleeRange );
	AddProperty( L"actions.attack.rangemin", &m_meleeRangeMin );
	AddProperty( L"position", &m_graphics_position, false, (NotifyFn)&CEntity::teleport );
	AddProperty( L"orientation", &m_graphics_orientation, false, (NotifyFn)&CEntity::reorient );
	
	for( int t = 0; t < EVENT_LAST; t++ )
		AddProperty( EventNames[t], &m_EventHandlers[t] );
	
	// Set our parent unit and build us an actor.
	m_actor = NULL;
	m_bounds = NULL;

	m_lastState = -1;
	m_transition = true;

	m_base = base;
	
	loadBase();
	
	if( m_bounds )
		m_bounds->setPosition( m_position.X, m_position.Z );
	
	m_position_previous = m_position;
	m_orientation_previous = m_orientation;

	m_graphics_position = m_position;
	m_graphics_orientation = m_orientation;

	m_destroyed = false;

	m_selected = false;

	m_grouped = -1;
}

CEntity::~CEntity()
{
	if( m_actor )
	{
		g_UnitMan.RemoveUnit( m_actor );
		delete( m_actor );
	}
	if( m_bounds ) delete( m_bounds );
}
	
void CEntity::loadBase()
{
	if( m_actor )
	{
		g_UnitMan.RemoveUnit( m_actor );
		delete( m_actor );
	}
	if( m_bounds )
	{
		delete( m_bounds );
	}

	m_actor = new CUnit( m_base->m_actorObject, m_base->m_actorObject->m_Model->Clone(), this );

	// Register the actor with the renderer.

	g_UnitMan.AddUnit( m_actor );

	// Set up our instance data

	SetBase( m_base );

	if( m_base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
	{
 		m_bounds = new CBoundingCircle( m_position.X, m_position.Z, m_base->m_bound_circle );
	}
	else if( m_base->m_bound_type == CBoundingObject::BOUND_OABB )
	{
		m_bounds = new CBoundingBox( m_position.X, m_position.Z, m_ahead, m_base->m_bound_box );
	}
}

void CEntity::kill()
{
	g_Selection.removeAll( me );

	if( m_bounds ) delete( m_bounds );
	m_bounds = NULL;

	m_destroyed = true;
	Shutdown();

	if( m_actor )
	{
		g_UnitMan.RemoveUnit( m_actor );
		delete( m_actor );
		m_actor = NULL;
	}

	me = HEntity(); // will deallocate the entity, assuming nobody else has a reference to it
}

void CEntity::updateActorTransforms()
{
	CMatrix3D m;
	
	float s = sin( m_graphics_orientation );
	float c = cos( m_graphics_orientation );

	m._11 = -c;		m._12 = 0.0f;	m._13 = -s;		m._14 = m_graphics_position.X;
	m._21 = 0.0f;	m._22 = 1.0f;	m._23 = 0.0f;	m._24 = m_graphics_position.Y;
	m._31 = s;		m._32 = 0.0f;	m._33 = -c;		m._34 = m_graphics_position.Z;
	m._41 = 0.0f;	m._42 = 0.0f;	m._43 = 0.0f;	m._44 = 1.0f;

	m_actor->GetModel()->SetTransform( m );
}

void CEntity::snapToGround()
{
	CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
	
	m_graphics_position.Y = pTerrain->getExactGroundLevel( m_graphics_position.X, m_graphics_position.Z );
}

void CEntity::update( size_t timestep )
{
	m_position_previous = m_position;
	m_orientation_previous = m_orientation;

	// The process[...] functions return 'true' if the order at the top of the stack
	// still needs to be (re-)evaluated; else 'false' to terminate the processing of
	// this entity in this timestep.

	while( !m_orderQueue.empty() )
	{
		CEntityOrder* current = &m_orderQueue.front();

		m_transition = ( current->m_type != m_lastState );

		if( m_transition ) 
		{
			CEntity* target = NULL;
			if( current->m_data[0].entity )
				target = &( *( current->m_data[0].entity ) );
			
			CVector3D worldPosition = (CVector3D)current->m_data[0].location;

			CEventOrderTransition evt( m_lastState, current->m_type, target, worldPosition );

			if( !DispatchEvent( &evt ) )
			{
				m_orderQueue.pop_front();
				continue;
			}
			else if( target )
			{
				current->m_data[0].location = worldPosition;
				current->m_data[0].entity = target->me;
			}

			m_lastState = current->m_type;
		}

		switch( current->m_type )
		{
		case CEntityOrder::ORDER_GOTO_NOPATHING:
		case CEntityOrder::ORDER_GOTO_COLLISION:
		case CEntityOrder::ORDER_GOTO_SMOOTHED:
			if( processGotoNoPathing( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_ATTACK_MELEE:
			if( processAttackMeleeNoPathing( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_ATTACK_MELEE_NOPATHING:
			if( processAttackMeleeNoPathing( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_GOTO:
			if( processGoto( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_PATROL:
			if( processPatrol( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_PATH_END_MARKER:
			m_orderQueue.pop_front();
			break;
		default:
			assert( 0 && "Invalid entity order" );
		}
	}

	if( m_extant )
	{
		if( ( m_lastState != -1 ) || !m_actor->GetModel()->GetAnimation() )
			m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_IdleAnim );
	}
	else if( !m_actor->GetModel()->GetAnimation() )
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_CorpseAnim );

	if( m_lastState != -1 )
	{
		CEntity* d0;
		CVector3D d1;
		CEventOrderTransition evt( m_lastState, -1, d0, d1 );
		DispatchEvent( &evt );

		m_lastState = -1;
	}
}

void CEntity::Initialize()
{
	CEventInitialize evt;
	DispatchEvent( &evt );
}

void CEntity::Tick()
{
	CEventTick evt;
	DispatchEvent( &evt );
}

void CEntity::Damage( CDamageType& damage, CEntity* inflictor )
{
	CEventDamage evt( inflictor, &damage );
	DispatchEvent( &evt );
}

/*
void CEntity::dispatch( const CMessage* msg )
{
	
	switch( msg->type )
	{
	case CMessage::EMSG_TICK:
	{
		CEventTick Tick;
		DispatchEvent( &Tick );
		break;
	}
	case CMessage::EMSG_INIT:
	{
		CEventInitialize Init;
		if( !DispatchEvent( &Init ) )
			break;

		if( m_base->m_Tag == CStrW( L"Prometheus Dude" ) )
		{
			if( getCollisionObject( this ) )
			{
				// Prometheus telefragging. (Appeared inside another object)
				kill();
				return;
			}
		}
		break;
	}
	case CMessage::EMSG_ORDER:
		CMessageOrder* m;
		m = (CMessageOrder*)msg;
		if( !m->queue )
			clearOrders();
		pushOrder( m->order );
		break;
	case CMessage::EMSG_DAMAGE:
		CEntityOrder* o;
	}
}
*/

bool CEntity::DispatchEvent( CScriptEvent* evt )
{
	return( m_EventHandlers[evt->m_TypeCode].DispatchEvent( GetScript(), evt ) );
}

void CEntity::clearOrders()
{
	m_orderQueue.clear();
}
			
void CEntity::pushOrder( CEntityOrder& order )
{
	m_orderQueue.push_back( order );
}

int CEntity::defaultOrder( CEntity* orderTarget )
{
	CEventTargetChanged evt( orderTarget );
	DispatchEvent( &evt );
	return( evt.m_defaultAction );
}

bool CEntity::acceptsOrder( int orderType, CEntity* orderTarget )
{
	CEventPrepareOrder evt( orderTarget, orderType );
	return( DispatchEvent( &evt ) );

	/*
	// Hardcoding...
	switch( orderType )
	{
	case CEntityOrder::ORDER_GOTO:
	case CEntityOrder::ORDER_PATROL:
		return( m_speed > 0.0f );
	case CEntityOrder::ORDER_ATTACK_MELEE:
		return( orderTarget && ( m_meleeRange > 0.0f ) );
	}
	return( false );
	*/
}

void CEntity::repath()
{
	CVector2D destination;
	if( m_orderQueue.empty() ) return;

	while( !m_orderQueue.empty() &&
		( ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_COLLISION )
		|| ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_NOPATHING )
		|| ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_SMOOTHED ) ) )
	{
		destination = m_orderQueue.front().m_data[0].location;
		m_orderQueue.pop_front();
	}
	g_Pathfinder.requestPath( me, destination );
}

void CEntity::reorient()
{
	m_orientation = m_graphics_orientation;
	m_ahead.x = sin( m_orientation );
	m_ahead.y = cos( m_orientation );
	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*)m_bounds)->setOrientation( m_ahead );
	updateActorTransforms();
}
	
void CEntity::teleport()
{
	m_position = m_graphics_position;
	m_bounds->setPosition( m_position.X, m_position.Z );
	repath();
}

void CEntity::checkSelection()
{
	if( m_selected )
	{
		if( !g_Selection.isSelected( me ) )
			g_Selection.addSelection( me );
	}
	else
	{
		if( g_Selection.isSelected( me ) )
			g_Selection.removeSelection( me );
	}
}

void CEntity::checkGroup()
{
	g_Selection.changeGroup( me, -1 ); // Ungroup
	if( ( m_grouped >= 0 ) && ( m_grouped < MAX_GROUPS ) )
		g_Selection.changeGroup( me, m_grouped );		
}

void CEntity::interpolate( float relativeoffset )
{
	m_graphics_position = Interpolate<CVector3D>( m_position_previous, m_position, relativeoffset );
	
	// Avoid wraparound glitches for interpolating angles.
	while( m_orientation < m_orientation_previous - PI )
		m_orientation_previous -= 2 * PI;
	while( m_orientation > m_orientation_previous + PI )
		m_orientation_previous += 2 * PI;

	m_graphics_orientation = Interpolate<float>( m_orientation_previous, m_orientation, relativeoffset );
	snapToGround();
	updateActorTransforms();
}

void CEntity::render()
{	
	CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
	
	if( !m_orderQueue.empty() )
	{
		std::deque<CEntityOrder>::iterator it;
		CBoundingObject* destinationCollisionObject;
		float x0, y0, x, y;

		x = m_orderQueue.front().m_data[0].location.x;
		y = m_orderQueue.front().m_data[0].location.y;

		for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
		{
			if( it->m_type == CEntityOrder::ORDER_PATROL )
				break;
			x = it->m_data[0].location.x;
			y = it->m_data[0].location.y;
		}
		destinationCollisionObject = getContainingObject( CVector2D( x, y ) );

		glShadeModel( GL_FLAT );
		glBegin( GL_LINE_STRIP );
		
		

		glVertex3f( m_position.X, m_position.Y + 0.25f, m_position.Z );

		
		x = m_position.X;
		y = m_position.Z;
		
		for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
		{
			x0 = x; y0 = y;
			x = it->m_data[0].location.x;
			y = it->m_data[0].location.y;
			rayIntersectionResults r;
			CVector2D fwd( x - x0, y - y0 );
			float l = fwd.length();
			fwd = fwd.normalize();
			CVector2D rgt = fwd.beta();
			if( getRayIntersection( CVector2D( x0, y0 ), fwd, rgt, l, m_bounds->m_radius, destinationCollisionObject, &r ) )
			{
				glEnd();
				glBegin( GL_LINES );
				glColor3f( 1.0f, 0.0f, 0.0f );
				glVertex3f( x0 + fwd.x * r.distance, pTerrain->getExactGroundLevel( x0 + fwd.x * r.distance, y0 + fwd.y * r.distance ) + 0.25f, y0 + fwd.y * r.distance );
				glVertex3f( r.position.x, pTerrain->getExactGroundLevel( r.position.x, r.position.y ) + 0.25f, r.position.y );
				glEnd();
				glBegin( GL_LINE_STRIP );
				glVertex3f( x0, pTerrain->getExactGroundLevel( x0, y0 ), y0 );
			}
			switch( it->m_type )
			{
			case CEntityOrder::ORDER_GOTO:
				glColor3f( 1.0f, 0.0f, 0.0f ); break;
			case CEntityOrder::ORDER_GOTO_COLLISION:
				glColor3f( 1.0f, 0.5f, 0.5f ); break;
			case CEntityOrder::ORDER_GOTO_NOPATHING:
			case CEntityOrder::ORDER_GOTO_SMOOTHED:
				glColor3f( 0.5f, 0.5f, 0.5f ); break;
			case CEntityOrder::ORDER_PATROL:
				glColor3f( 0.0f, 1.0f, 0.0f ); break;
			default:
				continue;
			}
			
			glVertex3f( x, pTerrain->getExactGroundLevel( x, y ) + 0.25f, y );
		}

		glEnd();
		glShadeModel( GL_SMOOTH );
	}
	
	glColor3f( 1.0f, 1.0f, 1.0f );
	if( getCollisionObject( this ) ) glColor3f( 0.5f, 0.5f, 1.0f );
	m_bounds->render( pTerrain->getExactGroundLevel( m_position.X, m_position.Z ) + 0.25f ); //m_position.Y + 0.25f );
}

void CEntity::renderSelectionOutline( float alpha )
{
	CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
	
	if( !m_bounds ) return;

	glColor4f( 1.0f, 1.0f, 1.0f, alpha );
	if( getCollisionObject( this ) ) glColor4f( 1.0f, 0.5f, 0.5f, alpha );
	
	glBegin( GL_LINE_LOOP );

	CVector3D pos = m_graphics_position;

	switch( m_bounds->m_type )
	{
	case CBoundingObject::BOUND_CIRCLE:
	{
		float radius = ((CBoundingCircle*)m_bounds)->m_radius;
		for( int i = 0; i < SELECTION_CIRCLE_POINTS; i++ )
		{
			float ang = i * 2 * PI / (float)SELECTION_CIRCLE_POINTS;
			float x = pos.X + radius * sin( ang );
			float y = pos.Z + radius * cos( ang );
#ifdef SELECTION_TERRAIN_CONFORMANCE
			glVertex3f( x, pTerrain->getExactGroundLevel( x, y ) + 0.25f, y );
#else
			glVertex3f( x, pos.Y + 0.25f, y );
#endif
		}
		break;
	}
	case CBoundingObject::BOUND_OABB:
	{
		CVector2D p, q;
		CVector2D u, v;
		q.x = pos.X; q.y = pos.Z;
		float h = ((CBoundingBox*)m_bounds)->m_h;
		float w = ((CBoundingBox*)m_bounds)->m_w;

		u.x = sin( m_graphics_orientation );
		u.y = cos( m_graphics_orientation );
		v.x = u.y;
		v.y = -u.x;

#ifdef SELECTION_TERRAIN_CONFORMANCE
		for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
		{
			p = q + u * h + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
			glVertex3f( p.x, pTerrain->getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
		{
			p = q + u * ( h * (float)i / (float)SELECTION_BOX_POINTS ) - v * w;
			glVertex3f( p.x, pTerrain->getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
		{
			p = q - u * h + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
			glVertex3f( p.x, pTerrain->getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
		{
			p = q + u * ( h * (float)i / (float)SELECTION_BOX_POINTS ) + v * w;
			glVertex3f( p.x, pTerrain->getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}
#else
			p = q + u * h + v * w;
			glVertex3f( p.x, pTerrain->getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );

			p = q + u * h - v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );

			p = q - u * h + v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );

			p = q + u * h + v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
#endif


		break;
	}
	}

	glEnd();
	
}
/*

	Scripting interface

*/

// Scripting initialization

void CEntity::ScriptingInit()
{
	AddMethod<jsval, &CEntity::ToString>( "toString", 0 );
	AddMethod<bool, &CEntity::OrderSingle>( "order", 1 );
	AddMethod<bool, &CEntity::OrderQueued>( "orderQueued", 1 );
	AddMethod<bool, &CEntity::Kill>( "kill", 0 );
	AddMethod<bool, &CEntity::Damage>( "damage", 1 );
	AddMethod<bool, &CEntity::IsIdle>( "isIdle", 0 );
	CJSObject<CEntity>::ScriptingInit( "Entity", Construct, 2 );
}

// Script constructor

JSBool CEntity::Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 2 );

	CBaseEntity* baseEntity = NULL;
	CVector3D position;
	float orientation = 0.0f;

	JSObject* jsBaseEntity = JSVAL_TO_OBJECT( argv[0] );
	CStrW templateName;

	if( !JSVAL_IS_OBJECT( argv[0] ) || !( baseEntity = ToNative<CBaseEntity>( cx, jsBaseEntity ) ) )
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
		baseEntity = g_EntityTemplateCollection.getTemplate( templateName );
	}

	if( !baseEntity )
	{
		*rval = JSVAL_NULL;
		JS_ReportError( cx, "No such template: %s", CStr8(templateName).c_str() );
		return( JS_TRUE );
	}

	JSI_Vector3D::Vector3D_Info* jsVector3D = NULL;
	if( JSVAL_IS_OBJECT( argv[1] ) && ( jsVector3D = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( argv[1] ), &JSI_Vector3D::JSI_class, NULL ) ) )
	{
		position = *( jsVector3D->vector );
	}
	if( argc >= 3 )
	{
		try
		{
			orientation = (float)g_ScriptingHost.ValueToDouble( argv[2] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			// TODO: Net-safe random for this parameter.
			orientation = 0.0f;
		}
	}

	HEntity handle = g_EntityManager.create( baseEntity, position, orientation );

	handle->Initialize();

	*rval = ToJSVal<CEntity>( *handle );
	return( JS_TRUE );
}

// Script-bound methods

jsval CEntity::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Entity: %ls]", m_base->m_Tag.c_str() );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

bool CEntity::Order( JSContext* cx, uintN argc, jsval* argv, bool Queued )
{
	// This needs to be sorted (uses Scheduler rather than network messaging)
	assert( argc >= 1 );

	int orderCode;

	try
	{
		orderCode = g_ScriptingHost.ValueToInt( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid order type" );
		return( false );
	}

	CEntityOrder newOrder;

	(int&)newOrder.m_type = orderCode;

	switch( orderCode )
	{
	case CEntityOrder::ORDER_GOTO:
	case CEntityOrder::ORDER_PATROL:
		if( argc < 3 )
		{
			JS_ReportError( cx, "Too few parameters" );
			return( false );
		}
		try
		{
			newOrder.m_data[0].location.x = (float)g_ScriptingHost.ValueToDouble( argv[1] );
			newOrder.m_data[0].location.y = (float)g_ScriptingHost.ValueToDouble( argv[2] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			JS_ReportError( cx, "Invalid location" );
			return( false );
		}
		break;
	case CEntityOrder::ORDER_ATTACK_MELEE:
		if( argc < 1 )
		{
			JS_ReportError( cx, "Too few parameters" );
			return( false );
		}
		CEntity* target;
		target = ToNative<CEntity>( argv[1] );
		if( !target )
		{
			JS_ReportError( cx, "Invalid target" );
			return( false );
		}
		newOrder.m_data[0].entity = target->me;
		break;
	default:
		JS_ReportError( cx, "Invalid order type" );
		return( false );
	}

	if( !Queued )
		clearOrders();
	pushOrder( newOrder );

	return( true );
}

bool CEntity::Damage( JSContext* cx, uintN argc, jsval* argv )
{
	CEntity* inflictor = NULL;

	if( argc >= 4 )
		inflictor = ToNative<CEntity>( argv[3] );

	if( argc >= 3 )
	{
		CDamageType dmgType( ToPrimitive<float>( argv[0] ), ToPrimitive<float>( argv[1] ), ToPrimitive<float>( argv[2] ) );
		Damage( dmgType, inflictor );
		return( true );
	}
	
	if( argc >= 2 )
		inflictor = ToNative<CEntity>( argv[1] );

	// If it's a DamageType, use that. Otherwise, see if it's a float, if so, use
	// that as the 'typeless' unblockable damage type.

	CDamageType* dmg = ToNative<CDamageType>( argv[0] );

	if( !dmg )
	{
		float dmgN;
		if( !ToPrimitive<float>( cx, argv[0], dmgN ) )
			return( false );
			
		CDamageType dmgType( dmgN );

		Damage(	dmgType, inflictor );
		return( true );	
	}

	Damage( *dmg, inflictor );
	return( true );
}

bool CEntity::Kill( JSContext* cx, uintN argc, jsval* argv )
{
	// Change this entity's template to the corpse entity - but note
	// we don't fiddle with the actors or bounding information that we 
	// usually do when changing templates.

	CBaseEntity* corpse = g_EntityTemplateCollection.getTemplate( m_corpse );
	if( corpse )
	{
		m_base = corpse;
		SetBase( m_base );
	}

	if( m_extant )
	{
		if( m_bounds )
			delete( m_bounds );
		m_bounds = NULL;

		m_extant = false;
	}
	
	g_Selection.removeAll( me );

	clearOrders();

	m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_DeathAnim, true );

	return( true );
}
