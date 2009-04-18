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

// PathfindSparse.h
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

#ifndef INCLUDED_PATHFINDSPARSE
#define INCLUDED_PATHFINDSPARSE

#include "EntityHandles.h"
#include "ps/Vector2D.h"
#include "Collision.h"

struct SparsePathTree
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
	int recursionDepth;
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
			SparsePathTree* leftPre;
			SparsePathTree* leftPost;
			SparsePathTree* rightPre;
			SparsePathTree* rightPost;
		};
		SparsePathTree* subtrees[4];
	};
	unsigned short nextSubtree;
	SparsePathTree( const CVector2D& from, const CVector2D& to, HEntity entity, CBoundingObject* destinationCollisionObject, int _recursionDepth );
	~SparsePathTree();
	bool slice();
	void pushResults( std::vector<CVector2D>& nodelist );
};

extern int SPF_RECURSION_DEPTH;

void NodePostProcess( HEntity entity, std::vector<CVector2D>& nodelist );
void PathSparse( HEntity entity, CVector2D destination );
bool PathSparseRecursive( HEntity entity, CVector2D from, CVector2D to, CBoundingObject* destinationCollisionObject );

#endif
