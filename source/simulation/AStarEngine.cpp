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

#include "precompiled.h"

#include "AStarEngine.h"

/* For AStarGoalLowLevel IsPassable/cost */
#include "Collision.h"
#include "Entity.h"

#include "ps/Overlay.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "graphics/TextureEntry.h"
#include "graphics/TerrainProperties.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"

#include "ps/Profile.h"

#include "lib/res/graphics/ogl_tex.h"

#include "ps/GameSetup/Config.h"



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


class PathFindingTerrainOverlay : public TerrainOverlay
{
public:
	char random[1021];
	std::vector<CVector2D> aPath;

	void setPath(const std::vector<CVector2D>& _aPath)
	{
		aPath =_aPath;

		for(size_t k = 0 ; k< aPath.size();k++)
		{
			aPath[k] = WorldspaceToTilespace( aPath[k] );
		}
	}

	CVector2D WorldspaceToTilespace( const CVector2D &ws )
	{
		return CVector2D(floor(ws.x/CELL_SIZE), floor(ws.y/CELL_SIZE));
	}

	bool inPath(ssize_t i, ssize_t j)
	{
		for(size_t k = 0 ; k<aPath.size();k++)
		{
			if(aPath[k].x== i && aPath[k].y== j)
				return true;
		}
		return false;
	}

	virtual void ProcessTile(ssize_t i, ssize_t j)
	{
		
		if ( inPath( i, j))
		{		
			RenderTile(CColor(random[(i*79+j*13) % ARRAY_SIZE(random)]/4.f, 1, 0, 0.3f), false);
			RenderTileOutline(CColor(1, 1, 1, 1), 1, true);
		}
	}
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
	
