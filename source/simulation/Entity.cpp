// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"

#include "Renderer.h"
#include "Model.h"
#include "Terrain.h"

CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation )
{
	// Set our parent unit and build us an actor.

	m_base = base;
	m_actor = new CUnit;

	m_actor->m_Object = m_base->m_actorObject;
	m_actor->m_Model = ( m_actor->m_Object->m_Model ) ? m_actor->m_Object->m_Model->Clone() : NULL;

	// Register the actor with the renderer.

	g_UnitMan.AddUnit( m_actor );

	// Set up our instance data

	m_speed = m_base->m_speed;
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
	m_properties["speed"] = &m_speed;
	m_properties["orientation"] = &m_orientation;
	m_properties["position"] = &m_position;
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

	m_actor->m_Model->SetTransform( m );
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
			{
				CVector2D delta;
				delta.x = (float)current->m_data[0].location.x - m_position.X;
				delta.y = (float)current->m_data[0].location.y - m_position.Z;

				m_ahead = delta.normalize();

				if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
					((CBoundingBox*)m_bounds)->setOrientation( m_ahead );

				float len = delta.length();

				float scale = timestep * m_speed;

				if( scale > len )
					scale = len;

				delta = m_ahead * scale;

				m_position.X += delta.x;
				m_position.Z += delta.y;
				m_bounds->setPosition( m_position.X, m_position.Z );

				HEntity collide = getCollisionObject();
				
				if( collide )
				{	
					// Hit something. Take a step back.
					m_position.X -= delta.x;
					m_position.Z -= delta.y;

					m_bounds->setPosition( m_position.X, m_position.Z );

					// Are we still hitting it?
					if( collide->m_bounds->intersects( m_bounds ) )
					{
						// Oh dear. Most likely explanation is that this unit was created
						// within the bounding area of another entity.
						// Try a little boost of speed, to help resolve the situation more quickly.
						m_position.X += delta.x * 2.0f;
						m_position.Z += delta.y * 2.0f;
						m_bounds->setPosition( m_position.X, m_position.Z );
						return;
					}
				
					if( collide->m_bounds->m_type == CBoundingObject::BOUND_OABB )
					{
						// And it's square.
						// TODO: Implement this case properly.

						// HACK: See if this thing we've hit is likely to be our destination. If so, just skip to our next waypoint.
						// Otherwise, turn right (as with circle collisions)

						if( len < collide->m_bounds->m_radius * 2.0f )
						{
							m_orderQueue.pop_front();
							return;
						}
						else
						{
							CEntityOrder avoidance;
							avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
							CVector2D right;
							right.x = m_ahead.y; right.y = -m_ahead.x;
							CVector2D avoidancePosition = collide->m_bounds->m_pos + right * ( collide->m_bounds->m_radius * 2.5f );
							avoidance.m_data[0].location = avoidancePosition;
							if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
								m_orderQueue.pop_front();
							m_orderQueue.push_front( avoidance );
							return;
						}
						
					}
					else
					{
						// A circle.
						// TODO: Implement this properly.
						// Try turning right.
						CEntityOrder avoidance;
						avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
						CVector2D right;
						right.x = m_ahead.y; right.y = -m_ahead.x;
						CVector2D avoidancePosition = collide->m_bounds->m_pos + right * ( collide->m_bounds->m_radius * 2.5f );
						avoidance.m_data[0].location = avoidancePosition;
						if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
							m_orderQueue.pop_front();
						m_orderQueue.push_front( avoidance );
						return;
					}
				}

				snapToGround();
				updateActorTransforms();
				if( len < 0.1f )
					m_orderQueue.pop_front();
				return;
				break;
			}
		case CEntityOrder::ORDER_GOTO:
			{
				CEntityOrder pathfind_solution;
				pathfind_solution.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
				pathfind_solution.m_data[0] = current->m_data[0];
				m_orderQueue.pop_front();
				m_orderQueue.push_front( pathfind_solution );
				if( m_actor->m_Model->GetAnimation() != m_actor->m_Object->m_WalkAnim )
				{
					m_actor->m_Model->SetAnimation( m_actor->m_Object->m_WalkAnim );
					m_actor->m_Model->Update( ( rand() * 1000.0f ) / 1000.0f );
				}
				break;
			}
		case CEntityOrder::ORDER_PATROL:
			{
				CEntityOrder this_segment;
				CEntityOrder repeat_patrol;
				this_segment.m_type = CEntityOrder::ORDER_GOTO;
				this_segment.m_data[0] = current->m_data[0];
				repeat_patrol.m_type = CEntityOrder::ORDER_PATROL;
				repeat_patrol.m_data[0] = current->m_data[0];
				m_orderQueue.pop_front();
				m_orderQueue.push_front( this_segment );
				m_orderQueue.push_back( repeat_patrol );
				break;
			}
		default:
			assert( 0 && "Invalid entity order" );
		}
	}
	if( m_actor->m_Model->GetAnimation() != m_actor->m_Object->m_IdleAnim )
		m_actor->m_Model->SetAnimation( m_actor->m_Object->m_IdleAnim );
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

	glColor3f( 1.0f, 1.0f, 1.0f );
	if( getCollisionObject() ) glColor3f( 0.5f, 0.5f, 1.0f );
	m_bounds->render( m_position.Y + 0.25f );
}

HEntity CEntity::getCollisionObject()
{
	if( !m_bounds ) return( HEntity() );
	std::vector<HEntity>* entities = g_EntityManager.getActive();
	std::vector<HEntity>::iterator it;

	for( it = entities->begin(); it != entities->end(); it++ )
	{
		if( *it == me ) continue;
		if( (*it)->m_bounds )
			if( m_bounds->intersects( (*it)->m_bounds ) )
			{
				HEntity collisionObject = *it;
				delete( entities );
				return( collisionObject );
			}
	}

	delete( entities );
	return( HEntity() );
}

HEntity CEntity::getCollisionObject( float x, float y )
{
	float _x = m_bounds->m_pos.x;
	float _y = m_bounds->m_pos.y;
	m_bounds->setPosition( x, y );
	HEntity _e = getCollisionObject();
	m_bounds->setPosition( _x, _y );
	return( _e );
}

void PASAPScenario()
{
	// Got rid of all the hardcoding that was here.
}