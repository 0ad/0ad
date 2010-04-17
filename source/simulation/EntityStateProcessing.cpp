/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
//#include "sound/SoundGroupMgr.h"

#include "ps/Game.h"
#include "ps/World.h"

#include "lib/rand.h"

#include "ps/GameSetup/Config.h"

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

bool CEntity::ShouldRun(float distance)
{
	if( !entf_get(ENTF_SHOULD_RUN) )
		return false;

	// tired
	if( m_staminaCurr <= 0 )
		return false;

	if( distance >= m_runMaxRange )
		return false; 

	// don't start running if less than minimum
	if( distance <= m_runMinRange && !entf_get(ENTF_IS_RUNNING) )
		return false;
		
	return true;
}

float CEntity::ChooseMovementSpeed( float distance )
{
	bool should_run = ShouldRun(distance);
	
	float speed = should_run? m_runSpeed : m_speed;
	const char* anim_name = should_run? "run" : "walk";

	// Modify the speed based on the slope of the terrain in our direction (obtained from our x orientation)
	float angle = m_orientation_unclamped.x;
	int sector = rintf( angle / ((float)M_PI/2) * m_base->m_pitchDivs );
	speed -= sector * m_base->m_pitchValue;

	entf_set_to(ENTF_IS_RUNNING, should_run);

	if ( m_actor )
	{
		if ( !m_actor->IsPlayingAnimation( anim_name ) )
		{
			m_actor->SetAnimationState( anim_name, false, speed, 0.f, false, L"" );

			// Animation desync
			m_actor->GetModel().Update( rand( 0, 1000 ) / 1000.0f );
		}
	}
	
	return speed;
}

// Does all the shared processing for line-of-sight gotos

