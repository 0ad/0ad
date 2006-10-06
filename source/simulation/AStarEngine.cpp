#include "precompiled.h"

#include "AStarEngine.h"

/* For AStarGoalLowLevel isPassable/cost */
#include "Collision.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"

#include "ps/Profile.h"

#define DEFAULT_SEARCH_LIMIT 1000
#define DEFAULT_INIT_NODES 1000
// TODO: do this for real
#define MAXSLOPE 3500


// Node status flags
enum
{
	kClear		= 0x00,	// empty, unexamined
	kPassable	= 0x01, // examined, not blocked
	kBlocked	= 0x02, // examined, blocked
	kOpen		= 0x04, // on the open list
	kClosed		= 0x08  // on the closed list
};


CAStarEngine::CAStarEngine()
{
	mSearchLimit = DEFAULT_SEARCH_LIMIT;
	for(int i=0; i<DEFAULT_INIT_NODES; i++)
	{
		freeNodes.push_back(new AStarNode);
	}
	mFlagArraySize = 300;
	mFlags = new AStarNodeFlag[mFlagArraySize*mFlagArraySize];
	memset(mFlags, 0, mFlagArraySize*mFlagArraySize*sizeof(AStarNodeFlag));
}

CAStarEngine::CAStarEngine(AStarGoalBase *goal)
{
	CAStarEngine();
	mGoal = goal;
}


