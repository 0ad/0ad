#include "precompiled.h"

#include <queue>

#include "PathfindEngine.h"
//#include "PathfindSparse.h"
#include "ConfigDB.h"
#include "Terrain.h"
#include "Collision.h"

#include "ps/Game.h"
#include "ps/World.h"

#define MAXSLOPE 3000
#define INITNODES 1000

void processPath(HEntity, AStarNode*, bool);

class AStarNode
{
public:
	float f, g, h;
	AStarNode* parent;
	CVector2D coord;
	bool operator <(const AStarNode& rhs) const { return f<rhs.f; }
	bool equals(const AStarNode& rhs) const 
	{ 
		return ( coord.x==rhs.coord.x ) && ( coord.y==rhs.coord.y ); 
	}
};

struct AStarNodeComp
{
  bool operator()(const AStarNode* n1, const AStarNode* n2) const
  {
    return (*n2) < (*n1);
  }
};

CVector2D TilespaceToWorldspace( const CVector2D &ts )
{
	return CVector2D(ts.x*CELL_SIZE+CELL_SIZE/2, ts.y*CELL_SIZE+CELL_SIZE/2);
}

CVector2D WorldspaceToTilespace( const CVector2D &ws )
{
	return CVector2D(floor(ws.x/CELL_SIZE), floor(ws.y/CELL_SIZE));
}

bool isPassable( const CVector2D &wc )
{
	CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();
	float slope = pTerrain->getSlope(wc.x, wc.y);
	if (  slope < MAXSLOPE )
	{
		// If no entity blocking, return true
		CBoundingBox bounds(wc.x, wc.y, 0, CELL_SIZE, CELL_SIZE, 3);
		if ( getCollisionObject(&bounds) == NULL )
		{
			return true;
		}
	}
	return false;
}

class PriQueue 
 : public std::priority_queue<AStarNode*, std::vector<AStarNode*>, AStarNodeComp>
{
public:
	// Promote a node in the PQ, or if it doesn't exist, add it
	void promote(AStarNode* node)
	{
		if (node == NULL)
			return;
		std::vector<AStarNode*>::iterator ind, first;
		for( ind = c.begin(); ind!=c.end() && !((*ind)->equals(*node)); ind++ );
		if (ind == c.end())
		{
			push(node);
			return;
		}
		if( (*ind)->f <= node->f ) return;

		first = c.begin();
		int index = ind-first;
		int parent = (index - 1)/2;

		while ( index>0 && (*(first+parent))->f > node->f )
		{
			*(first+index) = *(first+parent);
			index = parent;
			parent = (parent - 1)/2;
		}
		*(first+index) = node;
	}
};

bool CPathfindEngine::isVisited( const CVector2D& coord )
{
	ASNodeHashMap::iterator it = visited.find(coord);
	return ( it != visited.end() );
}

std::vector<AStarNode*> CPathfindEngine::getNeighbors( AStarNode* node )
{
	std::vector<AStarNode*> vec;

	for( int xdiff = -1; xdiff <= 1; xdiff++ )
	{
		for( int ydiff = -1; ydiff <= 1; ydiff++ )
		{
			if ( xdiff!=0 || ydiff!=0 )
			{
				CVector2D coord = node->coord;
				coord.x += xdiff;  coord.y += ydiff;
				if ( isVisited(coord) || isPassable(TilespaceToWorldspace(coord)) )
				{
					AStarNode* n = getFreeASNode();
					n->coord = coord;
					n->f = n->g = n->h = 0;
					n->parent = 0;
					vec.push_back(n);
				}
			}
		}
	}

	return vec;
}

CPathfindEngine::CPathfindEngine()
{
/*	CConfigValue* sparseDepth = g_ConfigDB.GetValue( CFG_USER, "pathfind.sparse.recursiondepth" );
	if( sparseDepth )
		sparseDepth->GetInt( SPF_RECURSION_DEPTH ); */
	for(int i=0; i<INITNODES; i++)
	{
		freeNodes.push_back(new AStarNode);
	}
}

