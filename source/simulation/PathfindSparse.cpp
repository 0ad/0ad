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

#include "PathfindSparse.h"
#include "graphics/Terrain.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "Entity.h"

int SPF_RECURSION_DEPTH = 10;

SparsePathTree::SparsePathTree( const CVector2D& _from, const CVector2D& _to, HEntity _entity, CBoundingObject* _destinationCollisionObject, int _recursionDepth )
{
	from = _from; to = _to;
	recursionDepth = _recursionDepth;

	entity = _entity; destinationCollisionObject = _destinationCollisionObject;
	leftPre = NULL; leftPost = NULL;
	rightPre = NULL; rightPost = NULL;
	type = SPF_OPEN_UNVISITED;
	leftImpossible = false; rightImpossible = false;
	nextSubtree = 0;
}

SparsePathTree::~SparsePathTree()
{
	if( leftPre ) delete( leftPre );
	if( leftPost ) delete( leftPost );
	if( rightPre ) delete( rightPre );
	if( rightPost ) delete( rightPost );
}

bool SparsePathTree::slice()
{
	CTerrain *pTerrain = g_Game->GetWorld()->GetTerrain();
	
	if( type == SPF_OPEN_UNVISITED )
	{
		if( !recursionDepth )
		{
			// Too deep.
			type = SPF_IMPOSSIBLE;
			return( true );
		}

		rayIntersectionResults r;

		CVector2D forward = to - from;
		float len = forward.Length();

		if( len == 0.0f )
		{
			// Too weird. (Heavy traffic, obstacles in positions leading to this degenerate state.
			type = SPF_IMPOSSIBLE;
			return( true );
		}

		forward /= len;
		CVector2D v_right = CVector2D( forward.y, -forward.x );

		if( !GetRayIntersection( from, forward, v_right, len, entity->m_bounds->m_radius * 1.1f, destinationCollisionObject, &r ) )
		{
			type = SPF_CLOSED_DIRECT;
			return( true );
		}

 		float turningRadius = ( entity->m_bounds->m_radius + r.boundingObject->m_radius ) * 1.1f; 
		
		if( entity->m_turningRadius > turningRadius ) turningRadius = entity->m_turningRadius;

		// Too close, an impossible turn
		if( r.distance < turningRadius )
		{
			// Too close to make a proper turn; try dodging immediately a long way to the left or right.
			left = from - v_right * r.boundingObject->m_radius * 2.5f;
			right = from + v_right * r.boundingObject->m_radius * 2.5f;
		}
		else if( r.distance > ( len - turningRadius ) )
		{
			// Again, too close to avoid it properly. Try approaching the goal from the left or right.
			left = to - v_right * r.boundingObject->m_radius * 2.5f;
			right = to + v_right * r.boundingObject->m_radius * 2.5f;
		}
		else
		{
			// Dodge to the left or right of the obstacle.
			// A distance of offsetDistance is sufficient to guarantee we'll make the turn.

			CVector2D delta = r.position - from;
			float length = delta.Length();

			float offsetDistance = ( turningRadius * length / sqrt( length * length - turningRadius * turningRadius ) );
			
			left = r.position - v_right * offsetDistance;
			right = r.position + v_right * offsetDistance;
		}
		favourLeft = false;
		if( r.closestApproach < 0 )
			favourLeft = true;
	
		
		// First we path to the left...
		
		leftPre = new SparsePathTree( from, left, entity, destinationCollisionObject, recursionDepth - 1 );
		leftPost = new SparsePathTree( left, to, entity, destinationCollisionObject, recursionDepth - 1 );

		// Then we path to the right...

		rightPre = new SparsePathTree( from, right, entity, destinationCollisionObject, recursionDepth - 1 );
		rightPost = new SparsePathTree( right, to, entity, destinationCollisionObject, recursionDepth - 1 );

		// If anybody reaches this point and is thinking:
		//
		// "Let's Do The Time-Warp Agaaaain!"
		//
		// Let me know.

		// Check that the subwaypoints are on the map...
		if( !pTerrain->IsOnMap( left ) )
		{
			// Shut that path down
			leftPre->type = SPF_IMPOSSIBLE;
			leftPost->type = SPF_IMPOSSIBLE;
		}

		if( !pTerrain->IsOnMap( right ) )
		{
			// Shut that path down
			rightPre->type = SPF_IMPOSSIBLE;
			rightPost->type = SPF_IMPOSSIBLE;
		}

		if( ( leftPre->type == SPF_IMPOSSIBLE ) && ( rightPre->type == SPF_IMPOSSIBLE ) )
		{
			// It's unlikely, but I suppose it /might/ happen
			type = SPF_IMPOSSIBLE;
			return( true );
		}

		type = SPF_OPEN_PROCESSING;

		return( true );
	}
	else /* type == SPF_OPEN_PROCESSING */
	{
		bool done = false;
		while( !done )
		{
			if( subtrees[nextSubtree]->type & SPF_OPEN )
				if( subtrees[nextSubtree]->slice() )
					done = true;
			nextSubtree++;
			nextSubtree %= 4;
		}
		if( ( leftPre->type == SPF_IMPOSSIBLE ) || ( leftPost->type == SPF_IMPOSSIBLE ) )
			leftImpossible = true;
		if( ( rightPre->type == SPF_IMPOSSIBLE ) || ( rightPost->type == SPF_IMPOSSIBLE ) )
			rightImpossible = true;
		if( leftImpossible && rightImpossible )
		{
			type = SPF_IMPOSSIBLE;
			return( done );
		}
		if( ( ( leftPre->type & SPF_SOLVED ) && ( leftPost->type & SPF_SOLVED ) ) ||
			( ( rightPre->type & SPF_SOLVED ) && ( rightPost->type & SPF_SOLVED ) ) )
		{
			type = SPF_CLOSED_WAYPOINTED;
			return( done );
		}
		return( done );
	}
}

