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

	// janwas added EVIL HACK: BoundsChecker complains about NaNs
	// in atan2 and fabs => delta must be 0 somewhere.
	// currently skip over all math code that would break.
	// what's the real solution?
	if(len == 0.0f)
		goto small_delta;

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

		float deltatheta = m_targetorientation - m_orientation;
		while( deltatheta > PI ) deltatheta -= 2 * PI;
		while( deltatheta < -PI ) deltatheta += 2 * PI;

		if( fabs( deltatheta ) > 0.01f )
		{
			float maxTurningSpeed = ( m_speed / m_turningRadius ) * timestep;
			if( deltatheta > 0 )
			{
				m_orientation += MIN( deltatheta, maxTurningSpeed );
			}
			else
				m_orientation += MAX( deltatheta, -maxTurningSpeed );

			m_ahead.x = sin( m_orientation );
			m_ahead.y = cos( m_orientation );
		}
		else
		{
			m_ahead = delta / len;
			m_orientation = atan2( m_ahead.x, m_ahead.y );
		}
	}

	if( len < 0.1f )
	{
small_delta:
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
		{
			// Repath.
			CVector2D destination;
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
		else
			m_orderQueue.pop_front();
		return( false );
	}

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
				CVector2D avoidancePosition = collide->m_bounds->m_pos + right * ( collide->m_bounds->m_radius + m_bounds->m_radius * 2.5f );
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
			// Work out if our path goes to the left or to the right
			// of this obstacle. Go that way.
			// Weight a little to the right, too (helps unit-unit collisions)
			
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
