// Collision.h
//
// Last modified: 28 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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

HEntity getCollisionObject( CEntity* entity );
HEntity getCollisionObject( CEntity* entity, float x, float y );

#endif