CAStarEngine::~CAStarEngine()
{
	delete[] mFlags;
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


bool CAStarEngine::findPath( 
	const CVector2D &src, const CVector2D &dest, CPlayer* player, float radius )
{
	mSolved = false;
	int iterations = 0;

	mGoal->setDestination(dest);
	mGoal->setRadius(radius);

	AStarNode *start = getFreeASNode();
	start->coord = mGoal->getTile(src);
	start->parent = NULL;
	start->g = 0;
	start->f = start->h = mGoal->distanceToGoal(start->coord);

	clearOpen();
	clearClosed();
	PROFILE_START("memset cache");
	memset(mFlags, 0, mFlagArraySize*mFlagArraySize*sizeof(AStarNodeFlag));
	PROFILE_END("memset cache");

	addToOpen(start);

	AStarNode *best = NULL;

	while( iterations<mSearchLimit && (best = removeBestOpenNode()) != NULL )
	{
		iterations++;
		PROFILE_START("addToClosed");
		addToClosed(best);
		PROFILE_END("addToClosed");
		if ( mGoal->atGoal(best->coord) )
		{
			/* Path solved */
			mSolved = true;
			break;
		}

		/* Get neighbors of the best node */
		std::vector<CVector2D> neighbors;
		PROFILE_START("get neighbors");
		neighbors = mGoal->getNeighbors(best->coord, player);
		PROFILE_END("get neighbors");

		/* Update the neighbors of the best node */
		std::vector<CVector2D>::iterator it;
		for( it = neighbors.begin(); it != neighbors.end(); it++ )
		{
			AStarNode* N = getFreeASNode();
			PROFILE_START("initialize neighbor");
			N->coord = *it;
			// Assign f,g,h to neighbor
			N->g = best->g + mGoal->getTileCost(best->coord, N->coord);
			N->h = mGoal->distanceToGoal(*it);
			N->f = N->g + N->h;
			N->parent = best;
			PROFILE_END("initialize neighbor");

			AStarNode* C;
			PROFILE_START("search closed");
			C = getFromClosed(N->coord);
			PROFILE_END("search closed");
			bool update = false;
			if( C!=NULL && (N->f < C->f) )
			{
				PROFILE_START("remove from closed");
				/* N is on Closed and N->f is better */
				removeFromClosed(C);
				update = true;
				PROFILE_END("remove from closed");
			}
			if (C==NULL || update)
			{
				PROFILE_START("add to open");
				/* N is not on Closed */
				addToOpen(N);
				PROFILE_END("add to open");
			}
		}
	}

	if (mSolved && best!=NULL)
	{
		//debug_printf("Number of nodes searched: %d\n", iterations);
		constructPath(best);
	}
	
	cleanup();

	return mSolved;
}


void CAStarEngine::constructPath( AStarNode* end )
{
	std::deque<CVector2D> path;
	mPath.clear();
	while( end!=NULL && (end->parent)!=NULL )
	{
		path.push_front(mGoal->getCoord(end->coord));
		end = end->parent;
	}
	mPath.insert(mPath.begin(), path.begin(), path.end());
}


std::vector<CVector2D> CAStarEngine::getLastPath()
{
	return mPath;
}


void CAStarEngine::setSearchLimit( int limit )
{
	mSearchLimit = limit;
}


void CAStarEngine::addToOpen( AStarNode* node )
{
	/* If not in open, should add, otherwise should promote */
	AStarNodeFlag *flag = GetFlag(node->coord);
	if (!GetIsOpen(flag))
	{
		mOpen.push(node);
	}
	else
	{
		mOpen.promote(node);
	}
	SetOpenFlag(flag);
}


AStarNode* CAStarEngine::removeBestOpenNode()
{
	if (mOpen.empty())
		return NULL;
	AStarNode* top;
	PROFILE_START("remove from open");
	top = mOpen.top();
	mOpen.pop();
	ClearOpenFlag(GetFlag(top->coord));
	PROFILE_END("remove from open");
	return top;
}


void CAStarEngine::addToClosed( AStarNode* node )
{
	mClosed[node->coord] = node;
	SetClosedFlag(GetFlag(node->coord));
}


void CAStarEngine::removeFromClosed( AStarNode* node )
{
	mClosed.erase(node->coord);
	ClearClosedFlag(GetFlag(node->coord));
}


AStarNode* CAStarEngine::getFromClosed( const CVector2D& loc )
{
	if (!GetIsClosed(GetFlag(loc)))
	{
		return NULL;
	}
	ASNodeHashMap::iterator it = mClosed.find(loc);
	return ( it != mClosed.end() ) ? (it->second) : (NULL);
}


void CAStarEngine::clearOpen()
{
	mOpen.clear();
}


void CAStarEngine::clearClosed()
{
	mClosed.clear();
}


AStarNode* CAStarEngine::getFreeASNode()
{
	AStarNode* ret;
	PROFILE_START("allocator");
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
	PROFILE_END("allocator");

	return ret;
}


void CAStarEngine::cleanup()
{
	std::vector<AStarNode*>::iterator it;
	for( it = usedNodes.begin(); it != usedNodes.end(); it++)
	{
		freeNodes.push_back(*it);
	}

	usedNodes.clear();
}

void PriQueue::promote( AStarNode *node )
{
	if (node == NULL)
	{
		return;
	}

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

void PriQueue::clear()
{
	c.clear();
}

CVector2D TilespaceToWorldspace( const CVector2D &ts )
{
	return CVector2D(ts.x*CELL_SIZE+CELL_SIZE/2, ts.y*CELL_SIZE+CELL_SIZE/2);
}

CVector2D WorldspaceToTilespace( const CVector2D &ws )
{
	return CVector2D(floor(ws.x/CELL_SIZE), floor(ws.y/CELL_SIZE));
}


void AStarGoalLowLevel::setDestination( const CVector2D &dest )
{
	coord = WorldspaceToTilespace(dest);
}


void AStarGoalLowLevel::setRadius( float r )
{
	radius = r;
}

float AStarGoalLowLevel::getRadius()
{
	return radius;
}

float AStarGoalLowLevel::distanceToGoal( const CVector2D &loc )
{
	return ((coord-loc).length());
}

bool AStarGoalLowLevel::atGoal( const CVector2D &loc )
{
	float dx = coord.x - loc.x;
	float dy = coord.y - loc.y;
	return dx*dx + dy*dy <= radius*radius;
}

float AStarGoalLowLevel::getTileCost( const CVector2D& loc1, const CVector2D& loc2 )
{
	return (loc2-loc1).length() - radius;
}

bool AStarGoalLowLevel::isPassable( const CVector2D &loc, CPlayer* player )
{
	CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();
	int size = pTerrain->GetTilesPerSide();

	if( loc.x<0 || loc.y<0 || loc.x>=size || loc.y>=size )
	{
		return false;
	}

	CVector2D wloc = TilespaceToWorldspace(loc);
	float slope = pTerrain->getSlope(wloc.x, wloc.y);
	if (  slope < MAXSLOPE )
	{
		// If no entity blocking, return true
		CBoundingBox bounds(wloc.x, wloc.y, 0, CELL_SIZE, CELL_SIZE, 3);
		if ( getCollisionObject(&bounds, player) == NULL )
		{
			return true;
		}
	}
	return false;
}

CVector2D AStarGoalLowLevel::getCoord( const CVector2D &loc )
{
	return TilespaceToWorldspace(loc);
}

CVector2D AStarGoalLowLevel::getTile( const CVector2D &loc )
{
	return WorldspaceToTilespace(loc);
}

std::vector<CVector2D> AStarGoalLowLevel::getNeighbors( const CVector2D &loc, CPlayer* player )
{
	std::vector<CVector2D> vec;

	for( int xdiff = -1; xdiff <= 1; xdiff++ )
	{
		for( int ydiff = -1; ydiff <= 1; ydiff++ )
		{
			if ( xdiff!=0 || ydiff!=0 )
			{
				CVector2D c = loc;
				c.x += xdiff;  c.y += ydiff;
				if ( isPassable(c, player) )
				{
					vec.push_back(c);
				}
			}
		}
	}

	return vec;
}



inline AStarNodeFlag* CAStarEngine::GetFlag(const CVector2D &loc)
{
	debug_assert(loc.x>=0 && loc.y>=0 && loc.x<mFlagArraySize && loc.y<mFlagArraySize);
	return mFlags + (mFlagArraySize * (long)loc.x + (long)loc.y);
}


inline bool CAStarEngine::GetIsClear(AStarNodeFlag* flag)
{
	return (*flag)==kClear;
}


inline bool CAStarEngine::GetIsClosed(AStarNodeFlag* flag)
{
	return ((*flag) & kClosed) != kClear;
}


inline bool CAStarEngine::GetIsOpen(AStarNodeFlag* flag)
{
	return ((*flag) & kOpen) != kClear;
}


inline void CAStarEngine::SetClosedFlag(AStarNodeFlag* flag)
{
	*flag |= kClosed;
}


inline void CAStarEngine::SetOpenFlag(AStarNodeFlag* flag)
{
	*flag |= kOpen;
}


inline void CAStarEngine::ClearClosedFlag(AStarNodeFlag* flag)
{
	*flag &= -kClosed;
}


inline void CAStarEngine::ClearOpenFlag(AStarNodeFlag* flag)
{
	*flag &= -kOpen;
}


inline bool CAStarEngine::GetIsPassable(AStarNodeFlag* flag)
{
	return ((*flag) & kPassable) != kClear;
}


inline bool CAStarEngine::GetIsBlocked(AStarNodeFlag* flag)
{
	return ((*flag) & kBlocked) != kClear;
}


inline void CAStarEngine::SetPassableFlag(AStarNodeFlag* flag)
{
	*flag |= kPassable;
}


inline void CAStarEngine::SetBlockedFlag(AStarNodeFlag* flag)
{
	*flag |= kBlocked;
}
