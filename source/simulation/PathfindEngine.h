// PathfindEngine.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// The pathfinding engine singleton.
//
// Usage: g_Pathfinder.requestPath( HEntity me, float x, float y );
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef PATHFIND_ENGINE_INCLUDED
#define PATHFIND_ENGINE_INCLUDED

#include "Singleton.h"
#include "EntityHandles.h"
#include "Vector2D.h"

#define g_Pathfinder CPathfindEngine::GetSingleton()
#define MAXSIZE 1024

class CEntityOrder;

enum EPathType
{
	PF_STANDARD,
	PF_ATTACK_MELEE,
};

class AStarNode;
class CVector2D_hash_compare
{
public:
	static const size_t bucket_size = 4;
	static const size_t min_buckets = 16;
	size_t operator() (const CVector2D& Key) const
	{
		return Key.x + Key.y*MAXSIZE;
	}
	bool operator() (const CVector2D& _Key1, const CVector2D& _Key2) const
	{
		return (_Key1.x < _Key2.x) || (_Key1.x==_Key2.x && _Key1.y < _Key2.y);
	}
};
typedef STL_HASH_MAP<CVector2D, AStarNode*, CVector2D_hash_compare> ASNodeHashMap;

class CPathfindEngine : public Singleton<CPathfindEngine>
{
public:
	CPathfindEngine();
	~CPathfindEngine();
	void requestPath( HEntity entity, const CVector2D& destination );
	void requestLowLevelPath( HEntity entity, const CVector2D& destination, bool contact );
	void requestContactPath( HEntity entity, CEntityOrder* current );
private:
	std::vector<AStarNode*> getNeighbors( AStarNode* node );
	bool isVisited( const CVector2D& coord );
	AStarNode* getFreeASNode();
	void cleanup();

	std::vector<AStarNode*> freeNodes;
	std::vector<AStarNode*> usedNodes;
	ASNodeHashMap visited;
};

#endif
