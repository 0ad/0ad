#include "precompiled.h"

#include "PathfindSparse.h"

sparsePathTree::sparsePathTree( const CVector2D& _from, const CVector2D& _to, HEntity _entity, CBoundingObject* _destinationCollisionObject )
{
	from = _from; to = _to;
	assert( from.length() > 0.01f );
	assert( to.length() > 0.01f );

	entity = _entity; destinationCollisionObject = _destinationCollisionObject;
	leftPre = NULL; leftPost = NULL;
	rightPre = NULL; rightPost = NULL;
	type = SPF_OPEN_UNVISITED;
	leftImpossible = false; rightImpossible = false;
	nextSubtree = 0;
}

sparsePathTree::~sparsePathTree()
{
	if( leftPre ) delete( leftPre );
	if( leftPost ) delete( leftPost );
	if( rightPre ) delete( rightPre );
	if( rightPost ) delete( rightPost );
}

bool sparsePathTree::slice()
{
	if( type == SPF_OPEN_UNVISITED )
	{
		rayIntersectionResults r;

		CVector2D forward = to - from;
		float len = forward.length();

		assert( len != 0.0f );

		forward /= len;
		CVector2D v_right = CVector2D( forward.y, -forward.x );

		if( !getRayIntersection( from, forward, v_right, len, entity->m_bounds->m_radius * 1.1f, destinationCollisionObject, &r ) )
		{
			type = SPF_CLOSED_DIRECT;
			return( true );
		}

 		float turningRadius = ( entity->m_bounds->m_radius + r.boundingObject->m_radius ) * 1.1f; 
		
		if( turningRadius < entity->m_turningRadius ) turningRadius = entity->m_turningRadius;

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
			float length = delta.length();

			float offsetDistance = ( turningRadius * length / sqrt( length * length - turningRadius * turningRadius ) );
			left = r.position - v_right * offsetDistance;
			right = r.position + v_right * offsetDistance;
		}
		favourLeft = false;
		if( r.closestApproach < 0 )
			favourLeft = true;
	
		// First we path to the left...
		
		leftPre = new sparsePathTree( from, left, entity, destinationCollisionObject );
		leftPost = new sparsePathTree( left, to, entity, destinationCollisionObject );

		// Then we path to the right...

		rightPre = new sparsePathTree( from, right, entity, destinationCollisionObject );
		rightPost = new sparsePathTree( right, to, entity, destinationCollisionObject );

		// If anybody reaches this point and is thinking:
		//
		// "Let's Do The Time-Warp Agaaaain!"
		//
		// Let me know.

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

void sparsePathTree::pushResults( std::vector<CVector2D>& nodelist )
{
	assert( type & SPF_SOLVED );

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
			assert( !rightImpossible );
			rightPost->pushResults( nodelist );
			rightPre->pushResults( nodelist );
		}
	}
}		

void nodeSmooth( HEntity entity, std::vector<CVector2D>& nodelist )
{
	std::vector<CVector2D>::iterator it;
	CVector2D next = nodelist.front();

	CEntityOrder node;
	node.m_type = CEntityOrder::ORDER_GOTO_SMOOTHED;
	node.m_data[0].location = next;

	entity->m_orderQueue.push_front( node );

	for( it = nodelist.begin() + 1; it != nodelist.end(); it++ )
	{
		if( ( it + 1 ) == nodelist.end() ) break;
		CVector2D current = *it;
		CVector2D previous = *( it + 1 );
		CVector2D u = current - previous;
		CVector2D v = next - current;
		u = u.normalize();
		v = v.normalize();
		CVector2D ubar = u.beta();
		CVector2D vbar = v.beta();
		float alpha = entity->m_turningRadius * ( ubar - vbar ).length() / ( u + v ).length();
		node.m_data[0].location = current - u * alpha;
		entity->m_orderQueue.push_front( node );
		next = current;
	}
	
	// If we try to apply turning constraints to getting onto this path, there's a reasonable
	// risk the entity will deviate so far from the first path segment that the path becomes
	// unwalkable for it.
	entity->m_orderQueue.front().m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
}

void pathSparse( HEntity entity, CVector2D destination )
{
	std::vector<CVector2D> pathnodes;
	CVector2D source( entity->m_position.X, entity->m_position.Z );
	sparsePathTree sparseEngine( source, destination, entity, getContainingObject( destination ) );
	while( sparseEngine.type & sparsePathTree::SPF_OPEN ) sparseEngine.slice();

	assert( sparseEngine.type & sparsePathTree::SPF_SOLVED ); // Shouldn't be any impossible cases yet.

	if( sparseEngine.type & sparsePathTree::SPF_SOLVED )
	{
		sparseEngine.pushResults( pathnodes );
		pathnodes.push_back( source );
		nodeSmooth( entity, pathnodes );
	}
	else
	{
		// Try a straight line. All we can do, really.
		CEntityOrder direct;
		direct.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
		direct.m_data[0].location = destination;
		entity->m_orderQueue.push_front( direct );
	}
}
