// Entity state-machine processing code.

#include "precompiled.h"

#include "Entity.h"
#include "Model.h"
#include "ObjectEntry.h"
#include "Unit.h"

#include "Collision.h"
#include "PathfindEngine.h"
#include "Terrain.h"

#include "Game.h"

enum EGotoSituation
{
	NORMAL = 0,
	ALREADY_AT_DESTINATION,
	REACHED_DESTINATION,
	COLLISION_WITH_DESTINATION,
	COLLISION_NEAR_DESTINATION,
	COLLISION_OVERLAPPING_OBJECTS,
	COLLISION_OTHER,
	WOULD_LEAVE_MAP
};

// Does all the shared processing for line-of-sight gotos

uint CEntity::processGotoHelper( CEntityOrder* current, size_t timestep_millis, HEntity& collide )
{
	float timestep=timestep_millis/1000.0f;

	CVector2D delta;
	delta.x = (float)current->m_data[0].location.x - m_position.X;
	delta.y = (float)current->m_data[0].location.y - m_position.Z;

	float len = delta.length();

	if( len < 0.1f )
		return( ALREADY_AT_DESTINATION );

	// Curve smoothing.
	// Here there be trig.

	float scale = m_speed * timestep;

	// Note: Easy optimization: flag somewhere that this unit
	// is already pointing the right way, and don't do this
	// trig every time.

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

		// We can only really attempt to smooth paths the pathfinder
		// has flagged for us. If the turning-radius calculations are
		// applied to other types of waypoint, wierdness happens.
		// Things like an entity trying to walk to a point inside
		// his turning radius (which he can't do directly, so he'll
		// orbit the point indefinately), or just massive deviations
		// making the paths we calculate useless.
		// It's also painful trying to watch two entities resolve their
		// collision when they're both bound by turning constraints.
		
		// So, as a compromise for the look of the thing, we'll just turn in
		// place until we're looking the right way. At least, that's what
		// seems logical. But in most cases that looks worse. So actually,
		// let's not.
	
		if( current->m_type != CEntityOrder::ORDER_GOTO_SMOOTHED )
			m_orientation = m_targetorientation;
			
	}
	else
	{
		m_ahead = delta / len;
		m_orientation = m_targetorientation;
	}

	if( m_bounds && m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*)m_bounds)->setOrientation( m_ahead );

	EGotoSituation rc = NORMAL;

	if( scale > len )
	{
		scale = len;
		rc = REACHED_DESTINATION;
	}

	delta = m_ahead * scale;

	// What would happen if we moved forward a little?

	m_position.X += delta.x;
	m_position.Z += delta.y;

	if( m_bounds )
	{
		m_bounds->setPosition( m_position.X, m_position.Z );

		collide = getCollisionObject( this );
		
		if( collide )
		{	
			// We'd hit something. Let's not.
			m_position.X -= delta.x;
			m_position.Z -= delta.y;
			m_bounds->m_pos -= delta;

			// Is it too late to avoid the collision?

			if( collide->m_bounds->intersects( m_bounds ) )
			{
				// Yes. Oh dear. That can't be good.
				// This really shouldn't happen in the current build.

				assert( false && "Overlapping objects" );

				// Erm... do nothing?
				
				return( COLLISION_OVERLAPPING_OBJECTS );
			}

			// No. Is our destination within the obstacle?
			if( collide->m_bounds->contains( current->m_data[0].location ) )
				return( COLLISION_WITH_DESTINATION );

			// No. Are we nearing our destination, do we wish to stop there, and is it obstructed?

			if( ( m_orderQueue.size() == 1 ) && ( len <= 10.0f ) )
			{
				CBoundingCircle destinationObs( current->m_data[0].location.x, current->m_data[0].location.y, m_bounds->m_radius );
				if( getCollisionObject( &destinationObs ) )
				{
					// Yes. (Chances are a bunch of units were tasked to the same destination)
					return( COLLISION_NEAR_DESTINATION );
				}
			}

			// No?
			return( COLLISION_OTHER );

		}
	}

	// Will we step off the map?
	if( !g_Game->GetWorld()->GetTerrain()->isOnMap( m_position.X, m_position.Z ) )
	{
		// Yes. That's not a particularly good idea, either.

		m_position.X -= delta.x;
		m_position.Z -= delta.y;
		if( m_bounds )
			m_bounds->setPosition( m_position.X, m_position.Z );

		// All things being equal, we should only get here while on a collision path
		// (No destination should be off the map)
		
		return( WOULD_LEAVE_MAP );
	}

	// No. I suppose it's OK to go there, then. *disappointed*

	return( rc );
}


