#include "precompiled.h"

#include "Stance.h"

#include "EntityManager.h"
#include "Entity.h"
#include "ps/Player.h"
#include "graphics/Terrain.h"

#include <algorithm>

// AggressStance ////////////////////////////////////////////////////

void CAggressStance::onIdle()
{
	CEntity* target = CStanceUtils::chooseTarget( m_Entity );
	if( target )
		CStanceUtils::attack( m_Entity, target );
}

void CAggressStance::onDamaged(CEntity *source)
{
	if( source && m_Entity->m_orderQueue.empty() 
		&& m_Entity->GetPlayer()->GetDiplomaticStance(source->GetPlayer()) != DIPLOMACY_ALLIED )
		CStanceUtils::attack( m_Entity, source );
}

// StandStance //////////////////////////////////////////////////////

void CStandStance::onIdle()
{
	CEntity* target = CStanceUtils::chooseTarget( m_Entity );
	if( target )
		CStanceUtils::attack( m_Entity, target );
}

void CStandStance::onDamaged(CEntity *source)
{
	if( source && m_Entity->m_orderQueue.empty() 
		&& m_Entity->GetPlayer()->GetDiplomaticStance(source->GetPlayer()) != DIPLOMACY_ALLIED )
		CStanceUtils::attack( m_Entity, source );
}

// DefendStance /////////////////////////////////////////////////////

void CDefendStance::onIdle()
{
	idlePos = CVector2D( m_Entity->m_position.X, m_Entity->m_position.Z );

	CEntity* target = CStanceUtils::chooseTarget( m_Entity );
	if( target )
		CStanceUtils::attack( m_Entity, target );
}

void CDefendStance::onDamaged(CEntity *source)
{
	if( source && m_Entity->m_orderQueue.empty()
		&& m_Entity->GetPlayer()->GetDiplomaticStance(source->GetPlayer()) != DIPLOMACY_ALLIED )
	{
		// Retaliate only if we can reach the enemy unit without walking farther than our LOS
		// radius away from idlePos.
		int action = m_Entity->GetAttackAction( source->me );
		if( action )
		{
			float range = m_Entity->m_actions[action].m_MaxRange;
			if( ( range + m_Entity->m_los * CELL_SIZE ) >= m_Entity->distance2D( source ) )
			{
				CEntityOrder order( CEntityOrder::ORDER_GENERIC, CEntityOrder::SOURCE_UNIT_AI );
				order.m_data[0].entity = source->me;
				order.m_data[1].data = action;
				m_Entity->pushOrder( order );
			}
		}
	}
}

bool CDefendStance::checkMovement( CVector2D proposedPos )
{
	float los = m_Entity->m_los*CELL_SIZE;

	// Check that we haven't moved too far from the place where we were stationed.
	if( (proposedPos - idlePos).length() > los )
	{
		// TODO: Make sure we don't clear any player orders here; the best way would be to make
		// shift-clicked player orders either unqueue any AI orders or convert those AI orders 
		// to player orders (since the player wants us to finish our attack then do other stuff).
		m_Entity->m_orderQueue.clear();

		// Try to find some other nearby enemy to attack, provided it's also in range of our idle spot
		// TODO: really we should be attack-moving to our spot somehow

		std::vector<CEntity*> results;
		g_EntityManager.GetInRange( m_Entity->m_position.X, m_Entity->m_position.Z, los, results );

		float bestDist = 1e20f;
		CEntity* bestTarget = 0;

		for( size_t i=0; i<results.size(); i++ )
		{
			CEntity* ent = results[i];
			float range = m_Entity->m_actions[m_Entity->GetAttackAction(ent->me)].m_MaxRange;
			if( m_Entity->GetPlayer()->GetDiplomaticStance( ent->GetPlayer() ) == DIPLOMACY_ENEMY )
			{
				float distToMe = ent->distance2D( m_Entity );
				float distToIdlePos = ent->distance2D( idlePos );
				if( distToIdlePos <= los+range && distToMe < bestDist )
				{
					bestDist = distToMe;
					bestTarget = ent;
				}
			}
		}

		if( bestTarget != 0 )
		{
			CStanceUtils::attack( m_Entity, bestTarget );
		}
		else
		{
			// Let's just walk back to our idle spot
			CEntityOrder order( CEntityOrder::ORDER_GOTO, CEntityOrder::SOURCE_UNIT_AI );
			order.m_data[0].location = idlePos;
			m_Entity->pushOrder( order );
		}
	
		return false;
	}
	else
	{
		return true;
	}
}

// StanceUtils //////////////////////////////////////////////////////

void CStanceUtils::attack(CEntity* entity, CEntity* target)
{
	int action = entity->GetAttackAction( target->me );
	if( action )
	{
		CEntityOrder order( CEntityOrder::ORDER_GENERIC, CEntityOrder::SOURCE_UNIT_AI );
		order.m_data[0].entity = target->me;
		order.m_data[1].data = action;
		entity->pushOrder( order );
	}
}

CEntity* CStanceUtils::chooseTarget( CEntity* entity )
{
	return chooseTarget( entity->m_position.X, entity->m_position.Z, entity->m_los*CELL_SIZE, entity->GetPlayer() );
}

CEntity* CStanceUtils::chooseTarget( float x, float z, float radius, CPlayer* myPlayer )
{
	std::vector<CEntity*> results;
	g_EntityManager.GetInRange( x, z, radius, results );

	float bestDist = 1e20f;
	CEntity* bestTarget = 0;

	for( size_t i=0; i<results.size(); i++ )
	{
		CEntity* ent = results[i];
		if( myPlayer->GetDiplomaticStance( ent->GetPlayer() ) == DIPLOMACY_ENEMY )
		{
			float dist = ent->distance2D( x, z );
			if( dist < bestDist )
			{
				bestDist = dist;
				bestTarget = ent;
			}
		}
	}

	return bestTarget;
}

