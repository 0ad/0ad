// Last modified: May 15 2004, Mark Thompson (mark@wildfiregames.com)

#include "precompiled.h"

#include "Profile.h"

#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "Unit.h"
#include "Aura.h"
#include "ProductionQueue.h"
#include "Renderer.h"
#include "Model.h"
#include "Terrain.h"
#include "Interact.h"
#include "Collision.h"
#include "PathfindEngine.h"
#include "Game.h"
#include "scripting/JSInterface_Vector3D.h"
#include "MathUtil.h"
#include "CConsole.h"
#include "renderer/WaterManager.h"
#include "EntityFormation.h"
#include "simulation/FormationManager.h"
#include "BaseFormation.h"
#include "graphics/GameView.h"


extern CConsole* g_Console;
extern int g_xres, g_yres;

#include <algorithm>
using namespace std;

CEntity::CEntity( CBaseEntity* base, CVector3D position, float orientation, const std::set<CStrW>& actorSelections, CStrW building )
{
    m_position = position;
    m_orientation = orientation;
    m_ahead.x = sin( m_orientation );
    m_ahead.y = cos( m_orientation );
	
	/* Anything added to this list MUST be added to BaseEntity.cpp (and variables used should 
		also be added to BaseEntity.h */

    AddProperty( L"actions.move.speed", &m_speed );
	AddProperty( L"actions.move.run.speed", &( m_run.m_Speed ) );
	AddProperty( L"actions.move.run.rangemin", &( m_run.m_MinRange ) );
	AddProperty( L"actions.move.run.range", &( m_run.m_MaxRange ) );
	AddProperty( L"actions.move.run.regen_rate", &m_runRegenRate );
	AddProperty( L"actions.move.run.decay_rate", &m_runDecayRate );
    AddProperty( L"selected", &m_selected, false, (NotifyFn)&CEntity::checkSelection );
    AddProperty( L"group", &m_grouped, false, (NotifyFn)&CEntity::checkGroup );
    AddProperty( L"traits.extant", &m_extant );
    AddProperty( L"traits.corpse", &m_corpse );
    AddProperty( L"actions.move.turningradius", &m_turningRadius );
    AddProperty( L"position", &m_graphics_position, false, (NotifyFn)&CEntity::teleport );
    AddProperty( L"orientation", &m_graphics_orientation, false, (NotifyFn)&CEntity::reorient );
    AddProperty( L"player", &m_player, false, (NotifyFn)&CEntity::playerChanged );
    AddProperty( L"traits.health.curr", &m_healthCurr );
    AddProperty( L"traits.health.max", &m_healthMax );
    AddProperty( L"traits.health.bar_height", &m_healthBarHeight );
	AddProperty( L"traits.health.bar_size", &m_healthBarSize );
	AddProperty( L"traits.health.bar_width", &m_healthBarWidth );
	AddProperty( L"traits.health.border_height", &m_healthBorderHeight);
	AddProperty( L"traits.health.border_width", &m_healthBorderWidth );
	AddProperty( L"traits.health.border_name", &m_healthBorderName );
	AddProperty( L"traits.health.regen_rate", &m_healthRegenRate );
	AddProperty( L"traits.health.regen_start", &m_healthRegenStart );
	AddProperty( L"traits.health.decay_rate", &m_healthDecayRate );
	
	AddProperty( L"traits.stamina.curr", &m_staminaCurr );
    AddProperty( L"traits.stamina.max", &m_staminaMax );
    AddProperty( L"traits.stamina.bar_height", &m_staminaBarHeight );
	AddProperty( L"traits.stamina.bar_size", &m_staminaBarSize );
	AddProperty( L"traits.stamina.bar_width", &m_staminaBarWidth );
	AddProperty( L"traits.stamina.border_height", &m_staminaBorderHeight);
	AddProperty( L"traits.stamina.border_width", &m_staminaBorderWidth );
	AddProperty( L"traits.stamina.border_name", &m_staminaBorderName );
	AddProperty( L"traits.angle_penalty.sectors", &m_sectorDivs);
	AddProperty( L"traits.angle_penalty.value", &m_sectorPenalty );
	AddProperty( L"traits.rank.width", &m_rankWidth );
	AddProperty( L"traits.rank.height", &m_rankHeight );
	AddProperty( L"traits.rank.name", &m_rankName );
    AddProperty( L"traits.minimap.type", &m_minimapType );
    AddProperty( L"traits.minimap.red", &m_minimapR );
    AddProperty( L"traits.minimap.green", &m_minimapG );
    AddProperty( L"traits.minimap.blue", &m_minimapB );
    AddProperty( L"traits.anchor.type", &m_anchorType );
    AddProperty( L"traits.vision.los", &m_los );
    AddProperty( L"traits.vision.permanent", &m_permanent );
	AddProperty( L"last_combat_time", &m_lastCombatTime );
	AddProperty( L"building", &m_building );

	m_productionQueue = new CProductionQueue( this );
	AddProperty( L"production_queue", m_productionQueue );

    for( int t = 0; t < EVENT_LAST; t++ )
    {
        AddProperty( EventNames[t], &m_EventHandlers[t], false );
        AddHandler( t, &m_EventHandlers[t] );
    }

// FIXME: janwas: this was uninitialized, which leads to disaster if
// its value happens to be positive.
// setting to what seems to be a reasonable default.
	m_sectorDivs = 4;
	if ( m_sectorDivs >= 0 )
	{	
		m_sectorAngles.resize(m_sectorDivs);
		m_sectorValues.resize(m_sectorDivs);
		float step = DEGTORAD(360.0f / m_sectorDivs);
	
		for ( int i=0; i<m_sectorDivs; ++i )
		{
			m_sectorAngles[i] = cosf( step*i );
			m_sectorValues[i] = false;
		}
	}

    m_collisionPatch = NULL;

    // Set our parent unit and build us an actor.
    m_actor = NULL;
    m_bounds = NULL;

    m_lastState = -1;
    m_transition = true;
    m_fsm_cyclepos = NOT_IN_CYCLE;

    m_base = base;

	m_actorSelections = actorSelections;
	loadBase();

    if( m_bounds )
        m_bounds->setPosition( m_position.X, m_position.Z );

    m_position_previous = m_position;
    m_orientation_previous = m_orientation;

    m_graphics_position = m_position;
    m_graphics_orientation = m_orientation;
    m_actor_transform_valid = false;

    m_destroyed = false;

    m_selected = false;
	m_isRunning = false;
	m_shouldRun = false;
	m_triggerRun = false;

	m_healthDecay = false;

	m_frameCheck = 0;
	m_lastCombatTime = 0;
	m_currentNotification = 0;
	m_currentRequest = 0;
	m_destroyNotifiers = true;
	
	m_formationSlot =	-1;
	m_formation = -1;
    m_grouped = -1;

	m_building = building;

    m_player = g_Game->GetPlayer( 0 );

    Initialize();
}