bool CEntity::processGotoNoPathing( CEntityOrder* current, size_t timestep_millis )
{
	HEntity collide;
	switch( processGotoHelper( current, timestep_millis, collide ) )
	{
	case ALREADY_AT_DESTINATION:
		// If on a collision path; decide where to go next. Otherwise, proceed to the next waypoint.
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
		{
			repath();
		}
		else
			m_orderQueue.pop_front();
		return( false );
	case COLLISION_OVERLAPPING_OBJECTS:
		return( false );
	case COLLISION_WITH_DESTINATION:
		// We're here...
		m_orderQueue.pop_front();
		return( false );
	case COLLISION_NEAR_DESTINATION:
	{
		// Here's a wierd idea: (I hope it works)
		// Spiral round the destination until a free point is found.
		CBoundingCircle destinationObs( current->m_data[0].location.x, current->m_data[0].location.y, m_bounds->m_radius );

		float interval = destinationObs.m_radius;
		float r = interval, theta = 0.0f, delta;
		float _x = current->m_data[0].location.x, _y = current->m_data[0].location.y;
		
		while( true )
		{
			delta = interval / r;
			theta += delta;
			r += ( interval * delta ) / ( 2 * PI );
			destinationObs.setPosition( _x + r * cosf( theta ), _y + r * sinf( theta ) );
			if( !getCollisionObject( &destinationObs ) ) break;
		}
		
		// Reset our destination
		current->m_data[0].location.x = _x + r * cosf( theta );
		current->m_data[0].location.y = _y + r * sinf( theta );

		return( false );
	}
	case COLLISION_OTHER:
	{
		// Path around it.
			
		CEntityOrder avoidance;
		avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
		CVector2D right;
		right.x = m_ahead.y; right.y = -m_ahead.x;
		CVector2D avoidancePosition;

		// Which is the shortest diversion, going left or right?
		// (Weight a little towards the right, to stop both units dodging the same way)

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

		// Create a short path representing this detour

		avoidance.m_data[0].location = avoidancePosition;
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
			m_orderQueue.pop_front();
		m_orderQueue.push_front( avoidance );
		return( false );
	}
	case WOULD_LEAVE_MAP:
		// Just stop here, repath if necessary.
		m_orderQueue.pop_front();
		return( false );
	default:
		return( false );
	}
}

// Handles processing common to (at the moment) gather and melee attack actions
bool CEntity::processContactAction( CEntityOrder* current, size_t timestep_millis, int transition, float range )
{
	m_orderQueue.pop_front();

	if( !current->m_data[0].entity || !current->m_data[0].entity->m_extant )
		return( false );
	
	current->m_data[0].location = current->m_data[0].entity->m_position;

	if( ( current->m_data[0].location - m_position ).length() < m_meleeRange ) 
	{
		(int&)current->m_type = transition;
		return( true );
	}

	if( m_transition && m_actor )
	{
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_WalkAnim );
		// Animation desync
		m_actor->GetModel()->Update( ( rand() * 1000.0f ) / 1000.0f );
	}

	// The pathfinder will push its result back into this unit's queue.

	g_Pathfinder.requestContactPath( me, current->m_data[0].entity, transition );

	return( true );
}
bool CEntity::processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, CSkeletonAnim* animation, CScriptEvent* contactEvent, float range, float minRange = 0.0f )
{
	// Target's dead (or exhausted)? Then our work here is done.
	if( !current->m_data[0].entity || !current->m_data[0].entity->m_extant )
	{
		m_orderQueue.pop_front();
		return( false );
	}

	if( m_actor )
	{
		// Still playing attack animation? Suspend processing.
		if( m_actor->GetModel()->GetAnimation() == animation )
			return( false );

		// Just transitioned? No animation? (=> melee just finished) Play walk.
		if( m_transition || !m_actor->GetModel()->GetAnimation() )
			m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_WalkAnim );
	}

	CVector2D delta = current->m_data[0].entity->m_position - m_position;

	float adjRange = range + m_bounds->m_radius + current->m_data[0].entity->m_bounds->m_radius;

	if( minRange > 0.0f )
	{
		float adjMinRange = m_meleeRangeMin + m_bounds->m_radius + current->m_data[0].entity->m_bounds->m_radius;
		if( delta.within( adjMinRange ) )
		{
			// Too close... do nothing.
			return( false );
		}
	}

	if( !delta.within( adjRange ) )
	{
		// Too far away at the moment, chase after the target...
		// We're aiming to end up at a location just inside our maximum range
		// (is this good enough?)

		delta = delta.normalize() * ( adjRange - m_bounds->m_radius );

		current->m_data[0].location = (CVector2D)current->m_data[0].entity->m_position - delta;

		HEntity collide;	
		switch( processGotoHelper( current, timestep_millis, collide ) )
		{
		case ALREADY_AT_DESTINATION:
		case REACHED_DESTINATION:
		case COLLISION_WITH_DESTINATION:
			// Not too far any more...
			break;
		case NORMAL:
			// May or may not be close enough, check...
			// (Assuming the delta above will never take us within minimum range)
			delta = current->m_data[0].entity->m_position - m_position;
			if( delta.within( adjRange ) )
				break;
			// Otherwise, continue chasing
			return( false );
		default:
			// Path around it.
			
			CEntityOrder avoidance;
			avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
			CVector2D right;
			right.x = m_ahead.y; right.y = -m_ahead.x;
			CVector2D avoidancePosition;

			// Which is the shortest diversion, going left or right?
			// (Weight a little towards the right, to stop both units dodging the same way)

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

			// Create a short path representing this detour

			avoidance.m_data[0].location = avoidancePosition;
			if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
				m_orderQueue.pop_front();
			m_orderQueue.push_front( avoidance );
			return( false );
		}
	}
	else
	{
		// Close enough, but turn to face them.
		m_orientation = atan2( delta.x, delta.y );
		m_ahead = delta.normalize();
	}

	if( DispatchEvent( contactEvent ) && m_actor )
		m_actor->GetModel()->SetAnimation( animation, true );

	return( false );
}

