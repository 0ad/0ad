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

#include "Collision.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EntityTemplate.h"

#include <float.h>

CBoundingObject* GetContainingObject( const CVector2D& point )
{
	std::vector<CEntity*> entities;
	g_EntityManager.GetInRange( point.x, point.y, COLLISION_RANGE, entities );
	std::vector<CEntity*>::iterator it;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds->Contains( point ) )
		{
			CBoundingObject* bounds = (*it)->m_bounds;
			return( bounds );
		}
	}

	return( NULL );
}

CEntity* GetCollisionObject( float x, float y )
{
	CVector2D point( x, y );

	std::vector<CEntity*> entities;
	g_EntityManager.GetInRange( x, y, COLLISION_RANGE, entities );
	std::vector<CEntity*>::iterator it;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds->Contains( point ) )
		{
			CEntity* e = (*it);
			return( e );
		}
	}

	return( NULL );
}

CBoundingObject* GetCollisionObject( CBoundingObject* bounds, CPlayer* player, const CStrW* ignoreClass )
{
	std::vector<CEntity*> entities;
	g_EntityManager.GetInRange( bounds->m_pos.x, bounds->m_pos.y, COLLISION_RANGE, entities );
	std::vector<CEntity*>::iterator it;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds == bounds ) continue;

		/* If the unit is marked to ignore ally collisions, and the player parameter 
		   is passed in and the same player as the unit, then ignore the (potential) collision */
		if( player && (*it)->m_base->m_passThroughAllies && (*it)->GetPlayer() == player ) continue;

		if( ignoreClass && (*it)->m_classes.IsMember( *ignoreClass ) ) continue;

		if( bounds->Intersects( (*it)->m_bounds ) )
		{
			CBoundingObject* obj = (*it)->m_bounds;
			return( obj );
		}
	}

	return( NULL );
}

CEntity* GetCollisionEntity( CBoundingObject* bounds, CPlayer* player, const CStrW* ignoreClass )
{
	std::vector<CEntity*> entities;
	g_EntityManager.GetInRange( bounds->m_pos.x, bounds->m_pos.y, COLLISION_RANGE, entities );
	std::vector<CEntity*>::iterator it;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds == bounds ) continue;

		/* If the unit is marked to ignore ally collisions, and the player parameter 
		   is passed in and the same player as the unit, then ignore the (potential) collision */
		if( player && (*it)->m_base->m_passThroughAllies && (*it)->GetPlayer() == player ) continue;

		if( ignoreClass && (*it)->m_classes.IsMember( *ignoreClass ) ) continue;

		if( bounds->Intersects( (*it)->m_bounds ) )
		{
			return (*it);
		}
	}

	return( NULL );
}

HEntity GetCollisionObject( CEntity* entity, bool enablePassThroughAllies )
{
#ifndef NDEBUG
	debug_assert( entity->m_bounds ); 
#else
	if( !entity->m_bounds ) return HEntity();
#endif

	std::vector<CEntity*> entities;
	g_EntityManager.GetInRange( entity->m_position.X, entity->m_position.Z, COLLISION_RANGE, entities );
	std::vector<CEntity*>::iterator it;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds == entity->m_bounds ) continue;

		if( enablePassThroughAllies 
				&& entity->m_base->m_passThroughAllies 
				&& (*it)->m_base->m_passThroughAllies
				&& entity->GetPlayer() == (*it)->GetPlayer() ) 
			continue;

		if( entity->m_bounds->Intersects( (*it)->m_bounds ) )
		{
			HEntity collisionObject = HEntity((*it)->me);
			return( collisionObject );
		}
	}

	return HEntity();
}

HEntity GetCollisionObject( CEntity* entity, float x, float y )
{
	float _x = entity->m_bounds->m_pos.x;
	float _y = entity->m_bounds->m_pos.y;
	entity->m_bounds->SetPosition( x, y );
	HEntity _e = GetCollisionObject( entity );
	entity->m_bounds->SetPosition( _x, _y );
	return( _e );
}

