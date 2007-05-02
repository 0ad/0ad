#include "precompiled.h"

#include "ps/Profile.h"

#include "EntityOrders.h"
#include "Entity.h"
#include "EntityTemplate.h"
#include "PathfindEngine.h"
#include "graphics/Terrain.h"
#include "ps/World.h"


CPathfindEngine::CPathfindEngine()
{
}

void CPathfindEngine::RequestPath( HEntity entity, const CVector2D& destination,
								  CEntityOrder::EOrderSource orderSource )
{
	/* TODO: Add code to generate high level path
	         For now, just the one high level waypoint to the final 
			  destination is added
	*/
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT;
	waypoint.m_source = orderSource;
	waypoint.m_target_location = destination;
	waypoint.m_pathfinder_radius = 0.0f;
	entity->m_orderQueue.push_front( waypoint );
}

void CPathfindEngine::RequestLowLevelPath( HEntity entity, const CVector2D& destination, bool UNUSED(contact),
										  float radius, CEntityOrder::EOrderSource orderSource )
{
	PROFILE_START("Pathfinding");
	
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	if ( mLowPathfinder.FindPath(source, destination, entity, radius) )
	{
		std::vector<CVector2D> path = mLowPathfinder.GetLastPath();
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
			node.m_target_location = finalDest;
			entity->m_orderQueue.push_front(node);
			node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// push final goto step
			node.m_target_location = finalDest;
			entity->m_orderQueue.push_front(node);

			for( int i = ((int) path.size()) - 2; i >= 0; i-- )
			{
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;	// TODO: For non-contact paths, do we want some other order type?
				node.m_target_location = path[i];
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
				node.m_target_location = destination;
				entity->m_orderQueue.push_front(node);
				node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
				node.m_target_location = destination;
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

void CPathfindEngine::RequestContactPath( HEntity entity, CEntityOrder* current, float range )
{
	/* TODO: Same as non-contact: need high-level planner */
	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_source = current->m_source;
	HEntity target = current->m_target_entity;
	waypoint.m_target_location = target->m_position;
	waypoint.m_pathfinder_radius = std::max( target->m_bounds->m_radius, range );
	entity->m_orderQueue.push_front( waypoint );

	//PathSparse( entity, current->m_target_entity->m_position );
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

bool CPathfindEngine::RequestAvoidPath( HEntity entity, CEntityOrder* current, float avoidRange )
{
	/* TODO: Same as non-contact: need high-level planner */

	// TODO: Replace this with a new type of goal which is to avoid some point or line segment
	// (requires changes to pathfinder to support this type of goal)

	CEntityOrder waypoint;
	waypoint.m_type = CEntityOrder::ORDER_GOTO_WAYPOINT_CONTACT;
	waypoint.m_source = current->m_source;

	// Figure out a direction to move
	HEntity target = current->m_target_entity;
	CVector3D dir = entity->m_position - target->m_position;
	if(dir.LengthSquared() == 0) // shouldn't happen, but just in case
		dir = CVector3D(1, 0, 0);
	float dist = dir.Length();
	dir.Normalize();

	waypoint.m_target_location = entity->m_position + dir * (avoidRange - dist);

	if( !g_Game->GetWorld()->GetTerrain()->IsOnMap( waypoint.m_target_location ) )
	{
		return false;
	}

	waypoint.m_pathfinder_radius = 0.0f;
	entity->m_orderQueue.push_front( waypoint );
	return true;
}
