// Entity.h
//
// Last modified: 26 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity class.
//
// Usage: Do not attempt to instantiate this class directly. (See EntityManager.h)
//		  Most of the members are trivially obvious; some highlights are:
//
//			HEntity me: is a reference to this entity. Use instead of the address-of operator for
//						non-temporary references. See EntityHandles.h
//						When an entity dies, this should be set to refer to the bad-handle handle.
//			CUnit* m_actor: is the visible representation of this entity.
//			std::hash_map m_properties: isn't yet used, is capable of storing properties defined by script.
//			
//			snapToGround(): Called every frame, this will ensure the entity never takes flight.
//			updateActorTransforms(): Must be called every time the position of this entity changes.
//			Also remember to update the collision object if you alter the position directly.
//
//			Some notes: update() and dispatch() /can/ be called directly without ill effects,
//						but it's preferable to go through the Entity manager and the Scheduler, respectively.
//
//			Collision detection/avoidance is now present in some form; this is a work in progress.

#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <deque>
#include "EntityProperties.h"

#include "BaseEntity.h"
#include "Vector2D.h"
#include "BoundingObjects.h"
#include "Vector3D.h"
#include "Unit.h"
#include "UnitManager.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "EntityMessage.h"

class CEntityManager;

class CEntity
{
	friend CEntityManager;
private:
	// Intrinsic properties
public:
	CStr m_name;
	float m_speed;
	CVector3D m_position;
	CBoundingObject* m_bounds;
	float m_targetorientation;
	CVector2D m_ahead;
	float m_orientation;
	CBaseEntity* m_base;
	CUnit* m_actor;

	std::deque<CEntityOrder> m_orderQueue;

	// Extended properties table

	STL_HASH_MAP<CStr,CGenericProperty,CStr_hash_compare> m_properties;

private:
	CEntity( CBaseEntity* base, CVector3D position, float orientation );
public:

	// Handle-to-self.
	HEntity me;
	void dispatch( CMessage* msg );
	void update( float timestep );
	void updateActorTransforms();
	void render();
	float getExactGroundLevel( float x, float y );
	void snapToGround();
	void pushOrder( CEntityOrder& order );
	HEntity getCollisionObject();
	HEntity getCollisionObject( float x, float y );
};

#endif