int CEntity::ProcessGotoHelper( CEntityOrder* current, int timestep_millis, HEntity& collide, float& timeLeft )
{
	float timestep=timestep_millis/1000.0f;

	CVector2D delta;
	delta.x = (float)current->m_target_location.x - m_position.X;
	delta.y = (float)current->m_target_location.y - m_position.Z;

	float len = delta.Length();

	if( len < 0.01f )
		return( ALREADY_AT_DESTINATION );

	// Curve smoothing.
	// Here there be trig.

	float speed = ChooseMovementSpeed( len );
	float scale = speed * timestep;

	// Note: Easy optimization: flag somewhere that this unit
	// is already pointing the  way, and don't do this
	// trig every time.right

	m_targetorientation = atan2( delta.x, delta.y );
	float deltatheta = m_targetorientation - (float)m_orientation.Y;
	while( deltatheta > (float)M_PI ) deltatheta -= 2 * (float)M_PI;
	while( deltatheta < -(float)M_PI ) deltatheta += 2 * (float)M_PI;

	if( fabs( deltatheta ) > 0.01f )
	{
		if ( m_turningRadius != 0 )
		{
			float maxTurningSpeed = ( speed / m_turningRadius ) * timestep;
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
	
	UpdateXZOrientation();

	if( m_bounds && m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*) m_bounds)->SetOrientation( m_ahead );

	EGotoSituation rc = NORMAL;

	if( scale > len )
	{
		// Reached destination. Calculate how much time we have left for the next order.
		scale = len;
		timeLeft = timestep - (len / speed);
		rc = REACHED_DESTINATION;
	}

	delta = m_ahead * scale;

	// What would happen if we moved forward a little?

	m_position.X += delta.x;
	m_position.Z += delta.y;

	if( m_bounds )
	{
		m_bounds->SetPosition( m_position.X, m_position.Z );

		// For now, ignore passThroughAllies for low-level movement (but leave it on for long-range
		// pathfinding); ideally we will enable pass-through-allies only for the long-range pathing
		// and when the unit is moving to assume its place in a formation, since it looks bad to have
		// units stand on each other otherwise.
		collide = GetCollisionObject( this, false );
		
		if( collide )
		{	
			// We'd hit something. Let's not.
			m_position.X -= delta.x;
			m_position.Z -= delta.y;
			m_bounds->m_pos -= delta;

			// Is it too late to avoid the collision?

			if( collide->m_bounds->Intersects( m_bounds ) )
			{
				// Yes. Oh dear. That can't be good.
				// This really shouldn't happen in the current build.

				//debug_assert( false && "Overlapping objects" );

				// Erm... do nothing?
				
				return( COLLISION_OVERLAPPING_OBJECTS );
			}

			// No. Is our destination within the obstacle?
			if( collide->m_bounds->Contains( current->m_target_location ) )
				return( COLLISION_WITH_DESTINATION );

			// No. Are we nearing our destination, do we wish to stop there, and is it obstructed?

			if( ( m_orderQueue.size() == 1 ) && ( len <= 10.0f ) )
			{
				CBoundingCircle destinationObs( current->m_target_location.x, current->m_target_location.y, m_bounds->m_radius, 0.0f );
				if( GetCollisionObject( &destinationObs ) )
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
	if( !g_Game->GetWorld()->GetTerrain()->IsOnMap( m_position.X, m_position.Z ) )
	{
		// Yes. That's not a particularly good idea, either.

		m_position.X -= delta.x;
		m_position.Z -= delta.y;
		if( m_bounds )
			m_bounds->SetPosition( m_position.X, m_position.Z );

		// All things being equal, we should only get here while on a collision path
		// (No destination should be off the map)
		
		return( WOULD_LEAVE_MAP );
	}

	// Does our stance not allow us to go there?
	if( current->m_source==CEntityOrder::SOURCE_UNIT_AI && !m_stance->CheckMovement( m_position ) )
	{
		m_position.X -= delta.x;
		m_position.Z -= delta.y;
		if( m_bounds )
			m_bounds->SetPosition( m_position.X, m_position.Z );

		return( STANCE_DISALLOWS );
	}

	// No. I suppose it's OK to go there, then. *disappointed*

	return( rc );
}

// Go towards a waypoint, or repath if there is an obstacle in the way
bool CEntity::ProcessGotoNoPathing( CEntityOrder* current, int timestep_millis )
{
	HEntity collide;
	float timeLeft;
	switch( ProcessGotoHelper( current, timestep_millis, collide, timeLeft ) )
	{
	case ALREADY_AT_DESTINATION:	
	{
		// If on a collision path; decide where to go next. Otherwise, proceed to the next waypoint.
		if( current->m_type == CEntityOrder::ORDER_GOTO_COLLISION )
			Repath();
		else
			m_orderQueue.pop_front();
		return( false );
	}
	case REACHED_DESTINATION:
	{
		// Start along the next segment of the path, if one exists
		m_orderQueue.pop_front();
		if( !m_orderQueue.empty() )
		{
			CEntityOrder* newOrder = &m_orderQueue.front();
			switch( newOrder->m_type )
			{
				case CEntityOrder::ORDER_GOTO_NOPATHING:
				case CEntityOrder::ORDER_GOTO_COLLISION:
				case CEntityOrder::ORDER_GOTO_SMOOTHED:
				{
					int newTimestep = int(timeLeft * 1000.0f);
					return( ProcessGotoNoPathing( newOrder, newTimestep ) );
				}
			}
		}
		return( false );
	}
	case COLLISION_OVERLAPPING_OBJECTS:
	{
		return( false );
	}
	case COLLISION_WITH_DESTINATION:
	{
		// We're as close as we can get...
		m_orderQueue.pop_front();
		return( false );
	}
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
			r += ( interval * delta ) / ( 2 * (float)M_PI );
			destinationObs.SetPosition( _x + r * cosf( theta ), _y + r * sinf( theta ) );
			if( !GetCollisionObject( &destinationObs ) ) break;
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

		if( ( collide->m_bounds->m_pos - m_bounds->m_pos ).Dot( right ) < 1 )
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
	{
		// Just stop here, Repath if necessary.
		m_orderQueue.pop_front();
		return( false );
	}
	case STANCE_DISALLOWS:
	{
		return( false );		// The stance will have cleared our order queue already
	}
	default:	
		return( false );
	}
}

// Go towards a waypoint, unless you can perform a contact action
bool CEntity::ProcessGotoNoPathingContact( CEntityOrder* current, int timestep_millis )
{
	CEntityOrder* contact_order = 0;
	CEntityOrders::iterator it = m_orderQueue.begin();
	for(; it != m_orderQueue.end(); ++it)
	{
		if( it->m_type == CEntityOrder::ORDER_CONTACT_ACTION
			|| it->m_type == CEntityOrder::ORDER_CONTACT_ACTION_NOPATHING )
		{
			contact_order = &(*it);
			break;
		}
	}
	if( ProcessContactAction(contact_order, timestep_millis, false ) )
	{
		// We are in range to do our action; pop off all the pathing orders
		while( m_orderQueue.front().m_type != CEntityOrder::ORDER_CONTACT_ACTION
				&& m_orderQueue.front().m_type != CEntityOrder::ORDER_CONTACT_ACTION_NOPATHING )
		{
			m_orderQueue.pop_front();
		}
		return true;
	}
	else
	{
		return ProcessGotoNoPathing( current, timestep_millis );
	}
}

// Handles processing common to (at the moment) gather and melee attack actions
bool CEntity::ProcessContactAction( CEntityOrder* current, 
								   int UNUSED(timestep_millis),
								   bool repath_if_needed )
{
	if( m_actions.find( current->m_action ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}

	SEntityAction* action = &m_actions[current->m_action];
	CEntityOrder::EOrderType transition = CEntityOrder::ORDER_CONTACT_ACTION_NOPATHING;
	HEntity target = current->m_target_entity;

	if( !target || !target->m_extant )
	{
		PopOrder();
		if( m_orderQueue.empty() && target.IsValid() )
		{
			CEventTargetExhausted evt( target, action->m_Id );
			DispatchEvent( &evt );
		}
		return false;
	}

	if( current->m_source != CEntityOrder::SOURCE_TRIGGERS &&
		g_Game->GetWorld()->GetLOSManager()->GetUnitStatus( target, m_player ) == UNIT_HIDDEN )
	{
		PopOrder();
		return false;
	}

	current->m_target_location = target->m_position;
	float Distance = Distance2D(current->m_target_location);

	if( Distance < action->m_MaxRange ) 
	{
		current->m_type = transition;
		entf_clear(ENTF_IS_RUNNING);
		return true;
	}
	else if (repath_if_needed)
	{
		if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->AllowsMovement() )
		{
			PopOrder();
			return false;		// We're not allowed to move at all by the current stance
		}

		ChooseMovementSpeed( Distance );

		// The pathfinder will push its result back into this unit's queue and
		// add back the current order at the end with the transition type.
		current->m_type = transition;
		g_Pathfinder.RequestContactPath( me, current, action->m_MaxRange );

		return true;
	}
	else
	{
		return false;
	}
}

bool CEntity::ProcessContactActionNoPathing( CEntityOrder* current, int timestep_millis )
{
	if( m_actions.find( current->m_action ) == m_actions.end() )
	{	
		return false;	// we've been tasked as part of a group but we can't do this action
	}
	if( !m_actor )
	{
		return( false );	// shouldn't happen, but having no actor would mean we have no animation
	}

	CEventContactAction contactEvent( current->m_target_entity, current->m_action );
	SEntityAction* action = &m_actions[current->m_action];
	CStr& animation = action->m_Animation;
	HEntity target = current->m_target_entity;

	if( m_fsm_cyclepos != NOT_IN_CYCLE )
	{
		int nextpos = m_fsm_cyclepos + timestep_millis * 2;

		if( ( m_fsm_cyclepos <= action->m_Speed ) && ( nextpos > action->m_Speed ) )
		{
			const size_t soundGroupIndex = m_base->m_SoundGroupTable[animation];
//			g_soundGroupMgr->PlayNext(soundGroupIndex, m_position);

			if(!DispatchEvent( &contactEvent ))
			{
				// Cancel current order
				entf_clear(ENTF_IS_RUNNING);
				entf_clear(ENTF_SHOULD_RUN);
				m_actor->SetAnimationState( "idle", false, 1.f, 0.f, false, L"" );
				PopOrder();
				if( m_orderQueue.empty() && target.IsValid() )
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
		PopOrder();
		if( m_orderQueue.empty() && target.IsValid() )
		{
			CEventTargetExhausted evt( target, action->m_Id );
			DispatchEvent( &evt );
		}

		entf_clear(ENTF_IS_RUNNING);
		entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	CVector2D delta = CVector2D(target->m_position) - CVector2D(m_position);
	float deltaLength = delta.Length();

	float adjRange = action->m_MaxRange + m_bounds->m_radius + target->m_bounds->m_radius;

	if( action->m_MinRange > 0.0f )
	{
		float adjMinRange = action->m_MinRange + m_bounds->m_radius + target->m_bounds->m_radius;
		if( delta.within( adjMinRange ) )
		{
			// Too close... avoid it if allowed by the current stance.
			if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->AllowsMovement() )
			{
				PopOrder();
				m_actor->SetAnimationState( "idle", false, 1.f, 0.f, false, L"" );
				return false;		// We're not allowed to move at all by the current stance
			}

			entf_set(ENTF_SHOULD_RUN);
			ChooseMovementSpeed( action->m_MinRange );
			
			// The pathfinder will push its result in front of the current order
			if( !g_Pathfinder.RequestAvoidPath( me, current, action->m_MinRange + 2.0f ) )
			{
				m_actor->SetAnimationState( "idle", false, 1.f, 0.f, false, L"" );	// Nothing we can do.. maybe we'll find a better target
				PopOrder();
			}

			return false;
		}
	}

	if( !delta.within( adjRange ) )
	{
		// Too far away at the moment, chase after the target if allowed...
		if( current->m_source == CEntityOrder::SOURCE_UNIT_AI && !m_stance->AllowsMovement() )
		{
			PopOrder();
			return false;
		}

		// We're aiming to end up at a location just inside our maximum range
		// (is this good enough?)
		delta = delta.Normalize() * ( adjRange - m_bounds->m_radius );

		ChooseMovementSpeed(deltaLength);
	
		current->m_target_location = (CVector2D)target->m_position - delta;

		HEntity collide;
		float timeLeft;
		switch( ProcessGotoHelper( current, timestep_millis, collide, timeLeft ) )
		{
		case REACHED_DESTINATION:
		case ALREADY_AT_DESTINATION:
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
			// We have a collision. Path around it.
			CEntityOrder avoidance;
			avoidance.m_type = CEntityOrder::ORDER_GOTO_COLLISION;
			CVector2D right;
			right.x = m_ahead.y; right.y = -m_ahead.x;
			CVector2D avoidancePosition;

			// Which is the shortest diversion, going left or right?
			// (Weight a little towards the right, to stop both units dodging the same way)

			if( ( collide->m_bounds->m_pos - m_bounds->m_pos ).Dot( right ) < 1 )
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
		m_ahead = delta.Normalize();
		entf_clear(ENTF_IS_RUNNING);
	}

	m_actor->SetAnimationState( animation, false, 1000.f / (float)action->m_Speed, 0.f, false, L"" );
//	m_actor->SetAnimationSync( (float)( action->m_Speed / 2) / 1000.f );

	m_fsm_cyclepos = 0;

	return( false );
}

bool CEntity::ProcessGoto( CEntityOrder* current, int UNUSED(timestep_millis) )
{
	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_target_location;
	float Distance = ( path_to - pos ).Length();
	
	CEntityOrder::EOrderSource source = current->m_source;
	m_orderQueue.pop_front();
	// pop_front may delete 'current', so we mustn't use it after this point
	
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		//entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	ChooseMovementSpeed( Distance );

	// The pathfinder will push its result back into this unit's queue.

	g_Pathfinder.RequestPath( me, path_to, source );

	return( true );
}

bool CEntity::ProcessGotoWaypoint( CEntityOrder* current, int UNUSED(timestep_milli), bool contact )
{
	CVector2D pos( m_position.X, m_position.Z );
	CVector2D path_to = current->m_target_location;
	float Distance = ( path_to - pos ).Length();
	
	CEntityOrder::EOrderSource source = current->m_source;
	float pathfinder_radius = current->m_pathfinder_radius;
	m_orderQueue.pop_front();
	// pop_front may delete 'current', so we mustn't use it after this point
		
	// Let's just check we're going somewhere...
	if( Distance < 0.1f ) 
	{
		entf_clear(ENTF_IS_RUNNING);
		//entf_clear(ENTF_SHOULD_RUN);
		return( false );
	}

	ChooseMovementSpeed( Distance );

#ifdef USE_DCDT
	//Kai: invoking triangulation or original A* pathfinding
	if(g_TriPathfind)
	{
		g_Pathfinder.RequestTriangulationPath( me, path_to, contact, pathfinder_radius, source );
	}
	else
#endif // USE_DCDT
	{
		g_Pathfinder.RequestLowLevelPath( me, path_to, contact, pathfinder_radius, source );
	}
	

	

	return( true );
}

bool CEntity::ProcessPatrol( CEntityOrder* current, int UNUSED(timestep_millis) )
{
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

bool CEntity::ProcessProduce( CEntityOrder* order )
{
	CEventStartProduction evt( order->m_produce_type, order->m_name );
	if( DispatchEvent( &evt ) && evt.GetTime() >= 0 )
	{
		m_productionQueue->AddItem( order->m_produce_type, order->m_name, evt.GetTime() );
	}
	return( false );
}
