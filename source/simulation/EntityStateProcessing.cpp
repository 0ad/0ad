// Entity state-machine processing code.

#include "Entity.h"
#include "Model.h"

#include "Collision.h"
#include "PathfindEngine.h"

bool CEntity::processGotoNoPathing( CEntityOrder* current, float timestep )
{
	CVector2D delta;
	delta.x = (float)current->m_data[0].location.x - m_position.X;
	delta.y = (float)current->m_data[0].location.y - m_position.Z;

	float len = delta.length();

	if( len < 0.1f )
	{
		m_orderQueue.pop_front();
		return( false );
	}

	m_ahead = delta / len;

	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*)m_bounds)->setOrientation( m_ahead );

	
	float scale = timestep * m_speed;

	if( scale > len )
		scale = len;

	delta = m_ahead * scale;

	m_position.X += delta.x;
	m_position.Z += delta.y;
	m_bounds->setPosition( m_position.X, m_position.Z );

	HEntity collide = getCollisionObject( this );
	
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
			return( false );
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
				return( false );
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
				return( false );
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
			return( false );
		}
	}

	snapToGround();
	updateActorTransforms();

	return( false );
}

bool CEntity::processGoto( CEntityOrder* current, float timestep )
{
	CVector2D path_to = current->m_data[0].location;
	m_orderQueue.pop_front();
	if( m_actor->GetModel()->GetAnimation() != m_actor->GetObject()->m_WalkAnim )
	{
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_WalkAnim );
		m_actor->GetModel()->Update( ( rand() * 1000.0f ) / 1000.0f );
	}
	g_Pathfinder.requestPath( me, path_to );
	return( true );
}

bool CEntity::processPatrol( CEntityOrder* current, float timestep )
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
	return( true );
}