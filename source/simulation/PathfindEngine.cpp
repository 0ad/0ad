#include "precompiled.h"

#include "ps/Profile.h"

#include "EntityOrders.h"
#include "Entity.h"
#include "EntityTemplate.h"
#include "PathfindEngine.h"


CPathfindEngine::CPathfindEngine()
{
}

void CPathfindEngine::requestPath( HEntity entity, const CVector2D& destination,
								  CEntityOrder::EOrderSource orderSource )
{
	/* TODO: Add code to generate high level path
	         For now, just the one high level waypoint to the final 
			  destination is added
	*/
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT;
	waypoint.m_source = orderSource;
	waypoint.m_data[0].location = destination;
	*((float*)&waypoint.m_data[0].data) = 0.0f;
	entity->m_orderQueue.push_front( waypoint );
}

void CPathfindEngine::requestLowLevelPath( HEntity entity, const CVector2D& destination, bool UNUSED(contact),
										  float radius, CEntityOrder::EOrderSource orderSource )
{
	PROFILE_START("Pathfinding");
	
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	if ( mLowPathfinder.findPath(source, destination, entity->GetPlayer(), radius) )
	{
		std::vector<CVector2D> path = mLowPathfinder.getLastPath();
		if( path.size() > 0 )
		{
			// Push the path onto the front of our order queue in reverse order,
			// so that we run through it before continuing other orders.

			CEntityOrder node;
			node.m_source = orderSource;

			// Hack to make pathfinding slightly more precise:
			// If the radius was 0, make the final node be exactly at the destination
			// (otherwise, go to wherever the pathfinder tells us since we just want to be in range)
			CVector2D finalDest = (radius==0 ? destination : path[path.size()-1]);
			node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;	// push end marker (used as a sentinel when repathing)
			node.m_data[0].location = finalDest;
			entity->m_orderQueue.push_front(node);
			node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// push final goto step
			node.m_data[0].location = finalDest;
			entity->m_orderQueue.push_front(node);

			for( int i = ((int) path.size()) - 2; i >= 0; i-- )
			{
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// TODO: For non-contact paths, do we want some other order type?
				node.m_data[0].location = path[i];
				entity->m_orderQueue.push_front(node);
			}
		}
		else {
			// Hack to make pathfinding slightly more precise:
			// If radius = 0, we have an empty path but the user still wants us to move 
			// within the same tile, so add a GOTO order anyway
			if(radius == 0)
			{
				CEntityOrder node;
				node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
				node.m_data[0].location = destination;
				entity->m_orderQueue.push_front(node);
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
				node.m_data[0].location = destination;
				entity->m_orderQueue.push_front(node);
			}
		}
	}
	else
	{
		// If no path was found, then unsolvable
		// TODO: Figure out what to do in this case
	}
	
	PROFILE_END("Pathfinding");
}

void CPathfindEngine::requestContactPath( HEntity entity, CEntityOrder* current, float range )
{
	/* TODO: Same as non-contact: need high-level planner */
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_source = current->m_source;
	waypoint.m_data[0].location = current->m_data[0].entity->m_position;
	*((float*)&waypoint.m_data[0].data) = std::max( current->m_data[0].entity->m_bounds->m_radius, range );
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
