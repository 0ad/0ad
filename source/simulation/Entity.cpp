#include "precompiled.h"

#include "graphics/GameView.h"
#include "graphics/Model.h"
#include "graphics/Sprite.h"
#include "graphics/Terrain.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "ps/Game.h"
#include "ps/Interact.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scripting/ScriptableComplex.inl"

#include "Aura.h"
#include "Collision.h"
#include "Entity.h"
#include "EntityFormation.h"
#include "EntityManager.h"
#include "EntityTemplate.h"
#include "EntityTemplateCollection.h"
#include "EventHandlers.h"
#include "Formation.h"
#include "FormationManager.h"
#include "PathfindEngine.h"
#include "ProductionQueue.h"
#include "Simulation.h"
#include "Stance.h"
#include "TechnologyCollection.h"
#include "TerritoryManager.h"

#include <algorithm>

#include "ps/GameSetup/Config.h"

const float MAX_ROTATION_RATE = 2*PI; // radians per second

CEntity::CEntity( CEntityTemplate* base, CVector3D position, float orientation, const std::set<CStr8>& actorSelections, const CStrW* building )
{
	ent_flags = 0;

	m_position = position;
	m_orientation.X = 0;
	m_orientation.Y = orientation;
	m_orientation.Z = 0;
	m_ahead.x = sin( m_orientation.Y );
	m_ahead.y = cos( m_orientation.Y );
	m_position_previous = m_position;
	m_orientation_smoothed = m_orientation_previous = m_orientation;
	m_player = NULL;

	m_productionQueue = new CProductionQueue( this );
	m_stance = new CHoldStance( this );

	for( int t = 0; t < EVENT_LAST; t++ )
	{
		AddProperty( EventNames[t], &m_EventHandlers[t], false );
		AddHandler( t, &m_EventHandlers[t] );
	}

	m_collisionPatch = NULL;

	// Set our parent unit and build us an actor.
	m_actor = NULL;
	m_bounds = NULL;

	m_lastState = -1;
	entf_set(ENTF_TRANSITION);
	m_fsm_cyclepos = NOT_IN_CYCLE;

	m_base = base;

	m_actorSelections = actorSelections;
	LoadBase();

	if( m_bounds )
	m_bounds->SetPosition( m_position.X, m_position.Z );

	m_graphics_position = m_position;
	m_graphics_orientation = m_orientation;
	m_actor_transform_valid = false;
	entf_clear(ENTF_HAS_RALLY_POINT);

	entf_clear(ENTF_DESTROYED);

	m_selected = false;
	entf_clear(ENTF_IS_RUNNING);
	entf_clear(ENTF_SHOULD_RUN);
	entf_clear(ENTF_TRIGGER_RUN);

	entf_clear(ENTF_HEALTH_DECAY);

	m_frameCheck = 0;
	m_lastCombatTime = -100;
	m_lastRunTime = -100;
	m_currentNotification = 0;
	m_currentRequest = 0;
	entf_set(ENTF_DESTROY_NOTIFIERS);

	m_formationSlot = (size_t)-1;
	m_formation = -1;
	m_grouped = -1;

	if( building )
		m_building = *building;

	m_extant = true;
	m_visible = true;

	m_rallyPoint = m_position;

	m_associatedTerritory = NULL;
	
	m_player = g_Game->GetPlayer( 0 );
}

CEntity::~CEntity()
{
	if( m_actor )
	{
		g_Game->GetWorld()->GetUnitManager().RemoveUnit( m_actor );
		delete( m_actor );
	}

	if( m_bounds )
	{
		delete( m_bounds );
	}

	delete m_productionQueue;

	delete m_stance;

	for( AuraTable::iterator it = m_auras.begin(); it != m_auras.end(); it++ )
	{
		delete it->second;
	}
	m_auras.clear();

	entf_set(ENTF_DESTROY_NOTIFIERS);
	for ( size_t i=0; i<m_listeners.size(); i++ )
		m_listeners[i].m_sender->DestroyNotifier( this );
	m_listeners.clear();
	DestroyAllNotifiers();

	CEntity* remove = this;
	g_FormationManager.RemoveUnit(remove);
}

