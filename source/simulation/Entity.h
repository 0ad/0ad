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
#include "scripting/ScriptableObject.h"
#include "Player.h"

#include "Vector2D.h"
#include "Vector3D.h"
#include "UnitManager.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "EntityMessage.h"
#include "EventHandlers.h"
#include "ScriptObject.h"

#include "EntitySupport.h"

class CBaseEntity;
class CBoundingObject;
class CUnit;

// TODO MT: Put this is /some/ sort of order...

class CEntity : public CJSObject<CEntity>
{
	friend class CEntityManager;
public:
	// Intrinsic properties
	CBaseEntity* m_base;
	
	// The entity to switch to when this dies.
	CStrW m_corpse;

	float m_speed;
	float m_turningRadius;
	float m_meleeRange;
	float m_meleeRangeMin;
	bool m_selected;
	i32 m_grouped;

	// The player that owns this entity
	CPlayer* m_player;

	// If this unit has been removed from the gameworld but has still
	// has references.
	bool m_destroyed;

	// If this unit is still active in the gameworld - i.e. not a corpse.
	bool m_extant;

	//-- Interpolated property
	CVector3D m_position;
	CVector3D m_position_previous;
	CVector3D m_graphics_position;

	CBoundingObject* m_bounds;
	float m_targetorientation;
	CVector2D m_ahead;

	//-- Interpolated property
	float m_orientation;
	float m_orientation_previous;
	float m_graphics_orientation;

	//-- Scripts

	// EventListener adaptation? 
	//typedef std::vector<CScriptObject> ExtendedHandlerList;
	//typedef STL_HASH_MAP<CStrW, HandlerList, CStrW_hash_compare> ExtendedHandlerTable;

	CScriptObject m_EventHandlers[EVENT_LAST];

	// EventListener adaptation?
	/*
	void AddListener( const CStrW& EventType, CScriptObject* Handler )
	{
		m_EventHandlers[EventType].push_back( Handler );
	}
	*/

	bool DispatchEvent( CScriptEvent* evt );

	CUnit* m_actor;

	// State transition in the FSM (animations should be reset)
	bool m_transition;
	int m_lastState;

	std::deque<CEntityOrder> m_orderQueue;

private:
	CEntity( CBaseEntity* base, CVector3D position, float orientation );

	/*EGotoSituation*/ uint processGotoHelper( CEntityOrder* current, size_t timestep_milli, HEntity& collide );
	bool processAttackMelee( CEntityOrder* current, size_t timestep_milli );
	bool processAttackMeleeNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processGotoNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processGoto( CEntityOrder* current, size_t timestep_milli );
	bool processPatrol( CEntityOrder* current, size_t timestep_milli );

public:
	~CEntity();

	// Handle-to-self.
	HEntity me;

	// Updates gameplay information for the specified timestep
	void update( size_t timestep_millis );
	// Updates graphical information for a point between the last and current simulation frame; 0 < relativeoffset < 1.
	void interpolate( float relativeoffset );

	// Removes entity from the gameworld and deallocates it, but not neccessarily immediately.
	void kill();

	// Process initialization
	void Initialize();

	// Process tick.
	void Tick();

	// Process damage
	void Damage( CDamageType& damage, CEntity* inflictor = NULL );

	void snapToGround();
	void updateActorTransforms();

	// Things like selection circles and debug info - possibly move to gui if/when it becomes responsible for (and capable of) it.
	void render();
	void renderSelectionOutline( float alpha = 1.0f );

	// After a collision, recalc the path to the next fixed waypoint.
	void repath();

	// Reset properties after the entity-template we use changes.
	void loadBase();

	void reorient(); // Orientation
	void teleport(); // Fixes things if the position is changed by something externally.
	void checkSelection(); // In case anyone tries to select/deselect this through JavaScript.
	void checkGroup(); // Groups
	void checkExtant(); // Existence

	// Returns the default action of the entity upon the target (or -1 if none apply)
	int CEntity::defaultOrder( CEntity* orderTarget );
	// Returns whether the entity is capable of performing the given orderType on the target.
	bool acceptsOrder( int orderType, CEntity* orderTarget );

	void clearOrders();
	void pushOrder( CEntityOrder& order );

	// Script constructor

	static JSBool Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval );

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );
	bool Order( JSContext* cx, uintN argc, jsval* argv, bool Queued );
	inline bool OrderSingle( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, false ) );
	}
	inline bool OrderQueued( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, true ) );
	}
	bool Damage( JSContext* cx, uintN argc, jsval* argv );
	bool Kill( JSContext* cx, uintN argc, jsval* argv );
	bool IsIdle( JSContext* cx, uintN argc, jsval* argv )
	{
		return( m_orderQueue.empty() );
	}
	static void ScriptingInit();
};

// General entity globals


// In its current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE 

extern int SELECTION_CIRCLE_POINTS;
extern int SELECTION_BOX_POINTS;
extern int SELECTION_SMOOTHNESS_UNIFIED;

#endif
