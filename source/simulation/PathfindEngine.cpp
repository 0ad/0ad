#include "precompiled.h"

#include "PathfindEngine.h"
#include "PathfindSparse.h"
#include "ConfigDB.h"

CPathfindEngine::CPathfindEngine()
{
	CConfigValue* sparseDepth = g_ConfigDB.GetValue( CFG_USER, "pathfind.sparse.recursiondepth" );
	if( sparseDepth )
		sparseDepth->GetInt( SPF_RECURSION_DEPTH );
}

void CPathfindEngine::requestPath( HEntity entity, const CVector2D& destination )
{
	pathSparse( entity, destination );
}

void CPathfindEngine::requestMeleeAttackPath( HEntity entity, HEntity target )
{
	pathSparse( entity, target->m_position );
	// For attack orders, do some additional postprocessing (replace goto/nopathing 
	// with attack/nopathing, up until the attack order marker)
	std::deque<CEntityOrder>::iterator it;
	for( it = entity->m_orderQueue.begin(); it != entity->m_orderQueue.end(); it++ )
	{
		if( it->m_type == CEntityOrder::ORDER_PATH_END_MARKER )
			break;
		if( it->m_type == CEntityOrder::ORDER_GOTO_NOPATHING )
		{
			it->m_type = CEntityOrder::ORDER_ATTACK_MELEE_NOPATHING;
			it->m_data[0].entity = target;
		}
	}
}