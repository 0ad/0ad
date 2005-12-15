// Entity.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity class.
//
// Usage: Do not attempt to instantiate this class directly. (See EntityManager.h)
//		  Most of the members are trivially obvious; some highlights are:
//
//			HEntity me: is a reference to this entity. See EntityHandles.h
//
//			Destroying entities: An entity is destroyed when all references to it expire.
//								 It is somewhat unfunny if this happens while a method from this
//                               class is still executing. If you need to kill an entity,
//								 use CEntity::kill(). If kill() releases the final handle,
//								 the entity is placed in the reaper queue and is deleted immediately
//								 prior to its next update cycle.
//
//			CUnit* m_actor: is the visible representation of this entity.
//			
//			snapToGround(): Called every frame, this will ensure the entity never takes flight.
//			updateActorTransforms(): Must be called every time the position of this entity changes.
//			Also remember to update the collision object if you alter the position directly.
//

#ifndef ENTITY_INCLUDED
#define ENTITY_INCLUDED

#include <deque>
#include "scripting/ScriptableComplex.h"
#include "Player.h"

#include "Vector2D.h"
#include "Vector3D.h"
#include "UnitManager.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "EntityMessage.h"
#include "EventHandlers.h"
#include "ScriptObject.h"
#include "ObjectEntry.h"
#include "EntitySupport.h"

class CBaseEntity;
class CBoundingObject;
class CUnit;
class CAura;

// TODO MT: Put this is /some/ sort of order...

class CEntity : public  CJSComplex<CEntity>, public IEventTarget
{
	friend class CEntityManager;
	
	typedef STL_HASH_MAP<CStrW, CAura*, CStrW_hash_compare> AuraTable;
	typedef std::set<CAura*> AuraSet;

private:
	// The player that owns this entity
	CPlayer* m_player;

public:
	// Intrinsic properties
	CBaseEntity* m_base;
	
	// The entity to switch to when this dies.
	CStrW m_corpse;

	// The class types this entity has
	SClassSet m_classes;

	float m_speed;
	float m_turningRadius;
	SEntityAction m_melee;
	SEntityAction m_gather;
	SEntityAction m_heal;
	bool m_selected;
	i32 m_grouped;

	// If this unit has been removed from the gameworld but has still
	// has references.
	bool m_destroyed;

	// If this unit is still active in the gameworld - i.e. not a corpse.
	bool m_extant;

	// HP properties
	float m_healthCurr;
	float m_healthMax;
	float m_healthBarHeight;
	
	// Minimap properties
	CStrW m_minimapType;
	int m_minimapR;
	int m_minimapG;
	int m_minimapB;

	// Y anchor
	CStrW m_anchorType;

	// LOS
	int m_los;
	bool m_permanent;

	// Auras
	AuraTable m_auras;
	AuraSet m_aurasInfluencingMe;

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

	// If the actor's current transform data is valid (i.e. the entity hasn't
	// moved since it was last calculated, and the terrain hasn't been changed).
	bool m_actor_transform_valid;

	//-- Scripts

	// Get script execution contexts - always run in the context of the entity that fired it.
	JSObject* GetScriptExecContext( IEventTarget* target ) { return( ((CEntity*)target)->GetScript() ); }

	CScriptObject m_EventHandlers[EVENT_LAST];

	CUnit* m_actor;

	// State transition in the FSM (animations should be reset)
	bool m_transition;
	int m_lastState;
	
	// Position in the current state's cycle
	static const size_t NOT_IN_CYCLE = (size_t)-1;
	size_t m_fsm_cyclepos; // -cycle_length....cycle_length
	CSkeletonAnim* m_fsm_animation; // the animation we're about to play this cycle,
	size_t m_fsm_anipos; // the time at which we should start playing it.
	size_t m_fsm_anipos2; // for when there are two animation-related events we need to take care of.

	std::deque<CEntityOrder> m_orderQueue;

private:
	CEntity( CBaseEntity* base, CVector3D position, float orientation );

	uint processGotoHelper( CEntityOrder* current, size_t timestep_milli, HEntity& collide );

	bool processContactAction( CEntityOrder* current, size_t timestep_millis, int transition, SEntityAction* action );
	bool processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, const CStr& animation, CScriptEvent* contactEvent, SEntityAction* action );

	bool processAttackMelee( CEntityOrder* current, size_t timestep_milli );
	bool processAttackMeleeNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processGather( CEntityOrder* current, size_t timestep_milli );
	bool processGatherNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processHeal( CEntityOrder* current, size_t timestep_milli );
	bool processHealNoPathing( CEntityOrder* current, size_t timestep_milli );
	
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

	// Forces update of actor information during next call to 'interpolate'.
	// (Necessary when terrain might move underneath the actor.)
	void invalidateActor();

	// Updates auras
	void UpdateAuras( size_t timestep_millis );

	// Removes entity from the gameworld and deallocates it, but not necessarily immediately.
	void kill();

	// Process initialization
	void Initialize();

	// Process tick.
	void Tick();

	// Store the player associated with this entity
	void SetPlayer(CPlayer *pPlayer);

	// Retrieve the player associated with this entity
	CPlayer* GetPlayer() { return m_player; } 

	// Process damage
	void Damage( CDamageType& damage, CEntity* inflictor = NULL );

	float getAnchorLevel( float x, float z );

	void snapToGround();
	void updateActorTransforms();

	// Getter and setter for the class sets
	jsval getClassSet();
	void setClassSet( jsval value );
	void rebuildClassSet();

	// Things like selection circles and debug info - possibly move to gui if/when it becomes responsible for (and capable of) it.
	void render();
	void renderSelectionOutline( float alpha = 1.0f );
	void renderHealthBar();

	// After a collision, recalc the path to the next fixed waypoint.
	void repath();

	// Reset properties after the entity-template we use changes.
	void loadBase();

	void playerChanged(); // Fixes player colour if player is changed by script
	void reorient(); // Orientation
	void teleport(); // Fixes things if the position is changed by something externally.
	void checkSelection(); // In case anyone tries to select/deselect this through JavaScript.
	void checkGroup(); // Groups
	void checkExtant(); // Existence

	// Returns whether the entity is capable of performing the given orderType on the target.
	bool acceptsOrder( int orderType, CEntity* orderTarget );

	void clearOrders();
	void pushOrder( CEntityOrder& order );

	// Script constructor

	static JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );
	
	bool Damage( JSContext* cx, uintN argc, jsval* argv );
	bool Kill( JSContext* cx, uintN argc, jsval* argv );
	jsval GetSpawnPoint( JSContext* cx, uintN argc, jsval* argv );

	jsval AddAura( JSContext* cx, uintN argc, jsval* argv );
	jsval RemoveAura( JSContext* cx, uintN argc, jsval* argv );

	bool Order( JSContext* cx, uintN argc, jsval* argv, bool Queued );
	inline bool OrderSingle( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, false ) );
	}
	inline bool OrderQueued( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, true ) );
	}

	bool IsIdle( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return( m_orderQueue.empty() );
	}

	bool HasClass( JSContext* cx, uintN argc, jsval* argv )
	{
		debug_assert( argc >= 1 );
		return( m_classes.IsMember( ToPrimitive<CStrW>( cx, argv[0] ) ) );
	}

	static void ScriptingInit();

private:
	// squelch "unable to generate" warnings
	CEntity(const CEntity& rhs);
	const CEntity& operator=(const CEntity& rhs);
};

// General entity globals


// In its current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE 

extern int SELECTION_CIRCLE_POINTS;
extern int SELECTION_BOX_POINTS;
extern int SELECTION_SMOOTHNESS_UNIFIED;

#endif
