// Collision.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Collision detection functions
//
// Usage: Fairly trivial; getCollisionObject( CEntity* entity ) will return the first entity colliding with the given entity.
//		  The version with (x, y) parameters is just a helper; it temporarily moves the entity's collision bounds to the given
//          position before transferring to the other function.
// Notes: getCollisionObject will only return the /first/ entity it finds in collision. This /may/ need a rethink when
//        multiple-entity (pileup) collisions become possible, I don't know.
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com

#ifndef COLLISION_INCLUDED
#define COLLISION_INCLUDED

#include "BoundingObjects.h"
#include "Entity.h"

struct rayIntersectionResults
{
	CEntity* Entity;
	CBoundingObject* boundingObject;
	CVector2D position;
	float closestApproach;
	float distance;
};

typedef std::vector<CEntity*> RayIntersects;

HEntity getCollisionObject( CEntity* entity );
HEntity getCollisionObject( CEntity* entity, float x, float y );
CBoundingObject* getCollisionObject( CBoundingObject* bounds );
CBoundingObject* getContainingObject( const CVector2D& point );
CEntity* GetCollisionObject( float x, float y ); // Point collision
bool getRayIntersection( const CVector2D& source, const CVector2D& forward, const CVector2D& right, float length, float maxDistance, CBoundingObject* destinationCollisionObject, rayIntersectionResults* results );
// Assumes zero width, also includes moving units (other one doesn't).
void GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length, RayIntersects& results );
// Stores results in shared area
RayIntersects& GetProjectileIntersection( const CVector2D& position, const CVector2D& axis, float length );

#endif
