// PathfindSparse.h
//
// Last modified: 28 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Sparse pathfinder.
//
// Usage: You won't. See PathfindEngine.h
//
// Notes: A geometric pathfinder primarily for path postprocessing. Takes straight line
//          paths and warps them to avoid obstacles.
//        Sparse, because it runs in /exponential/ time with number of detours. Hence, only use
//          where obstructions are sparse. You'll have fun if you try and path through, say, 
//          a forest with this.
//
//        It also won't work at all through impassable terrain.
//
// TODO: Fix the algorithm for OABB obstacles.
// TODO: Replace with timesliced version and proper manager (either a singleton or tasks linked into g_Pathfinder)
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef PATHFIND_SPARSE_INCLUDED
#define PATHFIND_SPARSE_INCLUDED

#include "EntityHandles.h"
#include "Vector2D.h"
#include "Collision.h"

struct sparsePathTree
{
	enum
	{
		SPF_IMPOSSIBLE = 0,
		SPF_OPEN_UNVISITED = 4,
		SPF_OPEN_PROCESSING = 5,
		SPF_CLOSED_DIRECT = 2,
		SPF_CLOSED_WAYPOINTED = 3,
		SPF_OPEN = 4,
		SPF_SOLVED = 2
	} type;
	HEntity entity;
	CBoundingObject* destinationCollisionObject;
	CVector2D from;
	CVector2D to;
	bool leftImpossible;
	CVector2D left;
	bool rightImpossible;
	CVector2D right;
	bool favourLeft;
	union
	{
		struct
		{
			sparsePathTree* leftPre;
			sparsePathTree* leftPost;
			sparsePathTree* rightPre;
			sparsePathTree* rightPost;
		};
		sparsePathTree* subtrees[4];
	};
	unsigned short nextSubtree;
	sparsePathTree( const CVector2D& from, const CVector2D& to, HEntity entity, CBoundingObject* destinationCollisionObject );
	~sparsePathTree();
	bool slice();
	void pushResults( std::vector<CVector2D>& nodelist );
};

void nodeSmooth( HEntity entity, std::vector<CVector2D>& nodelist );
void pathSparse( HEntity entity, CVector2D destination );
bool pathSparseRecursive( HEntity entity, CVector2D from, CVector2D to, CBoundingObject* destinationCollisionObject );

#endif