bool GetRayIntersection( const CVector2D& source, const CVector2D& forward, const CVector2D& right, float length, float maxDistance, CBoundingObject* destinationCollisionObject, rayIntersectionResults* results )
{
	std::vector<CEntity*> entities;
	g_EntityManager.GetExtant( entities );

	std::vector<CEntity*>::iterator it;

	float closestApproach, dist;

	CVector2D delta;

	results->distance = length + maxDistance;
	results->boundingObject = NULL;

	for( it = entities.begin(); it != entities.end(); it++ )
	{
		if( !(*it)->m_bounds ) continue;
		if( (*it)->m_bounds == destinationCollisionObject ) continue;
	
		// TODO MT: Replace this with something based on whether the unit is actually moving.
		if( (*it)->m_orderQueue.size() ) continue;

		CBoundingObject* obj = (*it)->m_bounds;
		delta = obj->m_pos - source;
		closestApproach = delta.Dot( right );
		dist = delta.Dot( forward );
		float collisionRadius = maxDistance + obj->m_radius;

		if( ( fabs( closestApproach ) < collisionRadius ) && ( dist > collisionRadius * 0.0f ) && ( dist < length - collisionRadius * 0.0f ) ) 
		{
			if( dist < results->distance )
			{
				results->boundingObject = obj;
				results->closestApproach = closestApproach;
				results->distance = dist;
				results->Entity = (*it);
				results->position = obj->m_pos;
			}
		}
	}
	if( results->boundingObject ) return( true );
	return( false );
}

void GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length, RayIntersects& results )
{
	results.clear();

	std::vector<CEntity*> entities;
	g_EntityManager.GetExtant( entities );

	float dist, closestApproach, l;
	CVector2D delta;

	std::vector<CEntity*>::iterator it;
	for( it = entities.begin(); it != entities.end(); it++ )
	{
		CBoundingObject* obj = (*it)->m_bounds;
		if( !obj ) continue;
		delta = obj->m_pos - position;
		closestApproach = delta.betadot( axis );
		if( fabs( closestApproach ) > obj->m_radius )
			continue; // Safe, doesn't get close enough.
		dist = delta.Dot( axis );
		// I just want to see if this will work before I simplify the maths
		l = sqrt( obj->m_radius * obj->m_radius - closestApproach * closestApproach );
		if( dist > 0 )
		{
			// Forward...
			if( ( dist - length ) > l )
				continue; // OK, won't reach it.
		}
		else
		{
			// Backward...
			if( -dist > l )
				continue; // OK, started far enough away
		}

		if( obj->m_type == CBoundingObject::BOUND_OABB )
		{
			// Run a more accurate test against the box
			CBoundingBox* box = (CBoundingBox*)obj;
			const float EPSILON = 0.0001f;
			float first = FLT_MAX, last = -FLT_MAX;
			CVector2D delta2;

			// Test against those sides of the box parallel with it's u vector.
			float t = box->m_u.y * axis.x - axis.y * box->m_u.x;
			float abs_t = fabs( t );
			if( abs_t >= EPSILON )
			{
				// If not parallel,
				delta2 = delta - box->m_v * box->m_w;
				if( fabs( axis.y * delta2.x - axis.x * delta2.y ) < box->m_d * abs_t )
				{
					// Possible intersection with one side
					float pos = ( box->m_u.y * delta2.x - box->m_u.x * delta2.y ) / t;
					if( pos < first ) first = pos;
					if( pos > last ) last = pos;
				}

				delta2 = delta + box->m_v * box->m_w;
				if( fabs( axis.y * delta2.x - axis.x * delta2.y ) < box->m_d * abs_t )
				{
					// Possible intersection with one side
					float pos = ( box->m_u.y * delta2.x - box->m_u.x * delta2.y ) / t;
					if( pos < first ) first = pos;
					if( pos > last ) last = pos;
				}
			}

			// Next test against those sides of the box parallel with it's v vector.
			t = box->m_v.y * axis.x - axis.y * box->m_v.x;
			abs_t = fabs( t );
			if( abs_t >= EPSILON )
			{
				// If not parallel,
				delta2 = delta - box->m_u * box->m_d;
				if( fabs( axis.y * delta2.x - axis.x * delta2.y ) < box->m_w * abs_t )
				{
					// Possible intersection with one side
					float pos = ( box->m_v.y * delta2.x - box->m_v.x * delta2.y ) / t;
					if( pos < first ) first = pos;
					if( pos > last ) last = pos;
				}

				delta2 = delta + box->m_u * box->m_d;
				if( fabs( axis.y * delta2.x - axis.x * delta2.y ) < box->m_w * abs_t )
				{
					// Possible intersection with one side
					float pos = ( box->m_v.y * delta2.x - box->m_v.x * delta2.y ) / t;
					if( pos < first ) first = pos;
					if( pos > last ) last = pos;
				}
			}

			// Then work out if we actually hit it within the given range.
			if( last < 0.0f )
				continue; // No, we started far enough 'after' there.
			if( first > length )
				continue; // No, we haven't yet moved far enough to hit it.
		}
		
		results.push_back( *it );
	}
}

static RayIntersects SharedResults;

RayIntersects& GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length )
{
	GetProjectileIntersection( position, axis, length, SharedResults );
	return( SharedResults );
}