CEntity::~CEntity()
{
    if( m_actor )
    {
        g_UnitMan.RemoveUnit( m_actor );
        delete( m_actor );
    }

    if( m_bounds )
    {
        delete( m_bounds );
    }

	delete m_productionQueue;

    for( AuraTable::iterator it = m_auras.begin(); it != m_auras.end(); it++ )
    {
        delete it->second;
    }
    m_auras.clear();
	
	for ( size_t i=0; i<m_listeners.size(); i++ )
		m_listeners[i].m_sender->DestroyNotifier( this );
	DestroyAllNotifiers();
	
	CEntity* remove = this;
	g_FormationManager.RemoveUnit(remove);
}

void CEntity::loadBase()
{
    if( m_actor )
    {
        g_UnitMan.RemoveUnit( m_actor );
        delete( m_actor );
        m_actor = NULL;
    }
    if( m_bounds )
    {
        delete( m_bounds );
        m_bounds = NULL;
    }

    CStr actorName ( m_base->m_actorName ); // convert CStrW->CStr8

	m_actor = g_UnitMan.CreateUnit( actorName, this, m_actorSelections );

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
}

void CEntity::kill()
{
    g_Selection.removeAll( me );
	
	CEntity* remove = this;
	g_FormationManager.RemoveUnit(remove);
	
	DestroyAllNotifiers();
   
	if( m_bounds )
        delete( m_bounds );
    m_bounds = NULL;

    m_extant = false;

    m_destroyed = true;
    //Shutdown(); // PT: tentatively removed - this seems to be called by ~CJSComplex, and we don't want to do it twice

    if( m_actor )
    {
        g_UnitMan.RemoveUnit( m_actor );
        delete( m_actor );
        m_actor = NULL;
    }
	
    updateCollisionPatch();
	

    me = HEntity(); // will deallocate the entity, assuming nobody else has a reference to it
}

void CEntity::SetPlayer(CPlayer *pPlayer)
{
    m_player = pPlayer;

    // Store the ID of the player in the associated model
    if( m_actor )
        m_actor->SetPlayerID( m_player->GetPlayerID() );
}

void CEntity::updateActorTransforms()
{
    CMatrix3D m;

    float s = sin( m_graphics_orientation );
    float c = cos( m_graphics_orientation );

    m._11 = -c;
    m._12 = 0.0f;
    m._13 = -s;
    m._14 = m_graphics_position.X;
    m._21 = 0.0f;
    m._22 = 1.0f;
    m._23 = 0.0f;
    m._24 = m_graphics_position.Y;
    m._31 = s;
    m._32 = 0.0f;
    m._33 = -c;
    m._34 = m_graphics_position.Z;
    m._41 = 0.0f;
    m._42 = 0.0f;
    m._43 = 0.0f;
    m._44 = 1.0f;

    if( m_actor )
        m_actor->GetModel()->SetTransform( m );
}

void CEntity::snapToGround()
{
    m_graphics_position.Y = getAnchorLevel( m_graphics_position.X, m_graphics_position.Z );
}

jsval CEntity::getClassSet()
{
    STL_HASH_SET<CStrW, CStrW_hash_compare>::iterator it;
    it = m_classes.m_Set.begin();
    CStrW result = L"";
    if( it != m_classes.m_Set.end() )
    {
        result = *( it++ );
        for( ; it != m_classes.m_Set.end(); it++ )
            result += L" " + *it;
    }
    return( ToJSVal( result ) );
}

void CEntity::setClassSet( jsval value )
{
    // Get the set that was passed in.
    CStr temp = ToPrimitive<CStrW>( value );
    CStr entry;

    m_classes.m_Added.clear();
    m_classes.m_Removed.clear();

    while( true )
    {
        long brk_sp = temp.Find( ' ' );
        long brk_cm = temp.Find( ',' );
        long brk = ( brk_sp == -1 ) ? brk_cm : ( brk_cm == -1 ) ? brk_sp : ( brk_sp < brk_cm ) ? brk_sp : brk_cm;

        if( brk == -1 )
        {
            entry = temp;
        }
        else
        {
            entry = temp.GetSubstring( 0, brk );
            temp = temp.GetSubstring( brk + 1, temp.Length() );
        }

        if( brk != 0 )
        {

            if( entry[0] == '-' )
            {
                entry = entry.GetSubstring( 1, entry.Length() );
                if( entry.Length() )
                    m_classes.m_Removed.push_back( entry );
            }
            else
            {
                if( entry[0] == '+' )
                    entry = entry.GetSubstring( 1, entry.Length() );
                if( entry.Length() )
                    m_classes.m_Added.push_back( entry );
            }
        }
        if( brk == -1 )
            break;
    }

    rebuildClassSet();
}

void CEntity::rebuildClassSet()
{
    m_classes.Rebuild();
    InheritorsList::iterator it;
    for( it = m_Inheritors.begin(); it != m_Inheritors.end(); it++ )
        (*it)->rebuildClassSet();
}

