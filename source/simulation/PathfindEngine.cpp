#include "precompiled.h"

#include "Profile.h"

#include "EntityOrders.h"
#include "Entity.h"
#include "PathfindEngine.h"


CPathfindEngine::CPathfindEngine()
{
}

void CPathfindEngine::requestPath( HEntity entity, const CVector2D& destination )
{
	/* TODO: Add code to generate high level path
	         For now, just the one high level waypoint to the final 
			  destination is added
	*/
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT;
	waypoint.m_data[0].location = destination;
	entity->m_orderQueue.push_front( waypoint );
}

void CPathfindEngine::requestLowLevelPath( HEntity entity, const CVector2D& destination, bool contact )
{
	PROFILE_START("Pathfinding");
	
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	if ( mLowPathfinder.findPath(source, destination, entity->m_player) )
	{
		std::vector<CVector2D> path = mLowPathfinder.getLastPath();
		std::vector<CVector2D>::iterator it;
		CEntityOrder node;
		for( it = path.begin(); (it+1) != path.end(); it++ )
		{
			if ( !contact )
			{
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
			}
			else
			{
				// TODO: Is this right?
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
			}
			node.m_data[0].location = *it;
			entity->m_orderQueue.push_back(node);
		}
		node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
		node.m_data[0].location = *it;
		entity->m_orderQueue.push_back(node);
	}
	else
	{
		// If no path was found, then unsolvable
		// TODO: Figure out what to do in this case
	}
	
	PROFILE_END("Pathfinding");
}

void CPathfindEngine::requestContactPath( HEntity entity, CEntityOrder* current )
{
	/* TODO: Same as non-contact: need high-level planner */
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_data[0].location = current->m_data[0].entity->m_position;
	entity->m_orderQueue.push_front( waypoint );

	//pathSparse( entity, current->m_data[0].entity->m_position );
	//// For attack orders, do some additional postprocessing (replace goto/nopathing 
	//// with attack/nopathing, up until the attack order marker)
	//std::deque<CEntityOrder>::iterator it;
	//for( it = entity->m_orderQueue.begin(); it != entity->m_orderQueue.end(); it++ )
	//{
	//	if( it->m_type == CEntityOrder::ORDER_PATH_END_MARKER )
	//		break;
	//	if( it->m_type == CEntityOrder::ORDER_GOTO_NOPATHING )
	//	{
	//		*it = *current;
	//	}
	//}
}
