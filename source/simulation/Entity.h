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
class CProductionQueue;

class CEntityFormation;

// TODO MT: Put this is /some/ sort of order...

class CEntity : public  CJSComplex<CEntity>, public IEventTarget
{
	friend class CEntityManager;

	typedef STL_HASH_MAP<CStrW, CAura*, CStrW_hash_compare> AuraTable;
	typedef STL_HASH_MAP<int, SEntityAction> ActionTable;
	typedef std::set<CAura*> AuraSet;

public:
	// The player that owns this entity
	CPlayer* m_player;

	// Intrinsic properties
	CBaseEntity* m_base;

	// The entity to switch to when this dies.
	CStrW m_corpse;

	// The class types this entity has
	SClassSet m_classes;

	// Production queue
	CProductionQueue* m_productionQueue;

	float m_speed;
	float m_turningRadius;
	float m_runRegenRate;
	float m_runDecayRate;

	float m_healthRegenRate;
	float m_healthRegenStart;
	float m_healthDecayRate;

	SEntityAction m_run;

	ActionTable m_actions;

	bool m_selected;
	i32 m_grouped;
	int m_formation;	//Indice of which formation we're in
	int m_formationSlot;	//The slot of the above formation

	// If this unit has been removed from the gameworld but has still
	// has references.
	bool m_destroyed;

	// If this unit is still active in the gameworld - i.e. not a corpse.
	bool m_extant;

	bool m_isRunning;	//is it actually running
	bool m_shouldRun;	//if run was issued, it will remain true until it is stopped
	bool m_triggerRun;	//used in SetRun, corrects 1 frame stamina imbalance
	int m_frameCheck;	//counts the frame

	float m_lastCombatTime;

	// Building to convert to if this is a foundation, or "" otherwise
	CStrW m_building;

	//SP properties
	float m_staminaCurr;
	float m_staminaMax;
	float m_staminaBarHeight;
	int m_staminaBarSize;
	float m_staminaBarWidth;

	int m_staminaBorderWidth;
	int m_staminaBorderHeight;
	CStr m_staminaBorderName;

	// HP properties
	float m_healthCurr;
	float m_healthMax;
	float m_healthBarHeight;
	int m_healthBarSize;
	float m_healthBarWidth;

	int m_healthBorderWidth;
	int m_healthBorderHeight;
	CStr m_healthBorderName;

	//Rank properties
	float m_rankHeight;
	int m_rankWidth;
	CStr m_rankName;

	bool m_healthDecay;

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

	// Our current collision patch in CEntityManager
	std::vector<CEntity*>* m_collisionPatch;

	//-- Scripts

	// Get script execution contexts - always run in the context of the entity that fired it.
	JSObject* GetScriptExecContext( IEventTarget* target ) { return( ((CEntity*)target)->GetScript() ); }

	CScriptObject m_EventHandlers[EVENT_LAST];

	CUnit* m_actor;
	std::set<CStrW> m_actorSelections;

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
	std::deque<CEntityListener> m_listeners;

	std::vector<CEntity*> m_notifiers;
	int m_currentNotification;	//Current order in the form of a notification code
	int m_currentRequest;	//Notification we our notifiers are sending
	bool m_destroyNotifiers;	//True: we destroy them. False: the script does.

	std::vector<bool> m_sectorValues;
	std::vector<float> m_sectorAngles;
	int m_sectorDivs;
	float m_sectorPenalty;

private:
	CEntity( CBaseEntity* base, CVector3D position, float orientation, const std::set<CStrW>& actorSelections, CStrW building = L"" );

	uint processGotoHelper( CEntityOrder* current, size_t timestep_milli, HEntity& collide );

