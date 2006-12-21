// Entity state-machine processing code.

#include "precompiled.h"

#include "Entity.h"
#include "EntityTemplate.h"
#include "EventHandlers.h"
#include "graphics/Model.h"
#include "graphics/ObjectEntry.h"
#include "graphics/SkeletonAnim.h"
#include "graphics/SkeletonAnimDef.h" // Animation duration
#include "graphics/Unit.h"
#include "ProductionQueue.h"
#include "maths/MathUtil.h"
#include "Collision.h"
#include "PathfindEngine.h"
#include "LOSManager.h"
#include "graphics/Terrain.h"
#include "Stance.h"

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
	WOULD_LEAVE_MAP,
	STANCE_DISALLOWS
};

bool CEntity::shouldRun(float distance)
{
	if (!entf_get(ENTF_SHOULD_RUN))
		return false;

	// tired
	if(m_staminaCurr <= 0)
		return false;

	if(distance >= m_runMaxRange)
		return false; 

	// don't start running if less than minimum
	if( distance <= m_runMinRange && !entf_get(ENTF_IS_RUNNING) )
		return false;
		
	return true;
}

float CEntity::chooseMovementSpeed( float distance )
{
	bool should_run = shouldRun(distance);
	
	float speed = should_run? m_runSpeed : m_speed;
	const char* anim_name = should_run? "run" : "walk";

	// Modify the speed based on the slope of the terrain in our direction (obtained from our x orientation)
	float angle = m_orientation_unclamped.x;
	int sector = rintf( angle / (PI/2) * m_base->m_pitchDivs );
	speed -= sector * m_base->m_pitchValue;

	// TODO: the animation code requires unicode for now. will be changed to
	// 8bit later (for consistency; note that filenames etc. need not be
	// unicode), so remove this then. 
	const CStrW u_anim_name(anim_name);

	if ( m_actor )
	{
		if ( !m_actor->IsPlayingAnimation( anim_name ) )
		{
			m_actor->SetEntitySelection( u_anim_name );
			m_actor->SetRandomAnimation( anim_name, false, speed );

			// Animation desync
			m_actor->GetModel()->Update( rand( 0, 1000 ) / 1000.0f );
			
			entf_set_to(ENTF_IS_RUNNING, should_run);
		}
	}
	
	return speed;
}

// Does all the shared processing for line-of-sight gotos

uint CEntity::processGotoHelper( CEntityOrder* current, size_t timestep_millis, HEntity& collide )
{
	float timestep=timestep_millis/1000.0f;

	CVector2D delta;
	delta.x = (float)current->m_target_location.x - m_position.X;
	delta.y = (float)current->m_target_location.y - m_position.Z;

	float len = delta.length();

	if( len < 0.01f )
		return( ALREADY_AT_DESTINATION );

	// Curve smoothing.
	// Here there be trig.

	float scale = chooseMovementSpeed( len ) * timestep;

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
	
	updateXZOrientation();

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

		// For now, ignore passThroughAllies for low-level movement (but leave it on for long-range
		// pathfinding); ideally we will enable pass-through-allies only for the long-range pathing
		// and when the unit is moving to assume its place in a formation, since it looks bad to have
		// units stand on each other otherwise.
		collide = getCollisionObject( this, false );
		
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

				//debug_assert( false && "Overlapping objects" );

				// Erm... do nothing?
				
				return( COLLISION_OVERLAPPING_OBJECTS );
			}

			// No. Is our destination within the obstacle?
			if( collide->m_bounds->contains( current->m_target_location ) )
				return( COLLISION_WITH_DESTINATION );

			// No. Are we nearing our destination, do we wish to stop there, and is it obstructed?

			if( ( m_orderQueue.size() == 1 ) && ( len <= 10.0f ) )
			{
				CBoundingCircle destinationObs( current->m_target_location.x, current->m_target_location.y, m_bounds->m_radius, 0.0f );
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

	// Does our stance not allow us to go there?
	if( current->m_source==CEntityOrder::SOURCE_UNIT_AI && !m_stance->checkMovement( m_position ) )
	{
		m_position.X -= delta.x;
		m_position.Z -= delta.y;
		if( m_bounds )
			m_bounds->setPosition( m_position.X, m_position.Z );

		return( STANCE_DISALLOWS );
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
			//entf_clear(ENTF_IS_RUNNING);
			//entf_clear(ENTF_SHOULD_RUN);
		}
		return( false );
	case COLLISION_OVERLAPPING_OBJECTS:
		return( false );
	case COLLISION_WITH_DESTINATION:
		// We're as close as we can get...
		m_orderQueue.pop_front();
		//entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		
		return( false );
	case COLLISION_NEAR_DESTINATION:
	{
		// Here's a weird idea: (I hope it works)
		// Spiral round the destination until a free point is found.
		CBoundingCircle destinationObs( current->m_target_location.x, current->m_target_location.y, m_bounds->m_radius, 0.0f );

		float interval = destinationObs.m_radius;
		float r = interval, theta = 0.0f, delta;
		float _x = current->m_target_location.x, _y = current->m_target_location.y;
		
		while( true )
		{
			delta = interval / r;
			theta += delta;
			r += ( interval * delta ) / ( 2 * PI );
			destinationObs.setPosition( _x + r * cosf( theta ), _y + r * sinf( theta ) );
			if( !getCollisionObject( &destinationObs ) ) break;
		}
		
		// Reset our destination
		current->m_target_location.x = _x + r * cosf( theta );
		current->m_target_location.y = _y + r * sinf( theta );

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

		avoidance.m_target_location = avoidancePosition;
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )

			m_orderQueue.pop_front();
		m_orderQueue.push_front( avoidance );
		return( false );
	}
	case WOULD_LEAVE_MAP:
		// Just stop here, repath if necessary.
		m_orderQueue.pop_front();
		//entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		return( false );
	case STANCE_DISALLOWS:
		return( false );		// The stance will have cleared our order queue already

	default:
		
		return( false );
	}
}