void CEntity::update( size_t timestep )
{
    m_position_previous = m_position;
    m_orientation_previous = m_orientation;
	
	CalculateRun( timestep );
	CalculateHealth( timestep );
	
	if ( m_triggerRun )
		m_frameCheck++;
	
	if ( m_frameCheck != 0 )
	{
		m_shouldRun = true;
		m_triggerRun = false;
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

    // The process[...] functions return 'true' if the order at the top of the stack
    // still needs to be (re-)evaluated; else 'false' to terminate the processing of
    // this entity in this timestep.

    PROFILE_START( "state processing" );

    while( !m_orderQueue.empty() )
    {
        CEntityOrder* current = &m_orderQueue.front();

        if( current->m_type != m_lastState )
        {
            m_transition = true;
            m_fsm_cyclepos = NOT_IN_CYCLE;

            PROFILE( "state transition / order" );

            CEntity* target = NULL;
            if( current->m_data[0].entity )
                target = &( *( current->m_data[0].entity ) );

            CVector3D worldPosition = (CVector3D)current->m_data[0].location;

            CEventOrderTransition evt( m_lastState, current->m_type, target, worldPosition );

            if( !DispatchEvent( &evt ) )
            {
                m_orderQueue.pop_front();
                continue;
            }
            else if( target )
            {
                current->m_data[0].location = worldPosition;
                current->m_data[0].entity = target->me;
            }

            m_lastState = current->m_type;
        }
        else
		{
            m_transition = false;
		}

        switch( current->m_type )
        {
            case CEntityOrder::ORDER_GOTO_NOPATHING:
            case CEntityOrder::ORDER_GOTO_COLLISION:
            case CEntityOrder::ORDER_GOTO_SMOOTHED:
				if( processGotoNoPathing( current, timestep ) )
					break;
				updateCollisionPatch();
				return;
            case CEntityOrder::ORDER_GENERIC:
				if( processGeneric( current, timestep ) )
					break;
				updateCollisionPatch();
				return;
            case CEntityOrder::ORDER_PRODUCE:
				processProduce( current );
				m_orderQueue.pop_front();
				break;
            case CEntityOrder::ORDER_GENERIC_NOPATHING:
				if( processGenericNoPathing( current, timestep ) )
					break;
				updateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO_WAYPOINT:
				if ( processGotoWaypoint( current, timestep, false ) )
					break;
				updateCollisionPatch();
				return;
			case CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT:
				if ( processGotoWaypoint( current, timestep, true ) )
					break;
				updateCollisionPatch();
				return;
            case CEntityOrder::ORDER_GOTO:
			case CEntityOrder::ORDER_RUN:
				if( processGoto( current, timestep ) )
					break;
				updateCollisionPatch();
				return;
            case CEntityOrder::ORDER_PATROL:
				if( processPatrol( current, timestep ) )
					break;
				updateCollisionPatch();
				return;
            case CEntityOrder::ORDER_PATH_END_MARKER:
				m_orderQueue.pop_front();
				break;
            default:
				debug_warn("Invalid entity order" );
		}
    }

    PROFILE_END( "state processing" );

    if( m_actor )
    {
        PROFILE( "animation updates" );
        if( m_extant )
        {
            if( ( m_lastState != -1 ) || !m_actor->GetModel()->GetAnimation() )
			{
				m_actor->SetEntitySelection( L"idle" );
                m_actor->SetRandomAnimation( "idle" );
			}
        }
        else if( !m_actor->GetModel()->GetAnimation() )
		{
			m_actor->SetEntitySelection( L"corpse" );
            m_actor->SetRandomAnimation( "corpse" );
		}
    }

    if( m_lastState != -1 )
    {
        PROFILE( "state transition event" );
        CEntity* d0;
        CVector3D d1;
        CEventOrderTransition evt( m_lastState, -1, d0, d1 );
        DispatchEvent( &evt );

        m_lastState = -1;
    }
}

void CEntity::updateCollisionPatch()
{
    vector<CEntity*>* newPatch = g_EntityManager.getCollisionPatch( this );
    if( newPatch != m_collisionPatch )
    {
        if( m_collisionPatch )
        {
            // remove ourselves from old patch
            vector<CEntity*>& old = *m_collisionPatch;
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

        if( m_extant )
        {
            // add ourselves to new patch
            newPatch->push_back( this );
            m_collisionPatch = newPatch;
        }
    }
}

#if AURA_TEST
void CEntity::UpdateAuras( size_t timestep_millis )
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

#endif

void CEntity::Initialize()
{
    CEventInitialize evt;
    DispatchEvent( &evt );
}

void CEntity::Tick()
{
    CEventTick evt;
    DispatchEvent( &evt );
}

void CEntity::clearOrders()
{
    if ( m_orderQueue.empty() )
		return;
	CIdleEvent evt( m_orderQueue.front(), m_currentNotification );
	DispatchEvent(&evt);
	m_orderQueue.clear();
}
void CEntity::popOrder()
{
	if ( m_orderQueue.empty() )
		return;
	CIdleEvent evt( m_orderQueue.front(), m_currentNotification );
	DispatchEvent(&evt);

	m_orderQueue.pop_front();
}
void CEntity::pushOrder( CEntityOrder& order )
{
	CEventPrepareOrder evt( order.m_data[0].entity, order.m_type, order.m_data[1].data, order.m_data[0].string );
	if( DispatchEvent(&evt) )
    {
		m_orderQueue.push_back( order );
		if(evt.m_notifyType != CEntityListener::NOTIFY_NONE) 
		{
			CheckListeners( evt.m_notifyType, evt.m_notifySource );
		}
    }
}

void CEntity::DispatchNotification( CEntityOrder order, int type )
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
	if (target->m_listeners.empty() || !m_destroyNotifiers)
		return 0;
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
	int removed = std::distance(newEnd2, m_notifiers.end());
	m_notifiers.erase(newEnd2, m_notifiers.end());
	return removed;
}
void CEntity::DestroyAllNotifiers()
{
	debug_assert(m_destroyNotifiers);
	//Make them stop listening to us
	while ( ! m_notifiers.empty() )
		DestroyNotifier( m_notifiers[0] );
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
void CEntity::repath()
{
    CVector2D destination;
    if( m_orderQueue.empty() )
        return;

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

void CEntity::reorient()
{
    m_orientation = m_graphics_orientation;
    m_ahead.x = sin( m_orientation );
    m_ahead.y = cos( m_orientation );
    if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
        ((CBoundingBox*)m_bounds)->setOrientation( m_ahead );
    updateActorTransforms();
}

void CEntity::teleport()
{
    m_position = m_graphics_position;
    m_bounds->setPosition( m_position.X, m_position.Z );
    updateCollisionPatch();
    repath();
}

void CEntity::playerChanged()
{
    if( m_actor )
        m_actor->SetPlayerID( m_player->GetPlayerID() );
}

void CEntity::checkSelection()
{
    if( m_selected )
    {
        if( !g_Selection.isSelected( me ) )
            g_Selection.addSelection( me );
    }
    else
    {
        if( g_Selection.isSelected( me ) )
            g_Selection.removeSelection( me );
    }
}

void CEntity::checkGroup()
{
    g_Selection.changeGroup( me, -1 ); // Ungroup
    if( ( m_grouped >= 0 ) && ( m_grouped < MAX_GROUPS ) )
        g_Selection.changeGroup( me, m_grouped );
}

void CEntity::interpolate( float relativeoffset )
{
    CVector3D old_graphics_position = m_graphics_position;
    float old_graphics_orientation = m_graphics_orientation;

    m_graphics_position = Interpolate<CVector3D>( m_position_previous, m_position, relativeoffset );

    // Avoid wraparound glitches for interpolating angles.
    while( m_orientation < m_orientation_previous - PI )
        m_orientation_previous -= 2 * PI;
    while( m_orientation > m_orientation_previous + PI )
        m_orientation_previous += 2 * PI;

    m_graphics_orientation = Interpolate<float>( m_orientation_previous, m_orientation, relativeoffset );

    // Mark the actor transform data as invalid if the entity has moved since
    // the last call to 'interpolate'.
    // position.Y is ignored because we can't determine the new value without
    // calling snapToGround, which is slow. TODO: This may need to be adjusted to
    // handle flying units or moving terrain.
    if( m_graphics_orientation != old_graphics_orientation ||
            m_graphics_position.X != old_graphics_position.X ||
            m_graphics_position.Z != old_graphics_position.Z )
        m_actor_transform_valid = false;

    // Update the actor transform data when necessary.
    if( !m_actor_transform_valid )
    {
        snapToGround();
        updateActorTransforms();
        m_actor_transform_valid = true;
    }
}

void CEntity::invalidateActor()
{
    m_actor_transform_valid = false;
}

void CEntity::render()
{
    if( !m_orderQueue.empty() )
    {
        std::deque<CEntityOrder>::iterator it;
        CBoundingObject* destinationCollisionObject;
        float x0, y0, x, y;

        x = m_orderQueue.front().m_data[0].location.x;
        y = m_orderQueue.front().m_data[0].location.y;

        for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
        {
            if( it->m_type == CEntityOrder::ORDER_PATROL )
                break;
            x = it->m_data[0].location.x;
            y = it->m_data[0].location.y;
        }
        destinationCollisionObject = getContainingObject( CVector2D( x, y ) );

        glShadeModel( GL_FLAT );
        glBegin( GL_LINE_STRIP );

        glVertex3f( m_position.X, m_position.Y + 0.25f, m_position.Z );

        x = m_position.X;
        y = m_position.Z;

        for( it = m_orderQueue.begin(); it < m_orderQueue.end(); it++ )
        {
            x0 = x;
            y0 = y;
            x = it->m_data[0].location.x;
            y = it->m_data[0].location.y;
            rayIntersectionResults r;
            CVector2D fwd( x - x0, y - y0 );
            float l = fwd.length();
            fwd = fwd.normalize();
            CVector2D rgt = fwd.beta();
            if( getRayIntersection( CVector2D( x0, y0 ), fwd, rgt, l, m_bounds->m_radius, destinationCollisionObject, &r ) )
            {
                glEnd();
                glBegin( GL_LINES );
                glColor3f( 1.0f, 0.0f, 0.0f );
                glVertex3f( x0 + fwd.x * r.distance, getAnchorLevel( x0 + fwd.x * r.distance, y0 + fwd.y * r.distance ) + 0.25f, y0 + fwd.y * r.distance );
                glVertex3f( r.position.x, getAnchorLevel( r.position.x, r.position.y ) + 0.25f, r.position.y );
                glEnd();
                glBegin( GL_LINE_STRIP );
                glVertex3f( x0, getAnchorLevel( x0, y0 ), y0 );
            }
            switch( it->m_type )
            {
                case CEntityOrder::ORDER_GOTO:
                glColor3f( 1.0f, 0.0f, 0.0f );
                break;
                case CEntityOrder::ORDER_GOTO_COLLISION:
                glColor3f( 1.0f, 0.5f, 0.5f );
                break;
                case CEntityOrder::ORDER_GOTO_NOPATHING:
                case CEntityOrder::ORDER_GOTO_SMOOTHED:
                glColor3f( 0.5f, 0.5f, 0.5f );
                break;
                case CEntityOrder::ORDER_PATROL:
                glColor3f( 0.0f, 1.0f, 0.0f );
                break;
                default:
                continue;
            }

            glVertex3f( x, getAnchorLevel( x, y ) + 0.25f, y );
        }

        glEnd();
        glShadeModel( GL_SMOOTH );
    }

    glColor3f( 1.0f, 1.0f, 1.0f );
    if( getCollisionObject( this ) )
        glColor3f( 0.5f, 0.5f, 1.0f );
    m_bounds->render( getAnchorLevel( m_position.X, m_position.Z ) + 0.25f ); //m_position.Y + 0.25f );
}

float CEntity::getAnchorLevel( float x, float z )
{
    CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
    float groundLevel = pTerrain->getExactGroundLevel( x, z );
    if( m_anchorType==L"Ground" )
    {
        return groundLevel;
    }
    else
    {
        return max( groundLevel, g_Renderer.GetWaterManager()->m_WaterHeight );
    }
}

void CEntity::renderSelectionOutline( float alpha )
{
    if( !m_bounds )
        return;

    if( getCollisionObject( this ) )
        glColor4f( 1.0f, 0.5f, 0.5f, alpha );
    else
    {
        const SPlayerColour& col = m_player->GetColour();
        glColor3f( col.r, col.g, col.b );
    }

    glBegin( GL_LINE_LOOP );

    CVector3D pos = m_graphics_position;

    switch( m_bounds->m_type )
    {
        case CBoundingObject::BOUND_CIRCLE:
        {
            float radius = ((CBoundingCircle*)m_bounds)->m_radius;
            for( int i = 0; i < SELECTION_CIRCLE_POINTS; i++ )
            {
                float ang = i * 2 * PI / (float)SELECTION_CIRCLE_POINTS;
                float x = pos.X + radius * sin( ang );
                float y = pos.Z + radius * cos( ang );
#ifdef SELECTION_TERRAIN_CONFORMANCE

                glVertex3f( x, getAnchorLevel( x, y ) + 0.25f, y );
#else

                glVertex3f( x, pos.Y + 0.25f, y );
#endif

            }
            break;
        }
        case CBoundingObject::BOUND_OABB:
        {
            CVector2D p, q;
            CVector2D u, v;
            q.x = pos.X;
            q.y = pos.Z;
            float d = ((CBoundingBox*)m_bounds)->m_d;
            float w = ((CBoundingBox*)m_bounds)->m_w;

            u.x = sin( m_graphics_orientation );
            u.y = cos( m_graphics_orientation );
            v.x = u.y;
            v.y = -u.x;

#ifdef SELECTION_TERRAIN_CONFORMANCE

            for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
            {
                p = q + u * d + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
                glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );
            }

            for( int i = SELECTION_BOX_POINTS; i > -SELECTION_BOX_POINTS; i-- )
            {
                p = q + u * ( d * (float)i / (float)SELECTION_BOX_POINTS ) - v * w;
                glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );
            }

            for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
            {
                p = q - u * d + v * ( w * (float)i / (float)SELECTION_BOX_POINTS );
                glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );
            }

            for( int i = -SELECTION_BOX_POINTS; i < SELECTION_BOX_POINTS; i++ )
            {
                p = q + u * ( d * (float)i / (float)SELECTION_BOX_POINTS ) + v * w;
                glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );
            }
#else
            p = q + u * h + v * w;
            glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );

            p = q + u * h - v * w;
            glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );

            p = q - u * h + v * w;
            glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );

            p = q + u * h + v * w;
            glVertex3f( p.x, getAnchorLevel( p.x, p.y ) + 0.25f, p.y );
