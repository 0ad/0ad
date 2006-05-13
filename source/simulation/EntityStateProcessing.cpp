// Entity state-machine processing code.

#include "precompiled.h"

#include "Entity.h"
#include "BaseEntity.h"
#include "Model.h"
#include "ObjectEntry.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h" // Animation duration
#include "Unit.h"
#include "ProductionQueue.h"
#include "MathUtil.h"

#include "Collision.h"
#include "PathfindEngine.h"
#include "Terrain.h"

#include "ps/Game.h"
#include "ps/World.h"

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

float CEntity::processChooseMovement( float distance )
{
	// Should we run or walk
	if (m_shouldRun && m_staminaCurr > 0 && distance < m_run.m_MaxRange && 
		( distance > m_run.m_MinRange || m_isRunning ) )
	{	
		if ( m_actor )
		{
			if ( !m_actor->IsPlayingAnimation( "run" ) )
			{
				m_actor->SetEntitySelection( L"run" );
				m_actor->SetRandomAnimation( "run", false, m_runSpeed );

				// Animation desync
				m_actor->GetModel()->Update( rand( 0, 1000 ) / 1000.0f );
				m_isRunning = true;
			}
		}
		return m_runSpeed;
	}
	else
	{
		if ( m_actor )
		{
			// Should we update animation?
			if ( !m_actor->IsPlayingAnimation( "walk" ) )
			{
				m_actor->SetEntitySelection( L"walk" );
				m_actor->SetRandomAnimation( "walk", false, m_speed );

				// Animation desync
				m_actor->GetModel()->Update( rand( 0, 1000 ) / 1000.0f );
				m_isRunning = false;
			}
		}
		return m_speed;
	}
}

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

	float scale = processChooseMovement( len ) * timestep;

	// Note: Easy optimization: flag somewhere that this unit
	// is already pointing the  way, and don't do this
	// trig every time.right

	m_targetorientation = atan2( delta.x, delta.y );
	float deltatheta = m_targetorientation - (float)m_orientation.Y;
	while( deltatheta > PI ) deltatheta -= 2 * PI;
	while( deltatheta < -PI ) deltatheta += 2 * PI;

	if( fabs( deltatheta ) > 0.01f )
	{
		if ( m_turningRadius != 0 )
		{
			float maxTurningSpeed = ( m_speed / m_turningRadius ) * timestep;
			deltatheta = clamp( deltatheta, -maxTurningSpeed, maxTurningSpeed );
		}
		m_orientation.Y = m_orientation.Y + deltatheta;

		m_ahead.x = sin( m_orientation.Y );
		m_ahead.y = cos( m_orientation.Y );

		// We can only really attempt to smooth paths the pathfinder
		// has flagged for us. If the turning-radius calculations are
		// applied to other types of waypoint, weirdness happens.
		// Things like an entity trying to walk to a point inside
		// his turning radius (which he can't do directly, so he'll
		// orbit the point indefinitely), or just massive deviations
		// making the paths we calculate useless.
		// It's also painful trying to watch two entities resolve their
		// collision when they're both bound by turning constraints.
		
		// So, as a compromise for the look of the thing, we'll just turn in
		// place until we're looking the right way. At least, that's what
		// seems logical. But in most cases that looks worse. So actually,
		// let's not.
	
		if( current->m_type != CEntityOrder::ORDER_GOTO_SMOOTHED )
			m_orientation.Y = m_targetorientation;
			
	}
	else
	{
		m_ahead = delta / len;
		m_orientation.Y = m_targetorientation;
	}
	CEntity* _this = this;
	CVector2D targetXZ = g_Game->GetWorld()->GetTerrain()->getSlopeAngleFace( m_position.X, m_position.Z, _this );
	
	while( targetXZ.x > PI ) targetXZ.x -= 2 * PI;
	while( targetXZ.x < -PI ) targetXZ.x += 2 * PI;
	while( targetXZ.y > PI ) targetXZ.y -= 2 * PI;
	while( targetXZ.y < -PI ) targetXZ.y += 2 * PI;
	
	m_orientation.X = clamp( targetXZ.x, -m_anchorConformX, m_anchorConformX );
	m_orientation.Z = clamp( targetXZ.y, -m_anchorConformZ, m_anchorConformZ );
	m_orientation_unclamped.x = targetXZ.x;
	m_orientation_unclamped.y = targetXZ.y;

	CMovementEvent evt( m_orientation_unclamped.x );
	DispatchEvent(&evt);

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

				debug_assert( false && "Overlapping objects" );

				// Erm... do nothing?
				
				return( COLLISION_OVERLAPPING_OBJECTS );
			}

			// No. Is our destination within the obstacle?
			if( collide->m_bounds->contains( current->m_data[0].location ) )
				return( COLLISION_WITH_DESTINATION );

			// No. Are we nearing our destination, do we wish to stop there, and is it obstructed?

			if( ( m_orderQueue.size() == 1 ) && ( len <= 10.0f ) )
			{
				CBoundingCircle destinationObs( current->m_data[0].location.x, current->m_data[0].location.y, m_bounds->m_radius, 0.0f );
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
		{
			m_orderQueue.pop_front();
			//m_isRunning = false;
			//m_shouldRun = false;
		}
		return( false );
	case COLLISION_OVERLAPPING_OBJECTS:
		return( false );
	case COLLISION_WITH_DESTINATION:
		// We're here...
		m_orderQueue.pop_front();
		//m_isRunning = false;
		//m_shouldRun = false;
		
		return( false );
	case COLLISION_NEAR_DESTINATION:
	{
		// Here's a weird idea: (I hope it works)
		// Spiral round the destination until a free point is found.
		CBoundingCircle destinationObs( current->m_data[0].location.x, current->m_data[0].location.y, m_bounds->m_radius, 0.0f );

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
		//m_isRunning = false;
		//m_shouldRun = false;
		return( false );
	default:
		
		return( false );
	}
}

