// Entity.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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
//								 use CEntity::kill(). If kill() releases the final handle,
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

class CEntity : public IPropertyOwner
{
	friend class CEntityManager;
public:
	// Intrinsic properties
	CProperty_CStr m_name;
	CProperty_float m_speed;
	CProperty_float m_turningRadius;
	CProperty_bool m_selected;

	bool m_extant; // Don't want JS to have direct write-access to these. (Things that should be done might not be)
	CProperty_bool m_extant_mirror; // plus this way limits the number of nasty semantics to work around.
	u8 m_grouped;
	CProperty_i32 m_grouped_mirror;

	//-- Interpolated property
	CVector3D m_position;
	CVector3D m_position_previous;
	CProperty_CVector3D m_graphics_position;

	CVector2D m_graphicsOffset;

	CBoundingObject* m_bounds;
	float m_targetorientation;
	CVector2D m_ahead;

	//-- Interpolated property
	float m_orientation;
	float m_orientation_previous;
	CProperty_float m_graphics_orientation;

	CUnit* m_actor;

	std::deque<CEntityOrder> m_orderQueue;

private:
	CEntity( CBaseEntity* base, CVector3D position, float orientation );

	bool processGotoNoPathing( CEntityOrder* current, float timestep );
	bool processGoto( CEntityOrder* current, float timestep );
	bool processPatrol( CEntityOrder* current, float timestep );

public:
	~CEntity();

	// Handle-to-self.
	HEntity me;

	void dispatch( const CMessage* msg );
	void update( float timestep );
	void kill();

	void interpolate( float relativeoffset );
	void snapToGround();
	void updateActorTransforms();
	void render();
	void renderSelectionOutline( float alpha = 1.0f );

	void repath();

	void loadBase();

	void reorient(); // Orientation
	void teleport(); // Fixes things if the position is changed by something externally.
	void checkSelection(); // In case anyone tries to select/deselect this through JavaScript. You'd think they'd have something better to do.
	void checkGroup(); // Groups
	void checkExtant(); // Existance

	bool acceptsOrder( int orderType, CEntity* orderTarget );

	void clearOrders();
	void pushOrder( CEntityOrder& order );
};

// General entity globals


// In it's current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE 

extern int SELECTION_CIRCLE_POINTS;
extern int SELECTION_BOX_POINTS;
extern int SELECTION_SMOOTHNESS_UNIFIED;


#endif
