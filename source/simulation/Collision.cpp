#include "Collision.h"
#include "EntityManager.h"

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
	return( HEntity() );
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