// Handles processing common to (at the moment) gather and melee attack actions
bool CEntity::processContactAction( CEntityOrder* current, size_t UNUSED(timestep_millis), int transition, SEntityAction* action )
{
	if( !current->m_data[0].entity || !current->m_data[0].entity->m_extant )
		return( false );
	current->m_data[0].location = current->m_data[0].entity->m_position;
	float Distance = (current->m_data[0].location - m_position).length();

	if( Distance < action->m_MaxRange ) 
	{
		(int&)current->m_type = transition;
		m_isRunning = false;
		return( true );
	}
	
	processChooseMovement( Distance );

	// The pathfinder will push its result back into this unit's queue and
	// add back the current order at the end with the transition type.
	(int&)current->m_type = transition;
	g_Pathfinder.requestContactPath( me, current );

	return( true );
}
bool CEntity::processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, const CStr& animation, CScriptEvent* contactEvent, SEntityAction* action )
{
	if( m_fsm_cyclepos != NOT_IN_CYCLE )
	{
		size_t nextpos = m_fsm_cyclepos + timestep_millis * 2;
		if( ( m_fsm_cyclepos <= m_fsm_anipos ) &&
			( nextpos > m_fsm_anipos ) )
		{
			// Start playing.
			// Start the animation. Actual damage/gather will be done in a 
			// few hundred ms, at the 'action point' of the animation we're
			// now setting.
			m_isRunning = false;
			// TODO: this is set to be looping, because apparently it otherwise
			// plays one frame of 'idle' after e.g. attacks. But this way means
			// animations sometimes play ~1.5 times then repeat, which looks
			// broken too.
			//m_actor->GetModel()->SetAnimation( m_fsm_animation, true, 1000.0f * m_fsm_animation->m_AnimDef->GetDuration() / (float)action->m_Speed, m_actor->GetRandomAnimation( "idle" ) );
			m_actor->GetModel()->SetAnimation( m_fsm_animation, false, 1000.0f * m_fsm_animation->m_AnimDef->GetDuration() / (float)action->m_Speed );
		}
		if( ( m_fsm_cyclepos <= m_fsm_anipos2 ) &&
			( nextpos > m_fsm_anipos2 ) )
		{
			// Load the ammunition.
			m_actor->ShowAmmunition();
		}
		if( ( m_fsm_cyclepos <= action->m_Speed ) && ( nextpos > action->m_Speed ) )
		{
			m_actor->HideAmmunition();
			if(!DispatchEvent( contactEvent ))
			{
				// Cancel current order
				popOrder();
				m_isRunning = false;
				m_shouldRun = false;
				m_actor->SetEntitySelection( L"idle" );
				m_actor->SetRandomAnimation( "idle" );
				return( false );
			}
		}
		
		if( nextpos >= ( action->m_Speed * 2 ) )
		{
			// End of cycle.
			m_fsm_cyclepos = NOT_IN_CYCLE;
			return( false );
		}

		// Otherwise, increment position.
		m_fsm_cyclepos = nextpos;
		return( false );
	}

	// Target's dead (or exhausted), or we cancelled? Then our work here is done.
	if( !current->m_data[0].entity || !current->m_data[0].entity->m_extant )
	{
		//TODO:  eventually when stances/formations are implemented, if applicable (e.g. not 
		//heal or if defensive stance), the unit should expand and continue the order.
		
		popOrder();
		m_isRunning = false;
		m_shouldRun = false;
		return( false );
	}

	CVector2D delta = current->m_data[0].entity->m_position - m_position;
	float deltaLength = delta.length();

	float adjRange = action->m_MaxRange + m_bounds->m_radius + current->m_data[0].entity->m_bounds->m_radius;

	if( action->m_MinRange > 0.0f )
	{
		float adjMinRange = action->m_MinRange + m_bounds->m_radius + current->m_data[0].entity->m_bounds->m_radius;
		if( delta.within( adjMinRange ) )
		{
			// Too close... do nothing.
			m_isRunning = false;
			m_shouldRun = false;
			return( false );
		}
	}

	if( !delta.within( adjRange ) )
	{
		// Too far away at the moment, chase after the target...
		// We're aiming to end up at a location just inside our maximum range
		// (is this good enough?)
		delta = delta.normalize() * ( adjRange - m_bounds->m_radius );

		processChooseMovement(deltaLength);
	
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
		m_orientation.Y = atan2( delta.x, delta.y );
		m_ahead = delta.normalize();
		m_isRunning = false;
	}

	// Pick our animation, calculate the time to play it, and start the timer.
	m_actor->SetEntitySelection( animation );
	m_fsm_animation = m_actor->GetRandomAnimation( animation );

	// Here's the idea - we want to be at that animation's event point
	// when the timer reaches action->m_Speed. The timer increments by 2 every millisecond.
	// animation->m_actionpos is the time offset into that animation that event
	// should happen. So...
	m_fsm_anipos = (size_t)( action->m_Speed * ( 1.0f - 2 * m_fsm_animation->m_ActionPos ) );
	// But...
	if( m_fsm_anipos < 0 ) // (FIXME: m_fsm_anipos is unsigned, so this will never be true...)
	{
		// We ought to have started it in the past. Oh well.
		// Here's what we'll do: play it now, and advance it to
		// the point it should be by now.
		
		m_actor->GetModel()->SetAnimation( m_fsm_animation, true, 1000.0f * m_fsm_animation->m_AnimDef->GetDuration() / (float)action->m_Speed, m_actor->GetRandomAnimation( "idle" ) );
		m_actor->GetModel()->Update( action->m_Speed * ( m_fsm_animation->m_ActionPos / 1000.0f - 0.0005f ) );
	}
	else
	{
		// If we've just transitioned, play idle. Otherwise, let the previous animation complete, if it
		// hasn't already.
		if( m_transition )
		{
			// (don't change actor's entity-selection)
			m_actor->SetRandomAnimation( "idle" );
		}
	}

	// Load time needs to be animation->m_ActionPos2 ms after the start of the animation.

	m_fsm_anipos2 = m_fsm_anipos + (size_t)( action->m_Speed * m_fsm_animation->m_ActionPos2 * 2 );
	if( m_fsm_anipos2 < 0 ) // (FIXME: m_fsm_anipos2 is unsigned, so this will never be true...)
	{
		// Load now.
		m_actor->ShowAmmunition();
	}

	m_fsm_cyclepos = 0;

	return( false );
}