bool CEntity::processAttackMelee( CEntityOrder* current, size_t timestep_millis )
{
	return( processContactAction( current, timestep_millis, CEntityOrder::ORDER_ATTACK_MELEE_NOPATHING, 0.5 ) );
}
bool CEntity::processAttackMeleeNoPathing( CEntityOrder* current, size_t timestep_milli )
{
	CEventAttack evt( current->m_data[0].entity );
	return( processContactActionNoPathing( current, timestep_milli, m_actor ? m_actor->GetObject()->m_MeleeAnim : NULL, &evt, m_meleeRange, m_meleeRangeMin ) );
}
bool CEntity::processGather( CEntityOrder* current, size_t timestep_millis )
{
	return( processContactAction( current, timestep_millis, CEntityOrder::ORDER_GATHER_NOPATHING, 0.5 ) );
}

bool CEntity::processGatherNoPathing( CEntityOrder* current, size_t timestep_millis )
{
	CEventGather evt( current->m_data[0].entity );
	return( processContactActionNoPathing( current, timestep_millis, m_actor ? m_actor->GetObject()->m_GatherAnim : NULL, &evt, 5.0, 0.0 ) );
}

bool CEntity::processGoto( CEntityOrder* current, size_t timestep_millis )
{
	float timestep=timestep_millis/1000.0f;

	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_data[0].location;
	m_orderQueue.pop_front();

	// Let's just check we're going somewhere...
	if( ( path_to - pos ).length() < 0.1f ) 
		return( false );

	if( m_transition && m_actor )
	{
		m_actor->GetModel()->SetAnimation( m_actor->GetObject()->m_WalkAnim );
		// Animation desync
		m_actor->GetModel()->Update( ( rand() * 1000.0f ) / 1000.0f );
	}

	// The pathfinder will push its result back into this unit's queue.

	g_Pathfinder.requestPath( me, path_to );

	return( true );
}

bool CEntity::processPatrol( CEntityOrder* current, size_t timestep_millis )
{
	float timestep=timestep_millis/1000.0f;

	CEntityOrder this_segment;
	CEntityOrder repeat_patrol;

	// Duplicate the patrol order, push one copy onto the start of our order queue
	// (that's the path we'll be taking next) and one copy onto the end of the
	// queue (to keep us patrolling)

	this_segment.m_type = CEntityOrder::ORDER_GOTO;
	this_segment.m_data[0] = current->m_data[0];
	repeat_patrol.m_type = CEntityOrder::ORDER_PATROL;
	repeat_patrol.m_data[0] = current->m_data[0];
	m_orderQueue.pop_front();
	m_orderQueue.push_front( this_segment );
	m_orderQueue.push_back( repeat_patrol );
	return( true );
}