// Handles processing common to (at the moment) gather and melee attack actions
bool CEntity::processContactAction( CEntityOrder* current, size_t UNUSED(timestep_millis), CEntityOrder::EOrderType transition, SEntityAction* action )
{
	HEntity target = current->m_target_entity;

	if( !target || !target->m_extant )
	{
		popOrder();
		if( m_orderQueue.empty() && target.isValid() )
		{
			CEventTargetExhausted evt( target, action->m_Id );
			DispatchEvent( &evt );
		}
		return false;
	}

	if( g_Game->GetWorld()->GetLOSManager()->GetUnitStatus( target, m_player ) == UNIT_HIDDEN )
	{
		popOrder();
		return false;
	}

	current->m_target_location = target->m_position;
	float Distance = distance2D(current->m_target_location);

	if( Distance < action->m_MaxRange ) 
	{
		current->m_type = transition;
		entf_clear(ENTF_IS_RUNNING);
		return true;
	}
	else
	{
		if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->allowsMovement() )
		{
			popOrder();
			return false;		// We're not allowed to move at all by the current stance
		}

		chooseMovementSpeed( Distance );

		// The pathfinder will push its result back into this unit's queue and
		// add back the current order at the end with the transition type.
		current->m_type = transition;
		g_Pathfinder.requestContactPath( me, current, action->m_MaxRange );

		return true;
	}
}
bool CEntity::processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, const CStr& animation, CScriptEvent* contactEvent, SEntityAction* action )
{
	HEntity target = current->m_target_entity;

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
			entf_clear(ENTF_IS_RUNNING);
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
				entf_clear(ENTF_IS_RUNNING);
				entf_clear(ENTF_SHOULD_RUN);
				m_actor->SetEntitySelection( "idle" );
				m_actor->SetRandomAnimation( "idle" );
				popOrder();
				if( m_orderQueue.empty() && target.isValid() )
				{
					CEventTargetExhausted evt( target, action->m_Id );
					DispatchEvent( &evt );
				}
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
	if( !target || !target->m_extant
		|| g_Game->GetWorld()->GetLOSManager()->GetUnitStatus( target, m_player ) == UNIT_HIDDEN )
	{
		popOrder();
		if( m_orderQueue.empty() && target.isValid() )
		{
			CEventTargetExhausted evt( target, action->m_Id );
			DispatchEvent( &evt );
		}

		entf_clear(ENTF_IS_RUNNING);
		entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	CVector2D delta = CVector2D(target->m_position) - CVector2D(m_position);
	float deltaLength = delta.length();

	float adjRange = action->m_MaxRange + m_bounds->m_radius + target->m_bounds->m_radius;

	if( action->m_MinRange > 0.0f )
	{
		float adjMinRange = action->m_MinRange + m_bounds->m_radius + target->m_bounds->m_radius;
		if( delta.within( adjMinRange ) )
		{
			// Too close... avoid it if allowed by the current stance.
			if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->allowsMovement() )
			{
				popOrder();
				m_actor->SetRandomAnimation( "idle" );
				return false;		// We're not allowed to move at all by the current stance
			}

			entf_set(ENTF_SHOULD_RUN);
			chooseMovementSpeed( action->m_MinRange );
			
			// The pathfinder will push its result in front of the current order
			if( !g_Pathfinder.requestAvoidPath( me, current, action->m_MinRange + 2.0f ) )
			{
				m_actor->SetRandomAnimation( "idle" );	// Nothing we can do.. maybe we'll find a better target
				popOrder();
			}

			return false;
		}
	}

	if( !delta.within( adjRange ) )
	{
		// Too far away at the moment, chase after the target if allowed...
		if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->allowsMovement() )
		{
			popOrder();
			return false;
		}

		// We're aiming to end up at a location just inside our maximum range
		// (is this good enough?)
		delta = delta.normalize() * ( adjRange - m_bounds->m_radius );

		chooseMovementSpeed(deltaLength);
	
		current->m_target_location = (CVector2D)target->m_position - delta;

		HEntity collide;	
		switch( processGotoHelper( current, timestep_millis, collide ) )
		{
		case ALREADY_AT_DESTINATION:
		case REACHED_DESTINATION:
		case COLLISION_WITH_DESTINATION:
		case WOULD_LEAVE_MAP:
		case STANCE_DISALLOWS:
			// Not too far any more...
			break;
		case NORMAL:
			// May or may not be close enough, check...
			// (Assuming the delta above will never take us within minimum range)
			delta = target->m_position - m_position;
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

			avoidance.m_target_location = avoidancePosition;
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
		entf_clear(ENTF_IS_RUNNING);
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
		if( entf_get(ENTF_TRANSITION) )
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
	if( m_actions.find( current->m_action ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}
	SEntityAction& action_obj = m_actions[current->m_action];
	return( processContactAction( current, timestep_millis, CEntityOrder::ORDER_GENERIC_NOPATHING, &action_obj ) );
}

bool CEntity::processGenericNoPathing( CEntityOrder* current, size_t timestep_millis )
{
	if( m_actions.find( current->m_action ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}
	CEventGeneric evt( current->m_target_entity, current->m_action );
	if( !m_actor ) return( false );
	SEntityAction& action_obj = m_actions[current->m_action];
	return( processContactActionNoPathing( current, timestep_millis, action_obj.m_Animation, &evt, &action_obj ) );
}

bool CEntity::processGoto( CEntityOrder* current, size_t UNUSED(timestep_millis) )
{
	// float timestep=timestep_millis/1000.0f;
	// janwas: currently unused


	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_target_location;
	m_orderQueue.pop_front();
	float Distance = ( path_to - pos ).length();
	
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		//entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	chooseMovementSpeed( Distance );

	// The pathfinder will push its result back into this unit's queue.

	g_Pathfinder.requestPath( me, path_to, current->m_source );

	return( true );
}

bool CEntity::processGotoWaypoint( CEntityOrder* current, size_t UNUSED(timestep_milli), bool contact )
{
	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_target_location;
	m_orderQueue.pop_front();
	float Distance = ( path_to - pos ).length();
	
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	chooseMovementSpeed( Distance );

	g_Pathfinder.requestLowLevelPath( me, path_to, contact, current->m_pathfinder_radius, current->m_source );

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
	this_segment.m_pathfinder_radius = current->m_pathfinder_radius;
	repeat_patrol.m_type = CEntityOrder::ORDER_PATROL;
	repeat_patrol.m_pathfinder_radius = current->m_pathfinder_radius;
	m_orderQueue.pop_front();
	m_orderQueue.push_front( this_segment );
	m_orderQueue.push_back( repeat_patrol );
	return( true );
}

bool CEntity::processProduce( CEntityOrder* order )
{
	CEventStartProduction evt( order->m_produce_type, order->m_produce_name );
	if( DispatchEvent( &evt ) && evt.GetTime() >= 0 )
	{
		m_productionQueue->AddItem( order->m_produce_type, order->m_produce_name, evt.GetTime() );
	}
	return( false );
}
