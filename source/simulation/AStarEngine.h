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

#ifndef INCLUDED_ASTARENGINE
#define INCLUDED_ASTARENGINE

#include "ps/Vector2D.h"
#include "ps/Player.h"
#include <queue>

#include "renderer/TerrainOverlay.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "EntityManager.h"

#ifdef USE_DCDT
# include "dcdt/se/se_dcdt.h"
#endif // USE_DCDT

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
class AStarGoalBase;
class AStarGoalLowLevel;
struct AStarNodeComp
{
  bool operator()(const AStarNode* n1, const AStarNode* n2) const
  {
    return (*n2) < (*n1);
  }
};

class CVector2D_hash_compare
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 16;
	size_t operator() (const CVector2D& Key) const
	{
		return (size_t)(Key.x + Key.y*1024.f);
	}
	bool operator() (const CVector2D& _Key1, const CVector2D& _Key2) const
	{
		return (_Key1.x < _Key2.x) || (_Key1.x==_Key2.x && _Key1.y < _Key2.y);
	}
};
typedef STL_HASH_MAP<CVector2D, AStarNode*, CVector2D_hash_compare> ASNodeHashMap;

class PriQueue 
	: public std::priority_queue<AStarNode*, std::vector<AStarNode*>, AStarNodeComp>
{
public:
	// Promote a node in the PQ, or if it doesn't exist, add it
	void promote(AStarNode* node);
	void clear();
};

typedef unsigned char AStarNodeFlag;

class PathFindingTerrainOverlay;

class CAStarEngine
{
public:
	CAStarEngine();
	virtual ~CAStarEngine();

	void SetGoal(AStarGoalBase* goal) { mGoal = goal; }

	bool FindPath( const CVector2D& src, const CVector2D& dest, HEntity entity, float radius=0.0f );
	std::vector<CVector2D> GetLastPath();

	// The maximum number of nodes that will be expanded before failure is declared
	void SetSearchLimit( int limit );

	//Kai:added tile overlay for pathfinding
	PathFindingTerrainOverlay* pathfindingOverlay;
	
#ifdef USE_DCDT
	SrPolygon pol;
	//void TAStarTest();
#endif // USE_DCDT

	

protected:
	AStarGoalBase* mGoal;
private:
	int mSearchLimit;
	bool mSolved;
	std::vector<CVector2D> mPath;
	AStarNodeFlag *mFlags;
	long mFlagArraySize;

	ASNodeHashMap mClosed;
	PriQueue mOpen;

	std::vector<AStarNode*> freeNodes;
	std::vector<AStarNode*> usedNodes;

	// AddToOpen will promote if the node already is in Open
	void AddToOpen( AStarNode* );
	AStarNode* RemoveBestOpenNode();

	void AddToClosed( AStarNode* );
	void RemoveFromClosed( AStarNode* );
	AStarNode* GetFromClosed( const CVector2D& );

	void ClearOpen();
	void ClearClosed();

	void ConstructPath( AStarNode* );

	AStarNode* GetFreeASNode();
	void Cleanup();

	inline AStarNodeFlag* GetFlag(const CVector2D&);
	inline bool IsClear(AStarNodeFlag*);
	inline bool IsClosed(AStarNodeFlag*);
	inline bool IsOpen(AStarNodeFlag*);
	inline void SetClosedFlag(AStarNodeFlag*);
	inline void ClearClosedFlag(AStarNodeFlag*);
	inline void SetOpenFlag(AStarNodeFlag*);
	inline void ClearOpenFlag(AStarNodeFlag*);
	inline bool IsPassable(AStarNodeFlag*);
	inline bool IsBlocked(AStarNodeFlag*);
	inline void SetPassableFlag(AStarNodeFlag*);
	inline void SetBlockedFlag(AStarNodeFlag*);

};

/**
 * An A* goal consists of a destination tile and a radius within which a
 * unit must get to the destination. The radius is necessary because for
 * actions on a target unit, like attacking or building, the destination
 * tile is obstructed by that unit and what we really want is not to get
 * to that tile but to get close enough to perform our action.
 **/
class AStarGoalBase
{
public:
	AStarGoalBase() {}
	virtual void SetDestination( const CVector2D& ) = 0;
	virtual void SetRadius( float r ) = 0;
	virtual float DistanceToGoal( const CVector2D& ) = 0;
	virtual bool IsAtGoal( const CVector2D& ) = 0;
	virtual float GetTileCost( const CVector2D&, const CVector2D& ) = 0;
	virtual bool IsPassable( const CVector2D&, HEntity entity) = 0;
	virtual std::vector<CVector2D> GetNeighbors( const CVector2D&, HEntity entity) = 0;
	virtual CVector2D GetCoord( const CVector2D& ) = 0;
	virtual CVector2D GetTile( const CVector2D& ) = 0;
	virtual float GetRadius() = 0;
};

class AStarGoalLowLevel : public AStarGoalBase
{
public:
	AStarGoalLowLevel(): radius(0.0f) {}
	void SetDestination( const CVector2D& dest );
	void SetRadius( float r );
	float DistanceToGoal( const CVector2D& loc );
	bool IsAtGoal( const CVector2D& loc );
	float GetTileCost( const CVector2D& loc1, const CVector2D& loc2 );
	bool IsPassable( const CVector2D& loc, HEntity entity);
	std::vector<CVector2D> GetNeighbors( const CVector2D& loc, HEntity entity);
	CVector2D GetCoord( const CVector2D& loc);
	CVector2D GetTile( const CVector2D& loc);
	float GetRadius();
private:
	CVector2D coord;
	float radius;
};

class CAStarEngineLowLevel : public CAStarEngine
{
public:
	CAStarEngineLowLevel()
	{
		mGoal = new AStarGoalLowLevel;
	}
	virtual ~CAStarEngineLowLevel()
	{
		delete mGoal;
	}
};


#endif