#endif


            break;
        }
    }

    glEnd();
}
CVector2D CEntity::getScreenCoords( float height )
{
	CCamera &g_Camera=*g_Game->GetView()->GetCamera();

    float sx, sy;
    CVector3D above;
    above.X = m_position.X;
    above.Z = m_position.Z;
    above.Y = getAnchorLevel(m_position.X, m_position.Z) + height;
    g_Camera.GetScreenCoordinates(above, sx, sy);
	return CVector2D( sx, sy );
}
void CEntity::renderBarBorders()
{
	pglActiveTextureARB( GL_TEXTURE0_ARB );
	
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);
	
	if ( g_Selection.m_unitUITextures.find(m_healthBorderName) != g_Selection.m_unitUITextures.end() )
	{
		ogl_tex_bind( g_Selection.m_unitUITextures[m_healthBorderName] );
		CVector2D pos = getScreenCoords( m_healthBarHeight );
		
		float left = pos.x - m_healthBorderWidth/2;
		float right = pos.x + m_healthBorderWidth/2;
		pos.y = g_yres - pos.y;
		float bottom = pos.y + m_healthBorderHeight/2;
		float top = pos.y - m_healthBorderHeight/2;	
		
		glBegin(GL_QUADS);
	
		glTexCoord2f(0.0f, 0.0f); glVertex3f( left, bottom, 0 );
		glTexCoord2f(0.0f, 1.0f); glVertex3f( left, top, 0 );
		glTexCoord2f(1.0f, 1.0f); glVertex3f( right, top, 0 );
		glTexCoord2f(1.0f, 0.0f); glVertex3f( right, bottom, 0 );
	  
		glEnd();
	}
	if ( g_Selection.m_unitUITextures.find(m_staminaBorderName) != g_Selection.m_unitUITextures.end() )
	{
		ogl_tex_bind( g_Selection.m_unitUITextures[m_staminaBorderName] );
		CVector2D pos = getScreenCoords( m_staminaBarHeight );
		float left = pos.x - m_staminaBorderWidth/2;
		float right = pos.x + m_staminaBorderWidth/2;
		pos.y = g_yres - pos.y;
		float bottom = pos.y + m_staminaBorderHeight/2;
		float top = pos.y - m_staminaBorderHeight/2;	
		
		glBegin(GL_QUADS);
	
		glTexCoord2f(0.0f, 0.0f); glVertex3f( left, bottom, 0 );
		glTexCoord2f(0.0f, 1.0f); glVertex3f( left, top, 0 );
		glTexCoord2f(1.0f, 1.0f); glVertex3f( right, top, 0 );
		glTexCoord2f(1.0f, 0.0f); glVertex3f( right, bottom, 0 );
	  
		glEnd();
	}
}
void CEntity::renderHealthBar()
{
    if( !m_bounds )
        return;
    if( m_healthBarHeight < 0 )
        return;  // negative bar height means don't display health bar
	
    float fraction;
	if(m_healthMax == 0) fraction = 1.0f;
	else fraction = clamp(m_healthCurr / m_healthMax, 0.0f, 1.0f);

	CVector2D pos = getScreenCoords( m_healthBarHeight );
    float x1 = pos.x - m_healthBarSize/2;
    float x2 = pos.x + m_healthBarSize/2;
    float y = g_yres - pos.y;
	
	glLineWidth( m_healthBarWidth );
	glBegin(GL_LINES);

    // green part of bar
    glColor3f( 0, 1, 0 );
    glVertex3f( x1, y, 0 );
    glColor3f( 0, 1, 0 );
    glVertex3f( x1 + m_healthBarSize*fraction, y, 0 );

    // red part of bar
    glColor3f( 1, 0, 0 );
    glVertex3f( x1 + m_healthBarSize*fraction, y, 0 );
    glColor3f( 1, 0, 0 );
    glVertex3f( x2, y, 0 );

    glEnd();
	
	glLineWidth(1.0f);
	
}