void CEntity::LoadBase()
{
	size_t previous_unit_id = invalidUnitId;

	if( m_actor )
	{
		previous_unit_id = m_actor->GetID();
		g_Game->GetWorld()->GetUnitManager().RemoveUnit( m_actor );
		delete( m_actor );
		m_actor = NULL;
	}
	if( m_bounds )
	{
		delete( m_bounds );
		m_bounds = NULL;
	}

	CStr actorName ( m_base->m_actorName ); // convert CStrW->CStr8

	m_actor = g_Game->GetWorld()->GetUnitManager().CreateUnit( actorName, this, m_actorSelections );
	if( m_actor )
		m_actor->SetID( previous_unit_id );

	// Set up our instance data

	SetBase( m_base );
	m_classes.SetParent( &( m_base->m_classes ) );
	SetNextObject( m_base );

	if( m_base->m_bound_type == CBoundingObject::BOUND_CIRCLE )
	{
		m_bounds = new CBoundingCircle( m_position.X, m_position.Z, m_base->m_bound_circle );
	}
	else if( m_base->m_bound_type == CBoundingObject::BOUND_OABB )
	{
		m_bounds = new CBoundingBox( m_position.X, m_position.Z, m_ahead, m_base->m_bound_box );
	}

	m_actor_transform_valid = false;

	if( m_player )
	{
		// Make sure the actor has the right player colour
		m_actor->SetPlayerID( m_player->GetPlayerID() );
	}

	// Re-enter all our auras so they can take into account our new traits
	ExitAuras();

	// Resize sectors array	
	m_sectorValues.resize(m_base->m_sectorDivs);
	for ( int i=0; i<m_base->m_sectorDivs; ++i )
		m_sectorValues[i] = false;
}

void CEntity::initAuraData()
{
	if ( m_auras.empty() )
		return;
	m_unsnappedPoints.resize(m_auras.size());
	size_t i=0;
	for ( AuraTable::iterator it=m_auras.begin(); it!=m_auras.end(); ++it, ++i )
	{
		m_unsnappedPoints[i].resize(AURA_CIRCLE_POINTS);
		float radius = it->second->m_radius;

		for ( int j=0; j<AURA_CIRCLE_POINTS; ++j )
		{
			float val = j * 2*PI / (float)AURA_CIRCLE_POINTS;
			m_unsnappedPoints[i][j] = CVector2D( cosf(val)*radius, 
											sinf(val)*radius );
		}
	}
}

void CEntity::removeObstacle()
{
#ifdef USE_DCDT
	if(g_Pathfinder.dcdtInitialized)
	{
		g_Pathfinder.dcdtPathfinder.remove_polygon(m_dcdtId);
		g_Pathfinder.dcdtPathfinder.DeleteAbstraction();
		g_Pathfinder.dcdtPathfinder.Abstract();

		if(g_ShowPathfindingOverlay)
		{
			g_Pathfinder.drawTriangulation();
		}
	}
#endif // USE_DCDT
}

void CEntity::Kill(bool keepActor)
{
	if( entf_get( ENTF_DESTROYED ) )
	{
		return;		// We were already killed this frame
	}
	entf_set(ENTF_DESTROYED);

	CEventDeath evt;
	DispatchEvent( &evt );

	g_FormationManager.RemoveUnit(this);
	
	entf_set(ENTF_DESTROY_NOTIFIERS);
	for ( size_t i=0; i<m_listeners.size(); i++ )
		m_listeners[i].m_sender->DestroyNotifier( this );
	m_listeners.clear();
	DestroyAllNotifiers();

	for( AuraTable::iterator it = m_auras.begin(); it != m_auras.end(); it++ )
	{
		it->second->RemoveAll();
		delete it->second;
	}
	m_auras.clear();

	ExitAuras();

	ClearOrders();

	SAFE_DELETE(m_bounds);

	m_extant = false;

	UpdateCollisionPatch();
	
	//Kai: added to remove the entity in the polygon soup (for triangulation)
	removeObstacle();
	

	g_Selection.RemoveAll( me );

	g_EntityManager.m_refd[me.m_handle] = false; // refd must be made false when DESTROYED is set
	g_EntityManager.SetDeath(true); // remember that a unit died this frame

	g_EntityManager.RemoveUnitCount(this);	//Decrease population

	// If we have a death animation and want to keep the actor, play that animation
	if( keepActor && m_actor && 
		m_actor->HasAnimation( "death" ) )
	{
		// Prevent "wiggling" as we try to interpolate between here and our death position (if we were moving)
		m_graphics_position = m_position;
		m_position_previous = m_position;
		m_graphics_orientation = m_orientation;
		m_orientation_smoothed = m_orientation;
		m_orientation_previous = m_orientation;

		SnapToGround();
		
		// Conform to the ground
		CVector2D targetXZ = g_Game->GetWorld()->GetTerrain()->GetSlopeAngleFace(this);
		m_orientation.X = clamp( targetXZ.x, -1.0f, 1.0f );
		m_orientation.Z = clamp( targetXZ.y, -1.0f, 1.0f );
		m_orientation_unclamped.x = targetXZ.x;
		m_orientation_unclamped.y = targetXZ.y;

		UpdateActorTransforms();
		m_actor_transform_valid = true;

		// Play death animation and keep the actor in the game in a dead state 
		// (TODO: remove the actor after some time through some kind of fading mechanism)
		m_actor->SetAnimationState( "death", true );
	}
	else
	{
		g_Game->GetWorld()->GetUnitManager().DeleteUnit( m_actor );
		m_actor = NULL;

		me = HEntity(); // Will deallocate the entity, assuming nobody else has a reference to it
	}
}

