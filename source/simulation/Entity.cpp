// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"

#include "Renderer.h"
#include "Model.h"
#include "Terrain.h"

#include "Collision.h"

CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation )
{
	// Set our parent unit and build us an actor.

	m_base = base;
	m_actor = new CUnit(m_base->m_actorObject,m_base->m_actorObject->m_Model->Clone());

	// Register the actor with the renderer.

	g_UnitMan.AddUnit( m_actor );

	// Set up our instance data

	m_speed = m_base->m_speed;
	m_turningRadius = m_base->m_turningRadius;
	m_position = position;
	m_orientation = orientation;

	m_ahead.x = sin( orientation );
	m_ahead.y = cos( orientation );

	if( m_base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
	{
		m_bounds = new CBoundingCircle( m_position.X, m_position.Z, m_base->m_bound_circle );
	}
	else if( m_base->m_bound_type == CBoundingObject::BOUND_OABB )
	{
		m_bounds = new CBoundingBox( m_position.X, m_position.Z, m_ahead, m_base->m_bound_box );
	}
	
	snapToGround();
	updateActorTransforms();

	// Register the addresses of our native properties with the properties table
	
	m_properties["speed"].associate( &m_speed );
	m_properties["orientation"].associate( &m_orientation );
	m_properties["position"].associate( &m_position );
	
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
	
bool isWaypoint( CEntity* e )
{
	return( e->m_base->m_name == CStr( "Waypoint" ) );
}

void CEntity::updateActorTransforms()
{
	CMatrix3D m;
	
	m._11 = -m_ahead.y;	m._12 = 0.0f;	m._13 = -m_ahead.x;	m._14 = m_position.X;
	m._21 = 0.0f;		m._22 = 1.0f;	m._23 = 0.0f;		m._24 = m_position.Y;
	m._31 = m_ahead.x;	m._32 = 0.0f;	m._33 = -m_ahead.y;	m._34 = m_position.Z;
	m._41 = 0.0f;		m._42 = 0.0f;	m._43 = 0.0f;		m._44 = 1.0f;

	/* Equivalent to:	
		m.SetYRotation( m_orientation );
		m.Translate( m_position );
		But the matrix multiplication seemed such a waste when we already have a forward vector
	*/

	m_actor->GetModel()->SetTransform( m );
}

float CEntity::getExactGroundLevel( float x, float y )
{
	// TODO MT: If OK with Rich, move to terrain core. Once this works, that is.

	x /= 4.0f;
	y /= 4.0f;

	int xi = (int)floor( x );
	int yi = (int)floor( y );
	float xf = x - (float)xi;
	float yf = y - (float)yi;

	u16* heightmap = g_Terrain.GetHeightMap();
	unsigned long mapsize = g_Terrain.GetVerticesPerSide();

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
	m_position.Y = getExactGroundLevel( m_position.X, m_position.Z );
}

void CEntity::update( float timestep )
{
	while( !m_orderQueue.empty() )
	{
		CEntityOrder* current = &m_orderQueue.front();

		switch( current->m_type )
		{
		case CEntityOrder::ORDER_GOTO_NOPATHING:
		case CEntityOrder::ORDER_GOTO_COLLISION:
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

void CEntity::dispatch( CMessage* msg )
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
				g_EntityManager.kill( me );
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
	}
}
			
void CEntity::pushOrder( CEntityOrder& order )
{
	m_orderQueue.push_back( order );
}

void CEntity::render()
{
	// Rich! Help! ;)
	// We can loose this later on, I just need a way to see collision boxes temporarily
	
	if( !m_orderQueue.empty() )
	{
		glShadeModel( GL_FLAT );
		glBegin( GL_LINE_STRIP );
		
		std::deque<CEntityOrder>::iterator it;

		glVertex3f( m_position.X, m_position.Y + 0.25f /* 20.0f */, m_position.Z );

		for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
		{
			float x = it->m_data[0].location.x;
			float y = it->m_data[0].location.y;
			switch( it->m_type )
			{
			case CEntityOrder::ORDER_GOTO:
				glColor3f( 1.0f, 0.0f, 0.0f ); break;
			case CEntityOrder::ORDER_GOTO_COLLISION:
				glColor3f( 1.0f, 0.5f, 0.5f ); break;
			case CEntityOrder::ORDER_GOTO_NOPATHING:
				glColor3f( 0.5f, 0.5f, 0.5f ); break;
			default:
				glColor3f( 1.0f, 1.0f, 1.0f ); break;
			}
			glVertex3f( x, getExactGroundLevel( x, y ) + 0.25f /* 20.0f */, y );
		}

		glEnd();
		glShadeModel( GL_SMOOTH );
	}
	
	glColor3f( 1.0f, 1.0f, 1.0f );
	

	if( getCollisionObject( this ) ) glColor3f( 0.5f, 0.5f, 1.0f );
	m_bounds->render( m_position.Y + 0.25f /* 20.0f */ );

}

void PASAPScenario()
{
	// Got rid of all the hardcoding that was here.
}