void CEntity::renderStaminaBar()
{
    if( !m_bounds )
        return;
    if( m_staminaBarHeight < 0 )
        return;  // negative bar height means don't display stamina bar

    float fraction;
	if(m_staminaMax == 0) fraction = 1.0f;
	else fraction = clamp(m_staminaCurr / m_staminaMax, 0.0f, 1.0f);

    CVector2D pos = getScreenCoords( m_staminaBarHeight );
    float x1 = pos.x - m_staminaBarSize/2;
    float x2 = pos.x + m_staminaBarSize/2;
    float y = g_yres - pos.y;
	
	glLineWidth( m_staminaBarWidth );
    glBegin(GL_LINES);

    // blue part of bar
    glColor3f( 0, 0, 1 );
    glVertex3f( x1, y, 0 );
    glColor3f( 0, 0, 1 );
    glVertex3f( x1 + m_staminaBarSize*fraction, y, 0 );

    // red part of bar
    glColor3f( 1, 0, 0 );
    glVertex3f( x1 + m_staminaBarSize*fraction, y, 0 );
    glColor3f( 1, 0, 0 );
    glVertex3f( x2, y, 0 );

    glEnd();
	glLineWidth(1.0f);
}
void CEntity::renderRank()
{
	if( !m_bounds )
        return;
    if( m_rankHeight < 0 )
        return;  // negative height means don't display rank
	//Check for valid texture
	if( g_Selection.m_unitUITextures.find( m_rankName ) == g_Selection.m_unitUITextures.end() )
		return;
	
    CCamera *g_Camera=g_Game->GetView()->GetCamera();

    float sx, sy;
    CVector3D above;
    above.X = m_position.X;
    above.Z = m_position.Z;
    above.Y = getAnchorLevel(m_position.X, m_position.Z) + m_rankHeight;
    g_Camera->GetScreenCoordinates(above, sx, sy);
    int size = m_rankWidth/2;

    float x1 = sx - size;
    float x2 = sx + size;
    float y1 = g_yres - (sy - size);	//top
	float y2 = g_yres - (sy + size);	//bottom
	
	ogl_tex_bind(g_Selection.m_unitUITextures[m_rankName]);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 0.0f); glVertex3f( x1, y2, 0 );
    glTexCoord2f(0.0f, 1.0f); glVertex3f( x1, y1, 0 );
    glTexCoord2f(1.0f, 1.0f); glVertex3f( x2, y1, 0 );
    glTexCoord2f(1.0f, 0.0f); glVertex3f( x2, y2, 0 );

    glEnd();
}


void CEntity::CalculateRun(float timestep)
{
	if( m_staminaMax > 0 )
	{
		if ( m_isRunning && m_runDecayRate > 0 )
		{
			m_staminaCurr = max( 0.0f, m_staminaCurr - timestep / 1000.0f / m_runDecayRate * m_staminaMax );
		}
		else if ( m_orderQueue.empty() && m_runRegenRate > 0 )
		{
			m_staminaCurr = min( m_staminaMax, m_staminaCurr + timestep / 1000.0f / m_runRegenRate * m_staminaMax );
		}
	}
}

void CEntity::CalculateHealth(float timestep)
{
	if ( m_healthDecay && m_healthDecayRate > 0 )
	{
		m_healthCurr = max( 0.0f, m_healthCurr - timestep / 1000.0f / m_healthDecayRate * m_healthMax );
	}
	else if ( m_healthRegenRate > 0 && g_Game->GetTime() - m_lastCombatTime > m_healthRegenStart )
	{
		m_healthCurr = min( m_healthMax, m_healthCurr + timestep / 1000.0f / m_healthRegenRate * m_healthMax );
	}
}

/*

 Scripting interface

*/

// Scripting initialization

void CEntity::ScriptingInit()
{
    AddMethod<jsval, &CEntity::ToString>( "toString", 0 );
    AddMethod<bool, &CEntity::OrderSingle>( "order", 1 );
    AddMethod<bool, &CEntity::OrderQueued>( "orderQueued", 1 );
	AddMethod<jsval, &CEntity::TerminateOrder>( "terminateOrder", 1 );
    AddMethod<bool, &CEntity::Kill>( "kill", 0 );
    AddMethod<bool, &CEntity::IsIdle>( "isIdle", 0 );
    AddMethod<bool, &CEntity::HasClass>( "hasClass", 1 );
    AddMethod<jsval, &CEntity::GetSpawnPoint>( "getSpawnPoint", 1 );
    AddMethod<jsval, &CEntity::AddAura>( "addAura", 3 );
    AddMethod<jsval, &CEntity::RemoveAura>( "removeAura", 1 );
    AddMethod<jsval, &CEntity::SetActionParams>( "setActionParams", 5 );
	AddMethod<bool, &CEntity::ForceCheckListeners>( "forceCheckListeners", 2 );
	AddMethod<bool, &CEntity::RequestNotification>( "requestNotification", 4 );
	AddMethod<jsval, &CEntity::DestroyNotifier>( "destroyNotifier", 1 );
	AddMethod<jsval, &CEntity::DestroyAllNotifiers>( "destroyAllNotifiers", 0 );
	AddMethod<jsval, &CEntity::TriggerRun>( "triggerRun", 1 );
	AddMethod<jsval, &CEntity::SetRun>( "setRun", 1 );
	AddMethod<jsval, &CEntity::GetRunState>( "getRunState", 0 );
	AddMethod<bool, &CEntity::IsInFormation>( "isInFormation", 0 );
	AddMethod<jsval, &CEntity::GetFormationBonus>( "getFormationBonus", 0 );
	AddMethod<jsval, &CEntity::GetFormationBonusType>( "getFormationBonusType", 0 );
	AddMethod<jsval, &CEntity::GetFormationBonusVal>( "getFormationBonusVal", 0 );
	AddMethod<jsval, &CEntity::GetFormationPenalty>( "getFormationPenalty", 0 );
	AddMethod<jsval, &CEntity::GetFormationPenaltyType>( "getFormationPenaltyType", 0 );
	AddMethod<jsval, &CEntity::GetFormationPenaltyVal>( "getFormationPenaltyVal", 0 );
	AddMethod<jsval, &CEntity::RegisterDamage>( "registerDamage", 0 );
	AddMethod<jsval, &CEntity::RegisterIdle>( "registerIdle", 0 );

    AddClassProperty( L"template", (CBaseEntity* CEntity::*)&CEntity::m_base, false, (NotifyFn)&CEntity::loadBase );
    AddClassProperty( L"traits.id.classes", (GetFn)&CEntity::getClassSet, (SetFn)&CEntity::setClassSet );

    CJSComplex<CEntity>::ScriptingInit( "Entity", Construct, 2 );
}

