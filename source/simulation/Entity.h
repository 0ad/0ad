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
//
//			Destroying entities: An entity is destroyed when all references to it expire.
//								 It is somewhat unfunny if this happens while a method from this
//                               class is still executing. If you need to kill an entity,
//								 use g_EntityManager.kill( entity ). If kill() releases the final handle,
//								 the entity is placed in the reaper queue and is deleted immediately
//								 prior to its next update cycle.
//
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
	friend class CEntityManager;
private:
	// Intrinsic properties
public:
	CStr m_name;
	float m_speed;
	float m_turningRadius;
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

	bool processGotoNoPathing( CEntityOrder* current, float timestep );
	bool processGoto( CEntityOrder* current, float timestep );
	bool processPatrol( CEntityOrder* current, float timestep );

public:
	~CEntity();

	// Handle-to-self.
	HEntity me;
	void dispatch( CMessage* msg );
	void update( float timestep );
	void updateActorTransforms();
	void render();
	float getExactGroundLevel( float x, float y );
	void snapToGround();
	void pushOrder( CEntityOrder& order );
};

#endif
