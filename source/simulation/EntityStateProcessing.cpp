// Entity state-machine processing code.

#include "precompiled.h"

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

	// ... 'Are we there yet?' ...

	if( len < 0.1f )
	{
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
		{
			repath();
		}
		else
			m_orderQueue.pop_front();
		return( false );
	}

	// Curve smoothing.
	// Here there be trig.

	if( current->m_type != CEntityOrder::ORDER_GOTO_SMOOTHED )
	{
		// We can only really attempt to smooth paths the pathfinder
		// has flagged for us. If the turning-radius calculations are
		// applied to other types of waypoint, wierdness happens.
		// Things like an entity trying to walk to a point inside
		// his turning radius (which he can't do directly, so he'll
		// orbit the point indefinately), or just massive deviations
		// making the paths we calculate useless.
		// It's also painful trying to watch two entities resolve their
		// collision when they're both bound by turning constraints.
		m_ahead = delta / len;
		m_orientation = atan2( m_ahead.x, m_ahead.y );
	}
	else
	{
		m_targetorientation = atan2( delta.x, delta.y );

		float deltatheta = m_targetorientation - (float)m_orientation;
		while( deltatheta > PI ) deltatheta -= 2 * PI;
		while( deltatheta < -PI ) deltatheta += 2 * PI;

		if( fabs( deltatheta ) > 0.01f )
		{
			float maxTurningSpeed = ( m_speed / m_turningRadius ) * timestep;
			if( deltatheta > 0 )
			{
				m_orientation = m_orientation + MIN( deltatheta, maxTurningSpeed );
			}
			else
				m_orientation = m_orientation + MAX( deltatheta, -maxTurningSpeed );

			m_ahead.x = sin( m_orientation );
			m_ahead.y = cos( m_orientation );
		}
		else
		{
			m_ahead = delta / len;
			m_orientation = atan2( m_ahead.x, m_ahead.y );
		}
	}

	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*)m_bounds)->setOrientation( m_ahead );

	
	float scale = m_speed * timestep;

	if( scale > len )
		scale = len;

	delta = m_ahead * scale;

	m_position.X += delta.x;
	m_position.Z += delta.y;
	m_bounds->setPosition( m_position.X, m_position.Z );

	HEntity collide = getCollisionObject( this );
	
	if( collide )
	{	
		// Hit something. Is it our destination?
		if( collide->m_bounds->contains( current->m_data[0].location ) )
		{
			m_orderQueue.pop_front();
			return( false );
		}
		
		// No? Take a step back.
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
		
		// No? Path around it.
			
		CEntityOrder avoidance;
		avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
		CVector2D right;
		right.x = m_ahead.y; right.y = -m_ahead.x;
		CVector2D avoidancePosition;

		if( ( collide->m_bounds->m_pos - m_bounds->m_pos ).dot( right ) < 1 )
		{
			// Turn right.
			avoidancePosition = collide->m_bounds->m_pos + right * ( collide->m_bounds->m_radius + m_bounds->m_radius * 2.5f );
		}
		else
		{
			// Turn left.
			avoidancePosition = collide->m_bounds->m_pos - right * ( collide->m_bounds->m_radius + m_bounds->m_radius * 2.5f );
		}

		avoidance.m_data[0].location = avoidancePosition;
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
			m_orderQueue.pop_front();
		m_orderQueue.push_front( avoidance );
		return( false );

	}

	return( false );
}

bool CEntity::processGoto( CEntityOrder* current, float timestep )
{
	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_data[0].location;
	m_orderQueue.pop_front();
	if( ( path_to - pos ).length() < 0.1f ) 
		return( false );
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