// Script constructor

JSBool CEntity::Construct( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
    debug_assert( argc >= 2 );

    CVector3D position;
    float orientation = (float)( PI * ( (double)( rand() & 0x7fff ) / (double)0x4000 ) );

    JSObject* jsBaseEntity = JSVAL_TO_OBJECT( argv[0] );
    CStrW templateName;

    CBaseEntity* baseEntity = NULL;
    if( JSVAL_IS_OBJECT( argv[0] ) ) // only set baseEntity if jsBaseEntity is a valid object
        baseEntity = ToNative<CBaseEntity>( cx, jsBaseEntity );

    if( !baseEntity )
    {
        try
        {
            templateName = g_ScriptingHost.ValueToUCString( argv[0] );
        }
        catch( PSERROR_Scripting_ConversionFailed )
        {
            *rval = JSVAL_NULL;
            JS_ReportError( cx, "Invalid template identifier" );
            return( JS_TRUE );
        }
        baseEntity = g_EntityTemplateCollection.getTemplate( templateName );
    }

    if( !baseEntity )
    {
        *rval = JSVAL_NULL;
        JS_ReportError( cx, "No such template: %s", CStr8(templateName).c_str() );
        return( JS_TRUE );
    }

    JSI_Vector3D::Vector3D_Info* jsVector3D = NULL;
    if( JSVAL_IS_OBJECT( argv[1] ) )
        jsVector3D = (JSI_Vector3D::Vector3D_Info*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( argv[1] ), &JSI_Vector3D::JSI_class, NULL );

    if( jsVector3D )
    {
        position = *( jsVector3D->vector );
    }

    if( argc >= 3 )
    {
        try
        {
            orientation = ToPrimitive<float>( argv[2] );
        }
        catch( PSERROR_Scripting_ConversionFailed )
        {
            // TODO: Net-safe random for this parameter.
            orientation = 0.0f;
        }
    }

	std::set<CStrW> selections; // TODO: let scripts specify selections?
    HEntity handle = g_EntityManager.create( baseEntity, position, orientation, selections );

    *rval = ToJSVal<CEntity>( *handle );
    return( JS_TRUE );
}

// Script-bound methods

jsval CEntity::ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
    wchar_t buffer[256];
    swprintf( buffer, 256, L"[object Entity: %ls]", m_base->m_Tag.c_str() );
    buffer[255] = 0;
    utf16string str16(buffer, buffer+wcslen(buffer));
    return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

bool CEntity::Order( JSContext* cx, uintN argc, jsval* argv, bool Queued )
{
    // This needs to be sorted (uses Scheduler rather than network messaging)
    debug_assert( argc >= 1 );

    int orderCode;

    try
    {
        orderCode = ToPrimitive<int>( argv[0] );
    }
    catch( PSERROR_Scripting_ConversionFailed )
    {
        JS_ReportError( cx, "Invalid order type" );
        return( false );
    }

    CEntityOrder newOrder;
	CEntity* target;

    (int&)newOrder.m_type = orderCode;

    switch( orderCode )
    {
        case CEntityOrder::ORDER_GOTO:
		case CEntityOrder::ORDER_RUN:
        case CEntityOrder::ORDER_PATROL:
			if( argc < 3 )
			{
				JS_ReportError( cx, "Too few parameters" );
				return( false );
			}
			try
			{
				newOrder.m_data[0].location.x = ToPrimitive<float>( argv[1] );
				newOrder.m_data[0].location.y = ToPrimitive<float>( argv[2] );
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid location" );
				return( false );
			}
			if ( orderCode == CEntityOrder::ORDER_RUN )
				m_triggerRun = true;
			//It's not a notification order
			if ( argc == 3 )
			{
				if ( m_destroyNotifiers )
				{
					m_currentRequest=0;
					DestroyAllNotifiers();
				}
			}
			break;
        case CEntityOrder::ORDER_GENERIC:
			if( argc < 3 )
			{
				JS_ReportError( cx, "Too few parameters" );
				return( false );
			}
			target = ToNative<CEntity>( argv[1] );
			if( !target )
			{
				JS_ReportError( cx, "Invalid target" );
				return( false );
			}
			newOrder.m_data[0].entity = target->me;
			try
			{
				newOrder.m_data[1].data = ToPrimitive<int>( argv[2] );
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid generic order type" );
				return( false );
			}
			//It's not a notification order
			if ( argc == 3 )
			{
				if ( m_destroyNotifiers )
				{
					m_currentRequest=0;
					DestroyAllNotifiers();
				}
			}
			break;
        case CEntityOrder::ORDER_PRODUCE:
			if( argc < 3 )
			{
				JS_ReportError( cx, "Too few parameters" );
				return( false );
			}
			try {
				newOrder.m_data[0].string = ToPrimitive<CStrW>(argv[2]);
				newOrder.m_data[1].data = ToPrimitive<int>(argv[1]);
			}
			catch( PSERROR_Scripting_ConversionFailed )
			{
				JS_ReportError( cx, "Invalid parameter types" );
				return( false );
			}
			break;
        default:
			JS_ReportError( cx, "Invalid order type" );
			return( false );
    }

    if( !Queued )
        clearOrders();
    pushOrder( newOrder );

    return( true );
}

