// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "precompiled.h"

#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"

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
	
	AddProperty( L"template", (CBaseEntity**)&m_base, false, (NotifyFn)loadBase );
	AddProperty( L"actions.move.speed", &m_speed );
	AddProperty( L"selected", &m_selected, false, (NotifyFn)checkSelection );
	AddProperty( L"group", &m_grouped, false, (NotifyFn)checkGroup );
	AddProperty( L"extant", &m_extant, false, (NotifyFn)checkExtant );
	AddProperty( L"actions.move.turningradius", &m_turningRadius );
	AddProperty( L"position", &m_graphics_position, false, (NotifyFn)teleport );
	AddProperty( L"orientation", &m_graphics_orientation, false, (NotifyFn)reorient );

	
	for( int t = 0; t < EVENT_LAST; t++ )
		AddProperty( EventNames[t], &m_EventHandlers[t] );
	
	// Set our parent unit and build us an actor.
	m_actor = NULL;
	m_bounds = NULL;

	m_moving = false;

	m_base = base;
	
	loadBase();
	
	if( m_bounds )
		m_bounds->setPosition( m_position.X, m_position.Z );
	
	m_position_previous = m_position;
	m_orientation_previous = m_orientation;

	m_graphics_position = m_position;
	m_graphics_orientation = m_orientation;

	m_extant = true;
	m_extant_mirror = true;

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

	m_extant = false;
	m_extant_mirror = false;
	
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

	while( !m_orderQueue.empty() )
	{
		CEntityOrder* current = &m_orderQueue.front();

		switch( current->m_type )
		{
		case CEntityOrder::ORDER_GOTO_NOPATHING:
		case CEntityOrder::ORDER_GOTO_COLLISION:
		case CEntityOrder::ORDER_GOTO_SMOOTHED:
			if( processGotoNoPathing( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_GOTO:
			if( processGoto( current, timestep ) ) break;
			return;
		case CEntityOrder::ORDER_PATROL:
			if( processPatrol( current, timestep ) ) break;
			return;
		default:
			assert( 0 && "Invalid entity order" );
		}
	}

	if( m_moving )
	{
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_IdleAnim );
		m_moving = false;
	}
}

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
		DispatchEvent( &Init );
		if( m_base->m_Tag == CStrW( L"Prometheus Dude" ) )
		{
			if( getCollisionObject( this ) )
			{
				// Prometheus telefragging. (Appeared inside another object)
				kill();
				return;
			}
			/*
			std::vector<HEntity>* waypoints = g_EntityManager.matches( isWaypoint );
			while( !waypoints->empty() )
			{
				CEntityOrder patrol;
				size_t id = rand() % waypoints->size();
				std::vector<HEntity>::iterator it = waypoints->begin();
				it += id;
				HEntity waypoint = *it;
				patrol.m_type = CEntityOrder::ORDER_PATROL;
				patrol.m_data[0].location.x = waypoint->m_position.X;
				patrol.m_data[0].location.y = waypoint->m_position.Z;
				pushOrder( patrol );
				waypoints->erase( it );
			}
			delete( waypoints );
			*/
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
	}
}

bool CEntity::DispatchEvent( CScriptEvent* evt )
{
	m_EventHandlers[evt->m_TypeCode].DispatchEvent( GetScript(), evt );
	return( false );
}

void CEntity::clearOrders()
{
	m_orderQueue.clear();
}
			
void CEntity::pushOrder( CEntityOrder& order )
{
	m_orderQueue.push_back( order );
}

bool CEntity::acceptsOrder( int orderType, CEntity* orderTarget )
{
	// Hardcoding...
	switch( orderType )
	{
	case CEntityOrder::ORDER_GOTO:
	case CEntityOrder::ORDER_PATROL:
		return( m_speed > 0.0f );
	}
	return( false );
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

void CEntity::checkExtant()
{
	if( m_extant && !( (bool)m_extant_mirror ) )
		kill();
	// Sorry. Dead stuff stays dead.
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
	AddMethod<jsval, ToString>( "toString", 0 );
	AddMethod<bool, OrderSingle>( "order", 1 );
	AddMethod<bool, OrderQueued>( "orderQueued", 1 );
	CJSObject<CEntity, true>::ScriptingInit( "Entity", Construct, 2 );
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

	CMessage message( CMessage::EMSG_INIT );
	handle->dispatch( &message );

	*rval = ToJSVal<CEntity>( *handle );
	return( JS_TRUE );
}

// Script-bound methods

jsval CEntity::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	utf16_t buffer[256];
	swprintf( buffer, 256, L"[object Entity: %ls]", m_base->m_Tag.c_str() );
	buffer[255] = 0;
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, buffer ) ) );
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
		g_Scheduler.pushFrame( ORDER_DELAY, me, new CMessageOrder( newOrder, Queued ) );
		return( true );
	default:
		JS_ReportError( cx, "Invalid order type" );
		return( false );
	}
}