void CEntity::SetPlayer(CPlayer *pPlayer)
{
	m_player = pPlayer;

	// This should usually be called CUnit::SetPlayerID, so we don't need to
	// update the actor here.

	// If we're a territory centre, change the territory's owner
	if( m_associatedTerritory )
		m_associatedTerritory->owner = pPlayer;
}

void CEntity::UpdateActorTransforms()
{
	CMatrix3D m;
	CMatrix3D mXZ;
	float Cos = cosf( m_graphics_orientation.Y );
	float Sin = sinf( m_graphics_orientation.Y );

	m._11=-Cos;  m._12=0.0f; m._13=-Sin;  m._14=0.0f;
	m._21=0.0f; m._22=1.0f; m._23=0.0f; m._24=0.0f;
	m._31=Sin; m._32=0.0f; m._33=-Cos;  m._34=0.0f;
	m._41=0.0f; m._42=0.0f; m._43=0.0f; m._44=1.0f;

	mXZ.SetXRotation( m_graphics_orientation.X );
	mXZ.RotateZ( m_graphics_orientation.Z );
	mXZ = m*mXZ;
	mXZ.Translate(m_graphics_position);

	if( m_actor )
		m_actor->GetModel()->SetTransform( mXZ );
}

void CEntity::SnapToGround()
{
	m_graphics_position.Y = GetAnchorLevel( m_graphics_position.X, m_graphics_position.Z );
}

void CEntity::UpdateXZOrientation()
{
	// Make sure m_ahead is correct
	m_ahead.x = sin( m_orientation.Y );
	m_ahead.y = cos( m_orientation.Y );

	CVector2D targetXZ = g_Game->GetWorld()->GetTerrain()->GetSlopeAngleFace(this);

	if ( !m_base ) 
	{
		return;
	}

	m_orientation.X = clamp( targetXZ.x, -m_base->m_anchorConformX, m_base->m_anchorConformX );
	m_orientation.Z = clamp( targetXZ.y, -m_base->m_anchorConformZ, m_base->m_anchorConformZ );
	m_orientation_unclamped.x = targetXZ.x;
	m_orientation_unclamped.y = targetXZ.y;
}

jsval CEntity::GetClassSet()
{
	CStrW result = m_classes.GetMemberList(); 
	return( ToJSVal( result ) );
}

void CEntity::SetClassSet( jsval value )
{
	CStr memberCmdList = ToPrimitive<CStrW>( value );
	m_classes.SetFromMemberList(memberCmdList);

	RebuildClassSet();
}

void CEntity::RebuildClassSet()
{
	m_classes.Rebuild();
	InheritorsList::iterator it;
	for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
		(*it)->RebuildClassSet();
}