bool CEntity::Kill( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
    CEventDeath evt;
    DispatchEvent( &evt );

    for( AuraTable::iterator it = m_auras.begin(); it != m_auras.end(); it++ )
    {
        it->second->RemoveAll();
        delete it->second;
    }
    m_auras.clear();

    for( AuraSet::iterator it = m_aurasInfluencingMe.begin(); it != m_aurasInfluencingMe.end(); it++ )
    {
        (*it)->Remove( this );
    }
    m_aurasInfluencingMe.clear();

    // Change this entity's template to the corpse entity - but note
    // we don't fiddle with the actors or bounding information that we
    // usually do when changing templates.

    if(m_corpse == L"null")
    {
        kill();
    }

    CBaseEntity* corpse = g_EntityTemplateCollection.getTemplate( m_corpse );
    if( corpse )
    {
        m_base = corpse;
        SetBase( m_base );
    }

    if( m_bounds )
    {
        delete( m_bounds );
        m_bounds = NULL;
    }

    if( m_extant )
    {
        m_extant = false;
    }

    updateCollisionPatch();

    g_Selection.removeAll( me );

    clearOrders();

	g_EntityManager.SetDeath(true);	
	

    if( m_actor )
	{
		m_actor->SetEntitySelection( L"death" );
        m_actor->SetRandomAnimation( "death", true );
	}

    return( true );
}

jsval CEntity::GetSpawnPoint( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
    float spawn_clearance = 2.0f;
    if( argc >= 1 )
    {
        CBaseEntity* be = ToNative<CBaseEntity>( argv[0] );
        if( be )
        {
            switch( be->m_bound_type )
            {
                case CBoundingObject::BOUND_CIRCLE:
                spawn_clearance = be->m_bound_circle->m_radius;
                break;
                case CBoundingObject::BOUND_OABB:
                spawn_clearance = be->m_bound_box->m_radius;
                break;
                default:
                debug_warn("No bounding information for spawned object!" );
            }
        }
        else
            spawn_clearance = ToPrimitive<float>( argv[0] );
    }
    else
        debug_warn("No arguments to Entity::GetSpawnPoint()" );

    // TODO: Make netsafe.
    CBoundingCircle spawn( 0.0f, 0.0f, spawn_clearance, 0.0f );

    if( m_bounds->m_type == CBoundingObject::BOUND_OABB )
    {
        CBoundingBox* oabb = (CBoundingBox*)m_bounds;

        // Pick a start point

        int edge = rand() & 3;
        int point;

        double max_w = oabb->m_w + spawn_clearance + 1.0;
        double max_d = oabb->m_d + spawn_clearance + 1.0;
        int w_count = (int)( max_w * 2 );
        int d_count = (int)( max_d * 2 );

        CVector2D w_step = oabb->m_v * (float)( max_w / w_count );
        CVector2D d_step = oabb->m_u * (float)( max_d / d_count );
        CVector2D pos( m_position );
        if( edge & 1 )
        {
            point = rand() % ( 2 * d_count ) - d_count;
            pos += ( oabb->m_v * (float)max_w + d_step * (float)point ) * ( ( edge & 2 ) ? -1.0f : 1.0f );
        }
        else
        {
            point = rand() % ( 2 * w_count ) - w_count;
            pos += ( oabb->m_u * (float)max_d + w_step * (float)point ) * ( ( edge & 2 ) ? -1.0f : 1.0f );
        }

        int start_edge = edge;
        int start_point = point;

        spawn.m_pos = pos;

        // Then step around the edge (clockwise) until a free space is found, or
        // we've gone all the way around.
        while( getCollisionObject( &spawn ) )
        {
            switch( edge )
            {
                case 0:
                point++;
                pos += w_step;
                if( point >= w_count )
                {
                    edge = 1;
                    point = -d_count;
                }
                break;
                case 1:
                point++;
                pos -= d_step;
                if( point >= d_count )
                {
                    edge = 2;
                    point = w_count;
                }
                break;
                case 2:
                point--;
                pos -= w_step;
                if( point <= -w_count )
                {
                    edge = 3;
                    point = d_count;
                }
                break;
                case 3:
                point--;
                pos += d_step;
                if( point <= -d_count )
                {
                    edge = 0;
                    point = -w_count;
                }
                break;
            }
            if( ( point == start_point ) && ( edge == start_edge ) )
                return( JSVAL_NULL );
            spawn.m_pos = pos;
        }
        CVector3D rval( pos.x, getAnchorLevel( pos.x, pos.y ), pos.y );
        return( ToJSVal( rval ) );
    }
    else if( m_bounds->m_type == CBoundingObject::BOUND_CIRCLE )
    {
        float ang;
        ang = (float)( rand() & 0x7fff ) / (float)0x4000; /* 0...2 */
        ang *= PI;
        float radius = m_bounds->m_radius + 1.0f + spawn_clearance;
        float d_ang = spawn_clearance / ( 2.0f * radius );
        float ang_end = ang + 2.0f * PI;
        float x = 0.0f, y = 0.0f; // make sure they're initialized
        for( ; ang < ang_end; ang += d_ang )
        {
            x = m_position.X + radius * cos( ang );
            y = m_position.Z + radius * sin( ang );
            spawn.setPosition( x, y );
            if( !getCollisionObject( &spawn ) )
                break;
        }
        if( ang < ang_end )
        {
            // Found a satisfactory position...
            CVector3D pos( x, 0, y );
            pos.Y = getAnchorLevel( x, y );
            return( ToJSVal( pos ) );
        }
        else
            return( JSVAL_NULL );
    }
    return( JSVAL_NULL );
}

jsval CEntity::AddAura( JSContext* cx, uintN argc, jsval* argv )
{
    debug_assert( argc >= 3 );
    debug_assert( JSVAL_IS_OBJECT(argv[2]) );

    CStrW name = ToPrimitive<CStrW>( argv[0] );
    float radius = ToPrimitive<float>( argv[1] );
    JSObject* handler = JSVAL_TO_OBJECT( argv[2] );

    if( m_auras[name] )
    {
        delete m_auras[name];
    }
    m_auras[name] = new CAura( cx, this, name, radius, handler );

    return JSVAL_VOID;
}

jsval CEntity::RemoveAura( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
    debug_assert( argc >= 1 );
    CStrW name = ToPrimitive<CStrW>( argv[0] );
    if( m_auras[name] )
    {
        delete m_auras[name];
        m_auras.erase(name);
    }
    return JSVAL_VOID;
}

jsval CEntity::SetActionParams( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
    debug_assert( argc == 5 );

    int id = ToPrimitive<int>( argv[0] );
    float minRange = ToPrimitive<int>( argv[1] );
    float maxRange = ToPrimitive<int>( argv[2] );
    uint speed = ToPrimitive<uint>( argv[3] );
    CStr8 animation = ToPrimitive<CStr8>( argv[4] );

	m_actions[id] = SEntityAction( minRange, maxRange, speed, animation );

    return JSVAL_VOID;
}