	bool processContactAction( CEntityOrder* current, size_t timestep_millis, int transition, SEntityAction* action );
	bool processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, const CStr& animation, CScriptEvent* contactEvent, SEntityAction* action );

	bool processGeneric( CEntityOrder* current, size_t timestep_milli );
	bool processGenericNoPathing( CEntityOrder* current, size_t timestep_milli );

	bool processProduce( CEntityOrder* order );

	bool processGotoNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processGoto( CEntityOrder* current, size_t timestep_milli );
	bool processGotoWaypoint( CEntityOrder* current, size_t timestep_milli, bool contact );

	bool processPatrol( CEntityOrder* current, size_t timestep_milli );

	float processChooseMovement( float distance );

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

	// Update collision patch (move ourselves to a new one if necessary)
	void updateCollisionPatch();

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
	void renderBarBorders( );
	void renderHealthBar();
	void renderStaminaBar();
	void renderRank();
	CVector2D getScreenCoords( float height );
	// After a collision, recalc the path to the next fixed waypoint.
	void repath();

	//Calculate stamina points
	void CalculateRun(float timestep);
	void CalculateHealth(float timestep);

	// Reset properties after the entity-template we use changes.
	void loadBase();

	void playerChanged(); // Fixes player colour if player is changed by script
	void reorient(); // Orientation
	void teleport(); // Fixes things if the position is changed by something externally.
	void checkSelection(); // In case anyone tries to select/deselect this through JavaScript.
	void checkGroup(); // Groups
	void checkExtant(); // Existence

	void clearOrders();
	void popOrder();	//Use this if and order has finished instead of m_orderQueue.pop_front()
	void pushOrder( CEntityOrder& order );

	void DispatchNotification( CEntityOrder order, int type );
	int DestroyNotifier( CEntity* target );	//Stop notifier from sending to us
	void DestroyAllNotifiers();

	CEntityFormation* GetFormation();
	bool IsInClass( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenalty( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenaltyType( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenaltyVal( JSContext* cx, uintN argc, jsval* argv );

	jsval GetFormationBonus( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationBonusType( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationBonusVal( JSContext* cx, uintN argc, jsval* argv );

	void DispatchFormationEvent( int type );

	jsval RegisterDamage( JSContext* cx, uintN argc, jsval* argv );
	jsval RegisterIdle( JSContext* cx, uintN argc, jsval* argv );
	jsval GetAttackDirections( JSContext* cx, uintN argc, jsval* argv );
	// Script constructor

	static JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	bool Kill( JSContext* cx, uintN argc, jsval* argv );
	jsval GetSpawnPoint( JSContext* cx, uintN argc, jsval* argv );

	jsval AddAura( JSContext* cx, uintN argc, jsval* argv );
	jsval RemoveAura( JSContext* cx, uintN argc, jsval* argv );

	jsval SetActionParams( JSContext* cx, uintN argc, jsval* argv );
	jsval TriggerRun( JSContext* cx, uintN argc, jsval* argv );
	jsval SetRun( JSContext* cx, uintN argc, jsval* argv );
	jsval IsRunning( JSContext* cx, uintN argc, jsval* argv );
	jsval GetRunState( JSContext* cx, uintN argc, jsval* argv );

	bool RequestNotification( JSContext* cx, uintN argc, jsval* argv );
	//Just in case we want to explicitly check the listeners without waiting for the order to be pushed
	bool ForceCheckListeners( JSContext* cx, uintN argc, jsval* argv );
	void CheckListeners( int type, CEntity *target );
	jsval DestroyAllNotifiers( JSContext* cx, uintN argc, jsval* argv );
	jsval DestroyNotifier( JSContext* cx, uintN argc, jsval* argv );

	bool IsInFormation( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return ( m_formation != 0 ? true : false );
	}

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
	jsval TerminateOrder( JSContext* UNUSED(cx), uintN argc, jsval* argv )
	{
		debug_assert( argc >= 1);
		if ( ToPrimitive<bool>( argv[0] ) )
			m_orderQueue.clear();
		else
			m_orderQueue.pop_front();
		return JSVAL_VOID;
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
