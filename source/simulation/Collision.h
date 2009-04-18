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

// Collision.h
// 
// Collision detection functions
//
// Usage: Fairly trivial; GetCollisionObject( CEntity* entity ) will return the first entity colliding with the given entity.
//		  The version with (x, y) parameters is just a helper; it temporarily moves the entity's collision bounds to the given
//          position before transferring to the other function.
// Notes: GetCollisionObject will only return the /first/ entity it finds in collision. This /may/ need a rethink when
//        multiple-entity (pileup) collisions become possible, I don't know.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef INCLUDED_COLLISION
#define INCLUDED_COLLISION

#include "BoundingObjects.h"

class CEntity;
class CPlayer;
class CStrW;

#include "EntityHandles.h"

struct rayIntersectionResults
{
	CEntity* Entity;
	CBoundingObject* boundingObject;
	CVector2D position;
	float closestApproach;
	float distance;
};

// maximum radius at which we check for entities (if some entity is much bigger than this, that's bad)
#define COLLISION_RANGE 20

typedef std::vector<CEntity*> RayIntersects;

HEntity GetCollisionObject( CEntity* entity, bool enablePassThroughAllies=true );
HEntity GetCollisionObject( CEntity* entity, float x, float y );
CBoundingObject* GetCollisionObject( CBoundingObject* bounds, CPlayer* player=0, const CStrW* ignoreClass=0 );
CEntity* GetCollisionEntity( CBoundingObject* bounds, CPlayer* player=0, const CStrW* ignoreClass=0 );
CBoundingObject* GetContainingObject( const CVector2D& point );
CEntity* GetCollisionObject( float x, float y ); // Point collision
bool GetRayIntersection( const CVector2D& source, const CVector2D& forward, const CVector2D& right, float length, float maxDistance, CBoundingObject* destinationCollisionObject, rayIntersectionResults* results );
// Assumes zero width, also includes moving units (other one doesn't).
void GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length, RayIntersects& results );
// Stores results in shared area
RayIntersects& GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length );

#endif
