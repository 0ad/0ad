#include "PathfindSparse.h"

#define NODESMOOTH_STEPS 4

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
		forward /= len;
		CVector2D right = CVector2D( forward.y, -forward.x );

		// Hit nothing or hit destination; that's OK.
		if( !getRayIntersection( from, forward, right, len, entity->m_bounds->m_radius * 1.1f, &r ) || ( r.boundingObject == destinationCollisionObject ) )
		{
			type = SPF_CLOSED_DIRECT;
			return( true );
		}

		float turningRadius = ( entity->m_bounds->m_radius + r.boundingObject->m_radius ) * 1.1f; 
		
		if( turningRadius < entity->m_turningRadius ) turningRadius = entity->m_turningRadius;

		// Too close, an impossible turn
		if( r.distance < turningRadius ||
			r.distance > ( len - turningRadius ) )
		{
			type = SPF_IMPOSSIBLE;
			return( true );
		}

		CVector2D delta = r.position - from;
		float length = delta.length();

		float offsetDistance = ( turningRadius * length / sqrt( length * length - turningRadius * turningRadius ) );
		
		favourLeft = false;
		if( r.closestApproach < 0 )
			favourLeft = true;

		// First we path to the left...

		left = r.position - right * offsetDistance;
		leftPre = new sparsePathTree( from, left, entity, destinationCollisionObject );
		leftPost = new sparsePathTree( left, to, entity, destinationCollisionObject );

		// Then we path to the right...

		right = r.position + right * offsetDistance;
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
		if( !leftImpossible )
		{
			if( !done && ( leftPre->type & SPF_OPEN ) )
				done |= leftPre->slice();
			if( !done && ( leftPost->type & SPF_OPEN ) )
				done |= leftPost->slice();
			if( ( leftPre->type == SPF_IMPOSSIBLE ) || ( leftPost->type == SPF_IMPOSSIBLE ) )
				leftImpossible = true;
		}
		if( !rightImpossible && !done )
		{
			if( !done && ( rightPre->type & SPF_OPEN ) )
				done |= rightPre->slice();
			if( !done && ( rightPost->type & SPF_OPEN ) )
				done |= rightPost->slice();
			if( ( rightPre->type == SPF_IMPOSSIBLE ) || ( rightPost->type == SPF_IMPOSSIBLE ) )
				rightImpossible = true;
		}
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
	// All your CPU are belong to us.
	// But Jan wanted it ;)

	std::vector<CVector2D>::iterator it;
	CVector2D next = nodelist.front();

	CEntityOrder node;
	node.m_type = CEntityOrder::ORDER_GOTO_NOPATHING;
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
		u *= alpha;
		v *= alpha;

		for( int t = NODESMOOTH_STEPS; t >= 0; t-- )
		{
			float lambda = t / (float)NODESMOOTH_STEPS;
			CVector2D arcpoint = current + v * lambda * lambda - u * ( 1 - lambda ) * ( 1 - lambda );
			node.m_data[0].location = arcpoint;
			entity->m_orderQueue.push_front( node );
		}
		
		next = current;
	}
}

void pathSparse( HEntity entity, CVector2D destination )
{
	std::vector<CVector2D> pathnodes;
	sparsePathTree sparseEngine( CVector2D( entity->m_position.X, entity->m_position.Z ), destination, entity, getContainingObject( destination ) );
	while( sparseEngine.type & sparsePathTree::SPF_OPEN ) sparseEngine.slice();

	if( sparseEngine.type & sparsePathTree::SPF_SOLVED )
	{
		sparseEngine.pushResults( pathnodes );
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