void CEntity::Update( int timestep )
{
	if( !m_extant ) return;

	m_position_previous = m_position;
	m_orientation_previous = m_orientation_smoothed;

	CalculateRegen( timestep );

	if ( entf_get(ENTF_TRIGGER_RUN) )
		m_frameCheck++;

	if ( m_frameCheck != 0 )
	{
		entf_set(ENTF_SHOULD_RUN);
		entf_clear(ENTF_TRIGGER_RUN);
		m_frameCheck = 0;
	}

	m_productionQueue->Update( timestep );

	// Note: aura processing is done before state processing because the state
	// processing code is filled with all kinds of returns

	PROFILE_START( "aura processing" );

	for( AuraTable::iterator it = m_auras.begin(); it != m_auras.end(); it++ )
	{
		it->second->Update( timestep );
	}

	PROFILE_END( "aura processing" );

	UpdateOrders( timestep );


	// Calculate smoothed rotation: rotate around Y by at most MAX_ROTATION_RATE per second

	float delta = m_orientation.Y - m_orientation_smoothed.Y;
	// Wrap delta to -PI..PI
	delta = fmod(delta + PI, 2*PI); // range -2PI..2PI
	if (delta < 0) delta += 2*PI; // range 0..2PI
	delta -= PI; // range -PI..PI
	// Clamp to max rate
	float deltaClamped = clamp(delta, -MAX_ROTATION_RATE*timestep/1000.f, +MAX_ROTATION_RATE*timestep/1000.f);
	// Calculate new orientation, in a peculiar way in order to make sure the
	// result gets close to m_orientation (rather than being n*2*PI out)
	float newY = m_orientation.Y + deltaClamped - delta;
	// Apply the smoothed rotation
	m_orientation_smoothed = CVector3D(
		m_orientation.X,
		newY,
		m_orientation.Z
	);
}

