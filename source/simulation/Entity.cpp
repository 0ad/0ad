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

extern CCamera g_Camera;

CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation )
{
	m_position = position;
	m_orientation = orientation;

	m_ahead.x = sin( m_orientation );
	m_ahead.y = cos( m_orientation );
	
	m_base.associate( this, "template", ( void( IPropertyOwner::* )() )&CEntity::loadBase );
	m_name.associate( this, "name" );
	m_speed.associate( this, "speed" );
	m_selected.associate( this, "selected", ( void( IPropertyOwner::* )() )&CEntity::checkSelection );
	m_grouped_mirror.associate( this, "group", ( void( IPropertyOwner::* )() )&CEntity::checkGroup );
	m_extant_mirror.associate( this, "extant", ( void( IPropertyOwner::* )() )&CEntity::checkExtant );
	m_turningRadius.associate( this, "turningRadius" );
	m_graphics_position.associate( this, "position", ( void( IPropertyOwner::* )() )&CEntity::teleport );
	m_graphics_orientation.associate( this, "orientation", ( void( IPropertyOwner::* )() )&CEntity::reorient );

	// Set our parent unit and build us an actor.
	m_actor = NULL;
	m_bounds = NULL;

	m_base = base;
	
	loadBase();
	
	// We can now freely admit that this exists.
	m_extant = true;
	m_extant_mirror = true;

	m_selected = false;
	m_grouped = 255;
	m_grouped_mirror = 0;
}

CEntity::~CEntity()
{
	for( size_t i = 0; i < m_base->m_inheritors.size(); i++ )
		if( m_base->m_inheritors[i] == this )
			m_base->m_inheritors.erase( m_base->m_inheritors.begin() + i );

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

	m_base->m_inheritors.push_back( this );
	rebuild();

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
	g_Selection.removeAll( this );

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

bool isWaypoint( CEntity* e )
{
	return( e->m_base->m_name == CStr( "Waypoint" ) );
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

float CEntity::getExactGroundLevel( float x, float y )
{
	x /= 4.0f;
	y /= 4.0f;

	int xi = (int)floor( x );
	int yi = (int)floor( y );
	float xf = x - (float)xi;
	float yf = y - (float)yi;

	u16* heightmap = g_Terrain.GetHeightMap();
	unsigned long mapsize = g_Terrain.GetVerticesPerSide();

	assert( ( xi >= 0 ) && ( xi < (int)mapsize ) && ( yi >= 0 ) && ( yi < (int)mapsize ) );

	// In non-debug builds, do something nicer than crashing
	if (! ( ( xi >= 0 ) && ( xi < (int)mapsize ) && ( yi >= 0 ) && ( yi < (int)mapsize ) ) )
	{
		return 0.0f;
	}

	float h00 = heightmap[yi*mapsize + xi];
	float h01 = heightmap[yi*mapsize + xi + mapsize];
	float h10 = heightmap[yi*mapsize + xi + 1];
	float h11 = heightmap[yi*mapsize + xi + mapsize + 1];

	/*
	if( xf < ( 1.0f - yf ) )
	{
		return( HEIGHT_SCALE * ( ( 1 - xf - yf ) * h00 + xf * h10 + yf * h01 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( xf + yf - 1 ) * h11 + ( 1 - xf ) * h01 + ( 1 - yf ) * h10 ) );
	*/

	/*
	if( xf > yf ) 
	{
		return( HEIGHT_SCALE * ( ( 1 - xf ) * h00 + ( xf - yf ) * h10 + yf * h11 ) );
	}
	else
		return( HEIGHT_SCALE * ( ( 1 - yf ) * h00 + ( yf - xf ) * h01 + xf * h11 ) );
	*/

	return( HEIGHT_SCALE * ( ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ) );

}

void CEntity::snapToGround()
{
	m_graphics_position.Y = getExactGroundLevel( m_graphics_position.X, m_graphics_position.Z );
}

void CEntity::update( float timestep )
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
	if( m_actor->GetModel()->GetAnimation() != m_actor->GetObject()->m_IdleAnim )
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_IdleAnim );
}

void CEntity::dispatch( const CMessage* msg )
{
	switch( msg->type )
	{
	case CMessage::EMSG_TICK:
		break;
	case CMessage::EMSG_INIT:
		if( m_base->m_name == CStr( "Prometheus Dude" ) )
		{
			if( getCollisionObject( this ) )
			{
				// Prometheus telefragging. (Appeared inside another object)
				kill();
				return;
			}
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
		}
		break;
	case CMessage::EMSG_ORDER:
		CMessageOrder* m;
		m = (CMessageOrder*)msg;
		if( !m->queue )
			clearOrders();
		pushOrder( m->order );
		break;
	}
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
		if( !g_Selection.isSelected( this ) )
			g_Selection.addSelection( this );
	}
	else
	{
		if( g_Selection.isSelected( this ) )
			g_Selection.removeSelection( this );
	}
}

void CEntity::checkGroup()
{
	if( m_grouped != m_grouped_mirror )
	{
		if( ( m_grouped_mirror >= 0 ) && ( m_grouped_mirror <= 10 ) )
		{
			u8 newgroup = m_grouped_mirror;
			if( newgroup == 0 ) newgroup = 255;
			if( newgroup == 10 ) newgroup = 0;

			g_Selection.changeGroup( this, newgroup );
		}
		else
			m_grouped_mirror = m_grouped;
	}
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
				glVertex3f( x0 + fwd.x * r.distance, getExactGroundLevel( x0 + fwd.x * r.distance, y0 + fwd.y * r.distance ) + 0.25f, y0 + fwd.y * r.distance );
				glVertex3f( r.position.x, getExactGroundLevel( r.position.x, r.position.y ) + 0.25f, r.position.y );
				glEnd();
				glBegin( GL_LINE_STRIP );
				glVertex3f( x0, getExactGroundLevel( x0, y0 ), y0 );
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
			
			glVertex3f( x, getExactGroundLevel( x, y ) + 0.25f, y );
		}

		glEnd();
		glShadeModel( GL_SMOOTH );
	}
	
	glColor3f( 1.0f, 1.0f, 1.0f );
	if( getCollisionObject( this ) ) glColor3f( 0.5f, 0.5f, 1.0f );
	m_bounds->render( getExactGroundLevel( m_position.X, m_position.Z ) + 0.25f ); //m_position.Y + 0.25f );
}

void CEntity::renderSelectionOutline( float alpha )
{
	if( !m_bounds ) return;

	glColor4f( 1.0f, 1.0f, 1.0f, alpha );
	
	glBegin( GL_LINE_LOOP );

	CVector3D pos = m_graphics_position;
	pos.X += m_bounds->m_offset.x;
	pos.Z += m_bounds->m_offset.y;

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
			glVertex3f( x, getExactGroundLevel( x, y ) + 0.25f, y );
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
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
		{
			p = q + u * ( h * (float)i / (float)SELECTION_BOX_POINTS ) - v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
		{
			p = q - u * h + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}

		for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
		{
			p = q + u * ( h * (float)i / (float)SELECTION_BOX_POINTS ) + v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );
		}
#else
			p = q + u * h + v * w;
			glVertex3f( p.x, getExactGroundLevel( p.x, p.y ) + 0.25f, p.y );

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