bool CEntity::RequestNotification( JSContext* cx, uintN argc, jsval* argv )
{
	if( argc < 4 )
	{
		JS_ReportError( cx, "Too few parameters" );
		return( false );
	}
	
	CEntityListener notify;
	CEntity *target = ToNative<CEntity>( argv[0] );
	(int&)notify.m_type = ToPrimitive<int>( argv[1] );
	bool tmpDestroyNotifiers = ToPrimitive<bool>( argv[2] );
	// TODO: ??? This local variable overrides the member variable of the same name...
	bool m_destroyNotifiers = !ToPrimitive<bool>( argv[3] );

	if (target == this)
		return false;
	
	notify.m_sender = this;

	//Clean up old requests
	if ( tmpDestroyNotifiers )
		DestroyAllNotifiers();
	//If new request is not the same and we're destroy notifiers, reset
	else if ( !(notify.m_type & m_currentRequest) && m_destroyNotifiers )
		DestroyAllNotifiers();

	m_currentRequest = notify.m_type;
	m_notifiers.push_back( target );
	int result = target->m_currentNotification & notify.m_type;
	
	//If our target isn't stationary and it's doing something we want to follow, send notification
	if ( result && !target->m_orderQueue.empty() )
	{
		CEntityOrder order = target->m_orderQueue.front();
		switch( result )
		{
			 case CEntityListener::NOTIFY_GOTO:
			 case CEntityListener::NOTIFY_RUN:
				DispatchNotification( order, result );
				break;
				 
			 case CEntityListener::NOTIFY_HEAL:
			 case CEntityListener::NOTIFY_ATTACK:
			 case CEntityListener::NOTIFY_GATHER:
		     case CEntityListener::NOTIFY_DAMAGE:
				 DispatchNotification( order, result );
				 break;
			 default:
				 JS_ReportError( cx, "Invalid order type" );
				 break;
		}
		target->m_listeners.push_back( notify );
		return true;
	}
		
	target->m_listeners.push_back( notify );
	return false;
}
bool CEntity::ForceCheckListeners( JSContext *cx, uintN argc, jsval* argv )
{	
	if( argc < 2 )
	{
		JS_ReportError( cx, "Too few parameters" );
		return false;
	}
	int type = ToPrimitive<int>( argv[0] );	   //notify code
	m_currentNotification = type;
	
	CEntity *target = ToNative<CEntity>( argv[1] );
	if ( target->m_orderQueue.empty() )
		return false;

	CEntityOrder order = target->m_orderQueue.front();
	for (size_t i=0; i<m_listeners.size(); i++)
	{
		int result = m_listeners[i].m_type & type;
		if ( result )
		{
			 switch( result )
			 {
				 case CEntityListener::NOTIFY_GOTO:
				 case CEntityListener::NOTIFY_RUN: 
				 case CEntityListener::NOTIFY_HEAL:
				 case CEntityListener::NOTIFY_ATTACK:
				 case CEntityListener::NOTIFY_GATHER:
				 case CEntityListener::NOTIFY_DAMAGE:
				 case CEntityListener::NOTIFY_IDLE:	//target should be 'this'
					m_listeners[i].m_sender->DispatchNotification( order, result );
					break;
				
				default:
					JS_ReportError( cx, "Invalid order type" );
					break;
			 }
		 }
	}
	return true;
}
void CEntity::CheckListeners( int type, CEntity *target)
{	
	m_currentNotification = type;
	
	debug_assert(target);
	if ( target->m_orderQueue.empty() )
		return;
	
	CEntityOrder order = target->m_orderQueue.front();
	for (size_t i=0; i<m_listeners.size(); i++)
	{
		int result = m_listeners[i].m_type & type;
		if ( result )
		{
			 switch( result )
			 {
				 case CEntityListener::NOTIFY_GOTO:
				 case CEntityListener::NOTIFY_RUN: 
				 case CEntityListener::NOTIFY_HEAL:
				 case CEntityListener::NOTIFY_ATTACK:
				 case CEntityListener::NOTIFY_GATHER:
				 case CEntityListener::NOTIFY_DAMAGE:
					 m_listeners[i].m_sender->DispatchNotification( order, result );
					 break;
				 default:
					debug_warn("Invalid notification: CheckListeners()");
					continue;
			 }
		}
	}
}
jsval CEntity::DestroyAllNotifiers( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	DestroyAllNotifiers();
	return JS_TRUE;
}
jsval CEntity::DestroyNotifier( JSContext* cx, uintN argc, jsval* argv )
{
	if ( argc < 1 )
	{
		JS_ReportError(cx, "too few parameters: CEntity::DestroyNotifier");
		return JS_FALSE;
	}
	DestroyNotifier( ToNative<CEntity>( argv[0] ) );
	return JS_TRUE;
}

jsval CEntity::TriggerRun( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	m_triggerRun = true;
	return JSVAL_VOID;
}

jsval CEntity::SetRun( JSContext* cx, uintN argc, jsval* argv )
{
	if( argc < 1 )
	{
		JS_ReportError( cx, "Too few parameters" );
		return( false );
	}
	m_shouldRun = ToPrimitive<bool> ( argv[0] );
	m_isRunning = m_shouldRun;
	return JSVAL_VOID;
}
jsval CEntity::GetRunState( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return m_isRunning;
}
jsval CEntity::GetFormationPenalty( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenalty() );
}
jsval CEntity::GetFormationPenaltyType( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenaltyType() );
}
jsval CEntity::GetFormationPenaltyVal( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetPenaltyVal() );
}
jsval CEntity::GetFormationBonus( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonus() );
}
jsval CEntity::GetFormationBonusType( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonusType() );
}
jsval CEntity::GetFormationBonusVal( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return ToJSVal( GetFormation()->GetBase()->GetBonusVal() );
}

jsval CEntity::RegisterDamage( JSContext* cx, uintN argc, jsval* argv )
{
	if ( argc < 1 )
	{
		JS_ReportError( cx, "Too few parameters" );
		return( false );
	}
	CEntity* inflictor = ToNative<CEntity>( argv[0] );
	CVector2D up(1.0f, 0.0f);
	CVector2D pos = CVector2D( inflictor->m_position.X, inflictor->m_position.Z );
	CVector2D posDelta = (pos - m_position).normalize(); 

	float angle = up.dot(posDelta);
	//Find what section it is between and "activate" it
	for ( int i=0; i<m_sectorDivs; ++i )
	{
		//Wrap around to the start-if we've made it this far, it's here
		if ( i == m_base->m_sectorDivs )
			m_sectorValues[i] = true;
		else if ( angle > m_sectorAngles[i] && angle < m_sectorAngles[i+1] )
			m_sectorValues[i] = true;
	}
	return JS_TRUE;
}
jsval CEntity::RegisterIdle( JSContext* cx, uintN argc, jsval* argv )
{
	if ( argc < 1 )
	{
		JS_ReportError( cx, "Too few parameters" );
		return( false );
	}
	CEntity* idleEntity = ToNative<CEntity>( argv[0] );

	CVector2D up(1.0f, 0.0f);
	CVector2D pos = CVector2D( idleEntity->m_position.X, idleEntity->m_position.Z );
	CVector2D posDelta = (pos - m_position).normalize(); 

	float angle = up.dot(posDelta);
	//Find what section it is between and "activate" it
	for ( int i=0; i<m_sectorDivs; ++i )
	{
		//Wrap around to the start-if we've made it this far, it's here
		if ( i == m_base->m_sectorDivs )
			m_sectorValues[i] = false;
		else if ( angle > m_sectorAngles[i] && angle < m_sectorAngles[i+1] )
			m_sectorValues[i] = false;
	}
	return JS_TRUE;
}
jsval CEntity::GetAttackDirections( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	int directions=0;

	for ( std::vector<bool>::iterator it=m_sectorValues.begin(); it != m_sectorValues.end(); it++ )
	{
		if ( *it )
			++directions;
	}
	return ToJSVal( directions );
}