CPathfindEngine::~CPathfindEngine()
{
	std::vector<AStarNode*>::iterator it;
	for( it = usedNodes.begin(); it != usedNodes.end(); it++)
	{
		delete (*it);
	}
	for( it = freeNodes.begin(); it != freeNodes.end(); it++)
	{
		delete (*it);
	}
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
	/* TODO: Pull out code into AStarEngine */
	/* TODO: Put a limit on the search space to prevent unreachable destinations from
	         eating the CPU */
	CVector2D source( entity->m_position.X, entity->m_position.Z );

	// If the goal is unreachable, move it towards the start until it is reachable
	CVector2D goalLoc = destination;
	CVector2D unitVec = (entity->m_position - goalLoc);
	unitVec= unitVec.normalize() * CELL_SIZE / 2;
	while( !isPassable(goalLoc) )
	{
		goalLoc += unitVec;
	}
	
	// Initialize priority queue (PQ) and visited list (V)
	visited.clear();
	PriQueue priQ;

	// Construct a dummy node for the goal
	AStarNode* goal = getFreeASNode();
	goal->coord = WorldspaceToTilespace(goalLoc);
	goal->parent = NULL;
	goal->f = goal->g = goal->h = 0;

	// Assign f,g,h to start location, add to PQ
	AStarNode* start = getFreeASNode();
	start->coord = WorldspaceToTilespace(source);
	start->g = 0;
	start->f = start->h = (goal->coord-start->coord).length();
	start->parent = NULL;
	priQ.push(start);
	visited[start->coord] = start;

	// Loop until PQ is empty
	while(!priQ.empty())
	{
	    // Select best cost node, B, from PQ
		AStarNode* best = priQ.top();
		priQ.pop();
		// If B is the goal, we are done, and found a path
		if ( best->equals( *goal ) )
		{
			goal->parent = best;
			goal->g = goal->f = best->g + 1;
			break;
		}
		
		std::vector<AStarNode*> neighbors = getNeighbors(best);
		// For each neighbor, C, of B
		std::vector<AStarNode*>::iterator it;
		for( it = neighbors.begin(); it != neighbors.end(); it++ )
		{
			AStarNode* C = *it;
			// Assign f,g,h to C
			C->g = best->g + 1;
			// Penalize for non-straight paths
			if ( best->parent )
			{
				int dx1 = C->coord.x - best->coord.x;
				int dy1 = C->coord.y - best->coord.y;
				int dx2 = best->coord.x - best->parent->coord.x;
				int dy2 = best->coord.y - best->parent->coord.y;
				if ( ((dx1 - dx2) + (dy1 - dy2)) != 0 )
				{
					C->g += 0.1f;
				}
			}

			C->h = ((goal->coord) - (C->coord)).length();
			C->f = C->g + C->h;
			C->parent = best;

			// If C not in V, add C to V and PQ
			// If the f of C is less than the f of C in the PQ, promote C in PQ and update V
			ASNodeHashMap::iterator it2 = visited.find(C->coord);
			if ( it2 != visited.end() && (C->f < it2->second->f) )
			{
				it2->second = C;
				priQ.promote(C);
			}
			else if ( it2 == visited.end() )
			{				
				visited[C->coord] = C;
				priQ.push(C);
			}
		}
	}

	if ( goal->parent )
	{
		processPath(entity, goal, contact);
	}
	else
	{
		// If no path was found, then unsolvable
		// TODO: Figure out what to do in this case
	}
	
	cleanup();
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

AStarNode* CPathfindEngine::getFreeASNode()
{
	AStarNode* ret;
	if (!freeNodes.empty())
	{
		ret = freeNodes.back();
		freeNodes.pop_back();
	}
	else
	{
		ret = new AStarNode;
	}
	usedNodes.push_back(ret);

	return ret;
}

void CPathfindEngine::cleanup()
{
	std::vector<AStarNode*>::iterator it;
	for( it = usedNodes.begin(); it != usedNodes.end(); it++)
	{
		freeNodes.push_back(*it);
	}

	usedNodes.clear();
}

void processPath(HEntity entity, AStarNode* goal, bool contact)
{
	AStarNode* current = goal;

	CEntityOrder node;
	node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
	entity->m_orderQueue.push_front( node );

	/* TODO: Smoothing for units with a turning radius */
	while( current != NULL && current->g != 0 )
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
		node.m_data[0].location = TilespaceToWorldspace(current->coord);
		entity->m_orderQueue.push_front( node );
		current = current->parent;
	}
}