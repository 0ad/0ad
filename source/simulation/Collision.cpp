#include "Collision.h"
#include "EntityManager.h"

CBoundingObject* getContainingObject( const CVector2D& point )
{
	std::vector<HEntity>* entities = g_EntityManager.getActive();
	std::vector<HEntity>::iterator it;

	for( it = entities->begin(); it != entities->end(); it++ )
	{
		assert( (*it)->m_bounds );
		if( (*it)->m_bounds->contains( point ) )
		{
			CBoundingObject* bounds = (*it)->m_bounds;
			delete( entities );
			return( bounds );
		}
	}

	delete( entities );
	return( NULL );
}

HEntity getCollisionObject( CEntity* entity )
{
	assert( entity->m_bounds ); 

	std::vector<HEntity>* entities = g_EntityManager.getActive();
	std::vector<HEntity>::iterator it;

	for( it = entities->begin(); it != entities->end(); it++ )
	{
		assert( (*it)->m_bounds );
		if( (*it)->m_bounds == entity->m_bounds ) continue;
		if( entity->m_bounds->intersects( (*it)->m_bounds ) )
		{
			HEntity collisionObject = *it;
			delete( entities );
			return( collisionObject );
		}
	}

	delete( entities );
	return HEntity();
}

HEntity getCollisionObject( CEntity* entity, float x, float y )
{
	float _x = entity->m_bounds->m_pos.x;
	float _y = entity->m_bounds->m_pos.y;
	entity->m_bounds->setPosition( x, y );
	HEntity _e = getCollisionObject( entity );
	entity->m_bounds->setPosition( _x, _y );
	return( _e );
}

bool getRayIntersection( const CVector2D& source, const CVector2D& forward, const CVector2D& right, float length, float maxDistance, CBoundingObject* destinationCollisionObject, rayIntersectionResults* results )
{
	std::vector<HEntity>* entities = g_EntityManager.getActive();
	std::vector<HEntity>::iterator it;

	float closestApproach, dist;

	CVector2D delta;

	results->distance = length + maxDistance;
	results->boundingObject = NULL;

	for( it = entities->begin(); it != entities->end(); it++ )
	{
		assert( (*it)->m_bounds );
		if( (*it)->m_bounds == destinationCollisionObject ) continue;
		// HACK:
		if( (*it)->m_bounds->m_type == CBoundingObject::BOUND_OABB ) continue;
		if( (*it)->m_speed ) continue;
		CBoundingObject* obj = (*it)->m_bounds;
		delta = obj->m_pos - source;
		closestApproach = delta.dot( right );
		dist = delta.dot( forward );
		float collisionRadius = maxDistance + obj->m_radius;

		if( ( fabs( closestApproach ) < collisionRadius ) && ( dist > collisionRadius * 0.0f ) && ( dist < length - collisionRadius * 0.0f ) ) 
		{
			if( dist < results->distance )
			{
				results->boundingObject = obj;
				results->closestApproach = closestApproach;
				results->distance = dist;
				results->hEntity = (*it);
				results->position = obj->m_pos;
			}
		}
	}
	delete( entities );
	if( results->boundingObject ) return( true );
	return( false );
}