void CEntity::UpdateOrders( int timestep )
{
	// The process[...] functions return 'true' if the order at the top of the stack
	// still needs to be (re-)evaluated; else 'false' to terminate the processing of
	// this entity in this timestep.

	PROFILE_START( "state processing" );

	if( entf_get(ENTF_IS_RUNNING) )
	{
		m_lastRunTime = g_Game->GetSimulation()->GetTime();
	}

	if( m_orderQueue.empty() )
	{
		// We are idle. Tell our stance in case it wants us to do something.
		PROFILE( "unit ai" );
		m_stance->OnIdle();
	}

	while( !m_orderQueue.empty() )
	{
		CEntityOrder* current = &m_orderQueue.front();
		CStr name = me;
#ifdef DEBUG_SYNCHRONIZATION
		debug_printf("Order for %ls: %d (src %d)\n", 
			m_base->m_Tag.c_str(), current->m_type, current->m_source);
#endif

		if( current->m_type != m_lastState )
		{
			entf_set(ENTF_TRANSITION);
			m_fsm_cyclepos = NOT_IN_CYCLE;

			PROFILE( "state transition / order" );

			CEntity* target = NULL;
			if( current->m_target_entity )
				target = &( *( current->m_target_entity ) );

			CVector3D worldPosition = (CVector3D)current->m_target_location;

			CEventOrderTransition evt( m_lastState, current->m_type, target, worldPosition );

			if( !DispatchEvent( &evt ) )
			{
				m_orderQueue.pop_front();
				continue;
			}
			else if( target )
			{
				current->m_target_location = worldPosition;
				current->m_target_entity = target->me;
			}

			m_lastState = current->m_type;
		}
		else
		{
			entf_clear(ENTF_TRANSITION);
		}

		switch( current->m_type )
		{
			case CEntityOrder::ORDER_GOTO_NOPATHING:
			case CEntityOrder::ORDER_GOTO_COLLISION:
			case CEntityOrder::ORDER_GOTO_SMOOTHED:
				if( ProcessGotoNoPathing( current, timestep ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO_NOPATHING_CONTACT:
				if( ProcessGotoNoPathingContact( current, timestep ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_CONTACT_ACTION:
				// Choose to run if and only if order.m_run is set
				entf_set_to(ENTF_TRIGGER_RUN, current->m_run);
				if( !entf_get(ENTF_TRIGGER_RUN) )
					entf_set_to(ENTF_SHOULD_RUN, false);
				if( ProcessContactAction( current, timestep, true ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_START_CONSTRUCTION:
				{
					CEventStartConstruction evt( current->m_new_obj );
					m_orderQueue.pop_front();
					DispatchEvent( &evt );
				}
				break;
			case CEntityOrder::ORDER_PRODUCE:
				ProcessProduce( current );
				m_orderQueue.pop_front();
				break;
			case CEntityOrder::ORDER_CONTACT_ACTION_NOPATHING:
				if( ProcessContactActionNoPathing( current, timestep ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO_WAYPOINT:
				if ( ProcessGotoWaypoint( current, timestep, false ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT:
				if ( ProcessGotoWaypoint( current, timestep, true ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO:
			case CEntityOrder::ORDER_RUN:
				// Choose to run if and only if type == ORDER_RUN
				entf_set_to(ENTF_TRIGGER_RUN, current->m_type == CEntityOrder::ORDER_RUN);
				if( !entf_get(ENTF_TRIGGER_RUN) )
					entf_set_to(ENTF_SHOULD_RUN, false);
				if( ProcessGoto( current, timestep ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_PATROL:
				if( ProcessPatrol( current, timestep ) )
					break;
				UpdateCollisionPatch();
				return;
			case CEntityOrder::ORDER_PATH_END_MARKER:
				m_orderQueue.pop_front();
				break;
			default:
				debug_warn( "Invalid entity order" );
		}
	}

	if( m_orderQueue.empty() ) 
	{
		// If we have no orders, stop running
		entf_clear(ENTF_IS_RUNNING);
		entf_clear(ENTF_SHOULD_RUN);
	}

	PROFILE_END( "state processing" );

	// If we get to here, it means we're idle or dead (no orders); update the animation

	if( m_actor )
	{
		PROFILE( "animation updates" );
		if( m_extant )
		{
			if( ( m_lastState != -1 ) || !m_actor->GetModel()->GetAnimation() )
			{
				m_actor->SetAnimationState( "idle" );
			}
		}
	}

	if( m_lastState != -1 )
	{
		PROFILE( "state transition event" );
		CVector3D vec(0, 0, 0);
		CEventOrderTransition evt( m_lastState, -1, 0, vec );
		DispatchEvent( &evt );

		m_lastState = -1;
	}
}

void CEntity::UpdateCollisionPatch()
{
	std::vector<CEntity*>* newPatch = g_EntityManager.GetCollisionPatch( this );
	if( newPatch != m_collisionPatch )
	{
		if( m_collisionPatch )
		{
			// remove ourselves from old patch
			std::vector<CEntity*>& old = *m_collisionPatch;
			if( old.size() == 1 )
			{
				// we were the only ones there, just pop us
				old.pop_back();
			}
			else
			{
				// find our location and put in the last guy in the patch, then pop back
				for( size_t i=0; i < old.size(); i++ )
				{
					if( old[i] == this )
					{
						old[i] = old.back();
						old.pop_back();
						break;
					}
				}
			}
		}

		if( newPatch )
		{
			// add ourselves to new patch
			newPatch->push_back( this );
			m_collisionPatch = newPatch;
		}
	}
}

#if AURA_TEST
void CEntity::UpdateAuras( int timestep_millis )
{
	std::vector<SAura>::iterator it_a;
	for( it_a = m_Auras.begin(); it_a != m_Auras.end(); it_a++ )
	{
		SAuraData& d = it_a->m_Data;
		std::set<CEntity*>
		& inRange = GetEntitiesWithinRange( m_position, d.m_Radius );

		std::vector<CEntity*>::iterator it1 = inRange.begin();
		std::vector<SAuraInstance>::iterator it2 = it_a->m_Influenced.begin();

		while( WORLD_IS_ROUND )
		{
			if( it1 == inRange.end() )
			{
				// No more in range => anything else in the influenced set must have gone
				// out of range.
				for( ; it2 != it_a->m_Influenced.end(); it2++ )
					UpdateAuras_LeaveRange( *it_a, it2->GetEntity() );

				break;
			}
			if( it2 == it_a->m_Influenced.end() )
			{
				// Everything else in the in-range set has only just come into range
				for( ; it1 != inRange.end(); it1++ )
					UpdateAuras_EnterRange( *it_a, *it );
				break;
			}
			CEntity* e1 = *it1, e2 = it2->GetEntity();
			if( e1 < e2 )
			{
				// A new entity e1 has just come into range.
				// Check to see if it can be affected by the aura.
				UpdateAuras_EnterRange( *it_a, e1 );
				++it1;
			}
			else if( e1 == e2 )
			{
				// The entity e1/e2 was previously in range, and still is.
				UpdateAuras_Normal( *it_a, e1 );
				++it1;
				++it2;
			}
			else
			{
				// The entity e2 was previously in range, but is no longer.
				UpdateAuras_LeaveRange( *it_a, e2 );
				++it2;
			}
		}
	}
}

void UpdateAuras_EnterRange( SAura& aura, CEntity* e )
{
	if( aura.m_Recharge )
		return( false );
	// Check to see if the entity is eligable
	if( !UpdateAuras_IsEligable( aura.m_Data, e ) )
		return; // Do nothing.

	SAuraInstance ai;
	ai.m_Influenced = e;
	ai.m_EnteredRange = ai.m_LastInRange = 0;
	ai.m_Applied = -1;

	// If there's no timer, apply the effect now.
	if( aura.m_Data.m_Time == 0 )
	{
		e->ApplyAuraEffect( aura.m_Data );
		ai.m_Applied = 0;
		aura.m_Recharge = aura.m_Data.m_Cooldown;
	}

	aura.m_Influenced.push_back( ai );
}

void UpdateAuras_Normal( SAura& aura, CEntity* e )
{
	// Is the entity no longer eligable?
	if( !UpdateAuras_IsEligable( aura.m_Data, e ) )
	{
		//}
	}

	bool UpdateAuras_IsEligable( SAuraData& aura, CEntity* e )
	{
		if( e == this )
		{
			if( !( aura.m_Allegiance & SAuraData::SELF ) )
				return( false );
		}
		else if( e->m_player == GetGaiaPlayer() )
		{
			if( !( aura.m_Allegiance & SAuraData::GAIA ) )
				return( false );
		}
		else if( e->m_player == m_player )
		{
			if( !( aura.m_Allegiance & SAuraData::PLAYER ) )
				return( false );
		}
		// TODO: Allied players
		else
		{
			if( !( aura.m_Allegiance & SAuraData::ENEMY ) )
				return( false );
		}
		if( e->m_hp > e->m_hp_max * aura.m_Hitpoints )
			return( false );
		return( true );
	}
}
#endif

bool CEntity::Initialize()
{
	// Apply our player's active techs to ourselves (we do this here since m_player isn't yet set in the constructor)
	const std::vector<CTechnology*>& techs = m_player->GetActiveTechs();
	for( size_t i=0; i<techs.size(); i++ )
	{
		techs[i]->Apply( this );
	}

	// Dispatch the initialize script event
    CEventInitialize evt;
    if( !DispatchEvent( &evt ) ) 
	{
		//debug_printf("start construction failed, killing self\n");
		Kill();
		return false;
	}

	if( g_EntityManager.m_screenshotMode )
	{
		// Stay in Hold stance no matter what the init script wanted us to be
		m_stanceName = "hold";
		StanceChanged();
	}

	return true;
}

/*
void CEntity::Tick()
{
	CEventTick evt;
	DispatchEvent( &evt );
}
*/

void CEntity::ClearOrders()
{
	if ( m_orderQueue.empty() )
		return;
	CIdleEvent evt( m_orderQueue.front(), m_currentNotification );
	DispatchEvent(&evt);
	m_orderQueue.clear();
}
void CEntity::PopOrder()
{
	if ( m_orderQueue.empty() )
		return;
	CIdleEvent evt( m_orderQueue.front(), m_currentNotification );
	DispatchEvent(&evt);

	m_orderQueue.pop_front();
}
void CEntity::PushOrder( const CEntityOrder& order )
{
	CEventPrepareOrder evt( order.m_target_entity, order.m_type, order.m_action, order.m_name );
	if( DispatchEvent(&evt) )
	{
		if (order.m_type == CEntityOrder::ORDER_SET_RALLY_POINT)
		{
			// It doesn't make sense to queue this type of order; just set the rally point
			entf_set(ENTF_HAS_RALLY_POINT);
			m_rallyPoint = order.m_target_location;
		}
		else if (order.m_type == CEntityOrder::ORDER_SET_STANCE)
		{
			// It doesn't make sense to queue this type of order; just set the stance
			m_stanceName = order.m_name;
			StanceChanged();
		}
		else
		{
			m_orderQueue.push_back( order );
			if(evt.m_notifyType != CEntityListener::NOTIFY_NONE)
			{
				CheckListeners( evt.m_notifyType, evt.m_notifySource );
			}
		}
	}
}

void CEntity::DispatchNotification( const CEntityOrder& order, int type )
{
	CEventNotification evt( order, type );
	DispatchEvent( &evt );
}

struct isListenerSender
{
	CEntity* sender;
	isListenerSender(CEntity* sender) : sender(sender) {}
	bool operator()(CEntityListener& listener)
	{
		return listener.m_sender == sender;
	}
};

int CEntity::DestroyNotifier( CEntity* target )
{
	//Stop listening
	// (Don't just loop and use 'erase', because modifying the deque while
	// looping over it is a bit dangerous)
	std::deque<CEntityListener>::iterator newEnd = std::remove_if(
		target->m_listeners.begin(), target->m_listeners.end(),
		isListenerSender(this));
	target->m_listeners.erase(newEnd, target->m_listeners.end());

	//Get rid of our copy
	std::vector<CEntity*>::iterator newEnd2 = std::remove_if(
		m_notifiers.begin(), m_notifiers.end(),
		bind2nd(std::equal_to<CEntity*>(), target));
	int removed = m_notifiers.end() - newEnd2;
	//m_notifiers.erase(newEnd2, m_notifiers.end());
	m_notifiers.resize(m_notifiers.size() - removed);
	return removed;
}
void CEntity::DestroyAllNotifiers()
{
	debug_assert(entf_get(ENTF_DESTROY_NOTIFIERS));
	//Make them stop listening to us
	while ( ! m_notifiers.empty() )
		DestroyNotifier( m_notifiers[m_notifiers.size()-1] );
}
CEntityFormation* CEntity::GetFormation()
{
	if ( m_formation < 0 )
		return NULL;
	return g_FormationManager.GetFormation(m_formation);
}
void CEntity::DispatchFormationEvent( int type )
{
	CFormationEvent evt( type );
	DispatchEvent( &evt );
}
void CEntity::Repath()
{
	debug_printf("Repath\n");
	CVector2D destination;
	CEntityOrder::EOrderSource orderSource = CEntityOrder::SOURCE_PLAYER;

	if( m_orderQueue.empty() )
		return;

	while( !m_orderQueue.empty() &&
			( ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_COLLISION )
			  || ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_NOPATHING )
			  || ( m_orderQueue.front().m_type == CEntityOrder::ORDER_GOTO_SMOOTHED ) ) )
	{
		destination = m_orderQueue.front().m_target_location;
		orderSource = m_orderQueue.front().m_source;
		m_orderQueue.pop_front();
	}
	g_Pathfinder.RequestPath( me, destination, orderSource );
}

void CEntity::Reorient()
{
	m_graphics_orientation = m_orientation;
	m_orientation_previous = m_orientation;
	m_orientation_smoothed = m_orientation;

	m_ahead.x = sin( m_orientation.Y );
	m_ahead.y = cos( m_orientation.Y );
	if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
		((CBoundingBox*)m_bounds)->SetOrientation( m_ahead );
	UpdateActorTransforms();
}

void CEntity::Teleport()
{
	m_position_previous = m_position;
	m_graphics_position = m_position;
	if (m_bounds)
		m_bounds->SetPosition( m_position.X, m_position.Z );
	UpdateActorTransforms();
	UpdateCollisionPatch();

	// TODO: Repath breaks things - entities get sent to (0,0) if they're moved in
	// Atlas. I can't see Teleport being used anywhere else important, so
	// hopefully it won't hurt to just remove it for now...
//	Repath();
}

void CEntity::StanceChanged()
{
	delete m_stance;
	m_stance = 0;

	if( m_stanceName == "aggress" )
	{
		m_stance = new CAggressStance( this );
	}
	else if( m_stanceName == "defend" )
	{
		m_stance = new CDefendStance( this );
	}
	else if( m_stanceName == "stand" )
	{
		m_stance = new CStandStance( this );
	}
	else	// m_stanceName == "hold" or undefined stance
	{
		m_stance = new CHoldStance( this );
	}
}

void CEntity::CheckSelection()
{
	if( m_selected )
	{
		if( !g_Selection.IsSelected( me ) )
			g_Selection.AddSelection( me );
	}
	else
	{
		if( g_Selection.IsSelected( me ) )
			g_Selection.RemoveSelection( me );
	}
}

void CEntity::CheckGroup()
{
	g_Selection.ChangeGroup( me, -1 ); // Ungroup
	if( ( m_grouped >= 0 ) && ( m_grouped < MAX_GROUPS ) )
		g_Selection.ChangeGroup( me, m_grouped );
}

void CEntity::Interpolate( float relativeoffset )
{
	CVector3D old_graphics_position = m_graphics_position;
	CVector3D old_graphics_orientation = m_graphics_orientation;

	relativeoffset = clamp( relativeoffset, 0.f, 1.f );

	m_graphics_position = ::Interpolate<CVector3D>( m_position_previous, m_position, relativeoffset );

	// Avoid wraparound glitches for interpolating angles.
	
	m_orientation.X = fmodf(m_orientation.X, 2*PI); // (ensure the following loops can't take forever)
	m_orientation.Y = fmodf(m_orientation.Y, 2*PI);
	m_orientation.Z = fmodf(m_orientation.Z, 2*PI);

	while( m_orientation.Y < m_orientation_previous.Y - PI )
		m_orientation_previous.Y -= 2 * PI;
	while( m_orientation.Y > m_orientation_previous.Y + PI )
		m_orientation_previous.Y += 2 * PI;

	while( m_orientation.X < m_orientation_previous.X - PI )
		m_orientation_previous.X -= 2 * PI;
	while( m_orientation.X > m_orientation_previous.X + PI )
		m_orientation_previous.X += 2 * PI;

	while( m_orientation.Z < m_orientation_previous.Z - PI )
		m_orientation_previous.Z -= 2 * PI;
	while( m_orientation.Z > m_orientation_previous.Z + PI )
		m_orientation_previous.Z += 2 * PI;

	UpdateXZOrientation();

	m_graphics_orientation = ::Interpolate<CVector3D>( m_orientation_previous, m_orientation_smoothed, relativeoffset );

	// Mark the actor transform data as invalid if the entity has moved since
	// the last call to 'interpolate'.
	// position.Y is ignored because we can't determine the new value without
	// calling SnapToGround, which is slow. TODO: This may need to be adjusted to
	// handle flying units or moving terrain.
	if( m_graphics_orientation != old_graphics_orientation ||
		m_graphics_position.X != old_graphics_position.X ||
		m_graphics_position.Z != old_graphics_position.Z		
		)
	{															
		m_actor_transform_valid = false;
	}
	// Update the actor transform data when necessary.
	if( !m_actor_transform_valid )
	{
		SnapToGround();
		UpdateActorTransforms();
		m_actor_transform_valid = true;
	}
}

void CEntity::InvalidateActor()
{
	m_actor_transform_valid = false;
}

float CEntity::GetAnchorLevel( float x, float z )
{
	CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
	float groundLevel = pTerrain->GetExactGroundLevel( x, z );
	if( m_base && m_base->m_anchorType==L"Ground" )
	{
		return groundLevel;
	}
	else
	{
		return std::max( groundLevel, g_Renderer.GetWaterManager()->m_WaterHeight );
	}
}

int CEntity::FindSector( int divs, float angle, float maxAngle, bool negative )
{
	float step=maxAngle/divs;
	if ( negative )
	{
		float tracker;
		int i=1, sectorRemainder;
		for ( tracker=-maxAngle/2.0f; tracker+step<0.0f; tracker+=step, ++i )
		{
			if ( angle > tracker && angle <= tracker+step ) 
				return i;
		}
		sectorRemainder = i;
		i=divs;
		for ( tracker=maxAngle/2.0f; tracker-step>0.0f; tracker-=step, --i )
		{
			if ( angle < tracker && angle >= tracker-step ) 
				return i;
		}
		return sectorRemainder;
	}
	else
	{
		int i=1;
		for (  float tracker=0.0f; tracker<maxAngle; tracker+=step, ++i )
		{
			if ( angle > tracker && angle <= tracker+step )
				return i;
		}
	}
	debug_warn("CEntity::FindSector() - invalid parameters passed.");
	return -1;
}

static inline float regen(float cur, float limit, float timestep, float regen_rate)
{
	if(regen_rate <= 0)
		return cur;
	return std::min(limit, cur + timestep / 1000.0f * regen_rate * limit );
}

static inline float decay(float cur, float limit, float timestep, float decay_rate)
{
	if(decay_rate <= 0)
		return cur;
	return std::max(0.0f, cur - timestep / 1000.0f * decay_rate * limit);
}

void CEntity::CalculateRegen(float timestep)
{
	// Health regen
	if(entf_get(ENTF_HEALTH_DECAY))
		m_healthCurr = decay(m_healthCurr, m_healthMax, timestep, m_healthDecayRate);
	else if(g_Game->GetSimulation()->GetTime() - m_lastCombatTime > m_healthRegenStart)
		m_healthCurr = regen(m_healthCurr, m_healthMax, timestep, m_healthRegenRate);
	
	// Stamina regen
	if( m_staminaMax > 0 )
	{
		if(entf_get(ENTF_IS_RUNNING))
			m_staminaCurr = decay(m_staminaCurr, m_staminaMax, timestep, m_runDecayRate);
		else if(m_orderQueue.empty())
			m_staminaCurr = regen(m_staminaCurr, m_staminaMax, timestep, m_runRegenRate);
	}
}

void CEntity::ExitAuras() 
{
	for( AuraSet::iterator it = m_aurasInfluencingMe.begin(); it != m_aurasInfluencingMe.end(); it++ )
	{
		(*it)->Remove( this );
	}
	m_aurasInfluencingMe.clear();
}