bool CEntity::processGeneric( CEntityOrder* current, size_t timestep_millis )
{
	int id = current->m_data[1].data;
	if( m_actions.find( id ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}
	SEntityAction& action = m_actions[id];
	return( processContactAction( current, timestep_millis, CEntityOrder::ORDER_GENERIC_NOPATHING, &action ) );
}

bool CEntity::processGenericNoPathing( CEntityOrder* current, size_t timestep_millis )
{
	int id = current->m_data[1].data;
	if( m_actions.find( id ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}
	SEntityAction& action = m_actions[id];
	CEventGeneric evt( current->m_data[0].entity, id );
	if( !m_actor ) return( false );
	return( processContactActionNoPathing( current, timestep_millis, action.m_Animation, &evt, &action ) );
}

bool CEntity::processGoto( CEntityOrder* current, size_t UNUSED(timestep_millis) )
{
	// float timestep=timestep_millis/1000.0f;
	// janwas: currently unused


	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_data[0].location;
	m_orderQueue.pop_front();
	float Distance = ( path_to - pos ).length();
	
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		//m_isRunning = false;
		//m_shouldRun = false;
		return( false );
	}

	processChooseMovement( Distance );

	// The pathfinder will push its result back into this unit's queue.

	g_Pathfinder.requestPath( me, path_to );

	return( true );
}

bool CEntity::processGotoWaypoint( CEntityOrder* current, size_t UNUSED(timestep_milli), bool contact )
{
	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_data[0].location;
	m_orderQueue.pop_front();
	float Distance = ( path_to - pos ).length();
	
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		m_isRunning = false;
		//m_shouldRun = false;
		return( false );
	}

	processChooseMovement( Distance );

	g_Pathfinder.requestLowLevelPath( me, path_to, contact );

	return( true );
}

bool CEntity::processPatrol( CEntityOrder* current, size_t UNUSED(timestep_millis) )
{
	// float timestep=timestep_millis/1000.0f;
	// janwas: currently unused

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

bool CEntity::processProduce( CEntityOrder* order )
{
	int type = order->m_data[1].data;
	CStrW name = order->m_data[0].string;
	debug_printf("Trying to produce %d %ls\n", type, name.c_str() );
	CEventStartProduction evt( type, name );
	if( DispatchEvent( &evt ) && evt.GetTime() >= 0 )
	{
		debug_printf("Production started, time is %f\n", evt.GetTime());
		m_productionQueue->AddItem( type, name, evt.GetTime() );
	}
	return( false );
}