void SparsePathTree::pushResults( std::vector<CVector2D>& nodelist )
{
	debug_assert( type & SPF_SOLVED );

	if( type == SPF_CLOSED_DIRECT )
	{
		nodelist.push_back( to );
	}
	else /* type == SPF_CLOSED_WAYPOINTED */
	{
		leftImpossible = !( ( leftPre->type & SPF_SOLVED ) && ( leftPost->type & SPF_SOLVED ) );
		rightImpossible = !( ( rightPre->type & SPF_SOLVED ) && ( rightPost->type & SPF_SOLVED ) );

		if( !leftImpossible && ( favourLeft || rightImpossible ) )
		{
			leftPost->pushResults( nodelist );
			leftPre->pushResults( nodelist );
		}
		else
		{
			debug_assert( !rightImpossible );
			rightPost->pushResults( nodelist );
			rightPre->pushResults( nodelist );
		}
	}
}		

void NodePostProcess( HEntity entity, std::vector<CVector2D>& nodelist )
{
	std::vector<CVector2D>::iterator it;
	CVector2D next = nodelist.front();

	CEntityOrder node;
	node.m_type = CEntityOrder::ORDER_PATH_END_MARKER;
	entity->m_orderQueue.push_front( node );

	node.m_type = CEntityOrder::ORDER_GOTO_SMOOTHED;
	node.m_target_location = next;

	entity->m_orderQueue.push_front( node );

	for( it = nodelist.begin() + 1; it != nodelist.end(); it++ )
	{
		if( ( it + 1 ) == nodelist.end() ) break;
		CVector2D current = *it;
		CVector2D previous = *( it + 1 );
		CVector2D u = current - previous;
		CVector2D v = next - current;
		u = u.Normalize();
		v = v.Normalize();
		CVector2D ubar = u.beta();
		CVector2D vbar = v.beta();
		float alpha = entity->m_turningRadius * ( ubar - vbar ).Length() / ( u + v ).Length();
		node.m_target_location = current - u * alpha;
		entity->m_orderQueue.push_front( node );
		next = current;
	}
	
	// If we try to apply turning constraints to getting onto this path, there's a reasonable
	// risk the entity will deviate so far from the first path segment that the path becomes
	// unwalkable for it.
	entity->m_orderQueue.front().m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
}

void PathSparse( HEntity entity, CVector2D destination )
{
	std::vector<CVector2D> pathnodes;
	CVector2D source( entity->m_position.X, entity->m_position.Z );
	// Sanity check:
	if( source.Length() < 0.01f ) return;

	SparsePathTree sparseEngine( source, destination, entity, GetContainingObject( destination ), SPF_RECURSION_DEPTH );
	while( sparseEngine.type & SparsePathTree::SPF_OPEN ) sparseEngine.slice();

	// debug_assert( sparseEngine.type & SparsePathTree::SPF_SOLVED ); // Shouldn't be any impossible cases yet.

	if( sparseEngine.type & SparsePathTree::SPF_SOLVED )
	{
		sparseEngine.pushResults( pathnodes );
		pathnodes.push_back( source );
		NodePostProcess( entity, pathnodes );
	}
	else
	{
		// Try a straight line. All we can do, really.
		CEntityOrder direct;
		direct.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
		direct.m_target_location = destination;
		entity->m_orderQueue.push_front( direct );
	}
}