	// TODO: only instantiate this object when it's going to be used
	pathfindingOverlay = new PathFindingTerrainOverlay();
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

/*
void CAStarEngine::TAStarTest()
{
	//Kai: added for TA*
	std::vector<CEntity*> results;
	
	g_EntityManager.GetExtant(results);

	for(int i =0 ; i < results.size(); i++)
	{
		CEntity* tempHandle = results[i];

		

		debug_printf("Entity position: %f %f %f\n", tempHandle->m_position.X,tempHandle->m_position.Y,tempHandle->m_position.Z);

		CBoundingObject* m_bounds = tempHandle->m_bounds;

		switch( m_bounds->m_type )
		{
			case CBoundingObject::BOUND_CIRCLE:
			{
				break;
			}
			case CBoundingObject::BOUND_OABB:
			{
				
				
		glColor3f( 1, 1, 1 );		// Colour outline with player colour

				glBegin(GL_LINES);

					glVertex3f( 2, tempHandle->GetAnchorLevel( 2, 2 ) + 0.25f, 2 );    // lower left vertex
					
					glVertex3f( 5, tempHandle->GetAnchorLevel( 5, 5 ) + 0.25f, 5 );     // upper vertex

				glEnd();

				

				break;
			}
		}


		if(tempHandler->m_bound_type == CBoundingOjbect::BOUND_OABB)
		{
			debug_printf("Entity bound box: %f\n", tempHandler->m_bound_box.m_v);
		}
		

		
	}




}*/

bool CAStarEngine::FindPath( 
	const CVector2D &src, const CVector2D &dest, HEntity entity, float radius )
{
	mSolved = false;
	int iterations = 0;

	mGoal->SetDestination(dest);
	mGoal->SetRadius(radius);

	AStarNode *start = GetFreeASNode();
	start->coord = mGoal->GetTile(src);
	start->parent = NULL;
	start->g = 0;
	start->f = start->h = mGoal->DistanceToGoal(start->coord);

	ClearOpen();
	ClearClosed();
	PROFILE_START("memset cache");
	memset(mFlags, 0, mFlagArraySize*mFlagArraySize*sizeof(AStarNodeFlag));
	PROFILE_END("memset cache");

	AddToOpen(start);

	AStarNode *best = NULL;

	while( iterations<mSearchLimit && (best = RemoveBestOpenNode()) != NULL )
	{
		iterations++;
		PROFILE_START("AddToClosed");
		AddToClosed(best);
		PROFILE_END("AddToClosed");
		if ( mGoal->IsAtGoal(best->coord) )
		{
			/* Path solved */
			mSolved = true;
			break;
		}

		/* Get neighbors of the best node */
		std::vector<CVector2D> neighbors;
		PROFILE_START("get neighbors");
		neighbors = mGoal->GetNeighbors(best->coord, entity);
		PROFILE_END("get neighbors");

		/* Update the neighbors of the best node */
		std::vector<CVector2D>::iterator it;
		for( it = neighbors.begin(); it != neighbors.end(); it++ )
		{
			
			
			AStarNode* N = GetFreeASNode();
			PROFILE_START("initialize neighbor");
			N->coord = *it;
			// Assign f,g,h to neighbor
			N->g = best->g + mGoal->GetTileCost(best->coord, N->coord);
			N->h = mGoal->DistanceToGoal(*it);
			N->f = N->g +N->h;
			N->parent = best;
			PROFILE_END("initialize neighbor");

			AStarNode* C;
			PROFILE_START("search closed");
			C = GetFromClosed(N->coord);
			PROFILE_END("search closed");
			bool update = false;
			if( C!=NULL && (N->f < C->f) )
			{
				PROFILE_START("remove from closed");
				// N is on Closed and N->f is better 
				RemoveFromClosed(C);
				update = true;
				PROFILE_END("remove from closed");
			}
			if (C==NULL || update)
			{
				PROFILE_START("add to open");
				// N is not on Closed 
				AddToOpen(N);
				PROFILE_END("add to open");
			}
		}
	}

	if (mSolved && best!=NULL)
	{
		//debug_printf("Number of nodes searched: %d\n", iterations);
		ConstructPath(best);
	}

	//switch on/off grid path drawing by command line arg "-showOverlay"
	//it's guarded here to stop setting the drawing path in pathfindingOverlay.
	//(efficiency issue)
	//the drawing is disabled in the render() function in TerrainOverlay.cpp
	if(g_ShowPathfindingOverlay)
	{
		pathfindingOverlay->setPath(mPath);
	}

	Cleanup();

	return mSolved;
}


void CAStarEngine::ConstructPath( AStarNode* end )
{
	std::deque<CVector2D> path;
	mPath.clear();
	while( end!=NULL && (end->parent)!=NULL )
	{
		path.push_front(mGoal->GetCoord(end->coord));
		end = end->parent;
	}
	mPath.insert(mPath.begin(), path.begin(), path.end());
}


std::vector<CVector2D> CAStarEngine::GetLastPath()
{
	return mPath;
}


void CAStarEngine::SetSearchLimit( int limit )
{
	mSearchLimit = limit;
}


void CAStarEngine::AddToOpen( AStarNode* node )
{
	/* If not in open, should add, otherwise should promote */
	AStarNodeFlag *flag = GetFlag(node->coord);
	if (!IsOpen(flag))
	{
		mOpen.push(node);
	}
	else
	{
		mOpen.promote(node);
	}
	SetOpenFlag(flag);
}


AStarNode* CAStarEngine::RemoveBestOpenNode()
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


void CAStarEngine::AddToClosed( AStarNode* node )
{
	mClosed[node->coord] = node;
	SetClosedFlag(GetFlag(node->coord));
}


void CAStarEngine::RemoveFromClosed( AStarNode* node )
{
	mClosed.erase(node->coord);
	ClearClosedFlag(GetFlag(node->coord));
}


AStarNode* CAStarEngine::GetFromClosed( const CVector2D& loc )
{
	if (!IsClosed(GetFlag(loc)))
	{
		return NULL;
	}
	ASNodeHashMap::iterator it = mClosed.find(loc);
	return ( it != mClosed.end() ) ? (it->second) : (NULL);
}


void CAStarEngine::ClearOpen()
{
	mOpen.clear();
}


void CAStarEngine::ClearClosed()
{
	mClosed.clear();
}


AStarNode* CAStarEngine::GetFreeASNode()
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


void CAStarEngine::Cleanup()
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


void AStarGoalLowLevel::SetDestination( const CVector2D &dest )
{
	coord = WorldspaceToTilespace(dest);
}


void AStarGoalLowLevel::SetRadius( float r )
{
	radius = r;
}

float AStarGoalLowLevel::GetRadius()
{
	return radius;
}

float AStarGoalLowLevel::DistanceToGoal( const CVector2D &loc )
{
	return ((coord-loc).Length());
}

bool AStarGoalLowLevel::IsAtGoal( const CVector2D &loc )
{
	float dx = coord.x - loc.x;
	float dy = coord.y - loc.y;
	return dx*dx + dy*dy <= radius*radius / (CELL_SIZE*CELL_SIZE); // Scale down radius to tilespace
}

float AStarGoalLowLevel::GetTileCost( const CVector2D& loc1, const CVector2D& loc2 )
{
	return (loc2-loc1).Length() - radius;
}

bool AStarGoalLowLevel::IsPassable( const CVector2D &loc, HEntity entity )
{
	CTerrain* pTerrain = g_Game->GetWorld()->GetTerrain();
	int size = pTerrain->GetTilesPerSide();

	if( loc.x<0 || loc.y<0 || loc.x>=size || loc.y>=size )
	{
		return false;
	}

	if ( pTerrain->IsPassable(loc, entity) )
	{
		// If no entity blocking, return true
		CVector2D wloc = TilespaceToWorldspace(loc);
		CBoundingBox bounds(wloc.x, wloc.y, 0, CELL_SIZE, CELL_SIZE, 3);
		if ( GetCollisionObject(&bounds, entity->GetPlayer()) == NULL )
		{
			return true;
		}
	}
	return false;
}

CVector2D AStarGoalLowLevel::GetCoord( const CVector2D &loc )
{
	return TilespaceToWorldspace(loc);
}

CVector2D AStarGoalLowLevel::GetTile( const CVector2D &loc )
{
	return WorldspaceToTilespace(loc);
}

std::vector<CVector2D> AStarGoalLowLevel::GetNeighbors( const CVector2D &loc, HEntity entity)
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
				if ( IsPassable(c, entity) )
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


inline bool CAStarEngine::IsClear(AStarNodeFlag* flag)
{
	return (*flag)==kClear;
}


inline bool CAStarEngine::IsClosed(AStarNodeFlag* flag)
{
	return ((*flag) & kClosed) != kClear;
}


inline bool CAStarEngine::IsOpen(AStarNodeFlag* flag)
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


inline bool CAStarEngine::IsPassable(AStarNodeFlag* flag)
{
	return ((*flag) & kPassable) != kClear;
}


inline bool CAStarEngine::IsBlocked(AStarNodeFlag* flag)
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
