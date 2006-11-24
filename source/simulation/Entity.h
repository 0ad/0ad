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
#include "ps/Vector2D.h"
#include "maths/Vector3D.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "EntityMessage.h"
#include "ScriptObject.h"
#include "EntitySupport.h"
#include "scripting/DOMEvent.h"
#include "scripting/ScriptCustomTypes.h"

class CAura;
class CEntityTemplate;
class CBoundingObject;
class CPlayer;
class CProductionQueue;
class CSkeletonAnim;
class CUnit;
class CTerritory;
class CEntityFormation;
class CStance;

// 
enum EntityFlags
{
	// If this unit has been removed from the gameworld but has still
	// has references.
	ENTF_DESTROYED                  = 0x0001,

	// State transition in the FSM (animations should be reset)
	ENTF_TRANSITION                 = 0x0002,

	ENTF_HAS_RALLY_POINT            = 0x0004,

	ENTF_HEALTH_DECAY               = 0x0008,

	// if set, we destroy them; otherwise, the script does.
	ENTF_DESTROY_NOTIFIERS          = 0x0010,

	// is it actually running
	ENTF_IS_RUNNING                 = 0x0020,
	// if run was issued, it will remain true until it is stopped
	ENTF_SHOULD_RUN                 = 0x0040,
	// used in SetRun, corrects 1 frame stamina imbalance
	ENTF_TRIGGER_RUN                = 0x0080
};

// TODO MT: Put this is /some/ sort of order...

class CEntity : public CJSComplex<CEntity>, public IEventTarget
{
	friend class CEntityManager;
	friend class CUnit;

	typedef STL_HASH_MAP<CStrW, CAura*, CStrW_hash_compare> AuraTable;
	typedef STL_HASH_MAP<int, SEntityAction> ActionTable;
	typedef std::set<CAura*> AuraSet;

public:
	// Intrinsic properties
	CEntityTemplate* m_base;

	// The class types this entity has
	CClassSet m_classes;

	// Production queue
	CProductionQueue* m_productionQueue;

	// Unit AI stance
	CStance* m_stance;

	// Movement properties
	float m_speed;
	float m_turningRadius;
	float m_runSpeed;
	float m_runRegenRate;
	float m_runDecayRate;

	float m_maxActorPitch;
	float m_minActorPitch;

	float m_healthRegenRate;
	float m_healthRegenStart;
	float m_healthDecayRate;

	float m_runMaxRange;
	float m_runMinRange;
	
	ActionTable m_actions;

	bool m_selected;
	i32 m_grouped;
	int m_formation;	// Index of which formation we're in
	int m_formationSlot;	// The slot of the above formation

	uint ent_flags;
	bool entf_get(uint desired_flag) const { return (ent_flags & desired_flag) != 0; }
	void entf_set(uint desired_flag) { ent_flags |= desired_flag; }
	void entf_clear(uint desired_flag) { ent_flags &= ~desired_flag; }
	void entf_set_to(uint desired_flag, bool value)
	{
		ent_flags &= ~desired_flag;
		const uint mask = value? ~0u : 0;
		ent_flags |= desired_flag & mask;
	}

	// If this unit is still active in the gameworld - i.e. not a corpse.
	bool m_extant;

	// If this is false, the unit will not be drawn and cannot be interacted with using the mouse.
	bool m_visible;

	int m_frameCheck;	//counts the frame

	float m_lastCombatTime;
	float m_lastRunTime;

	// Building to convert to if this is a foundation, or "" otherwise
	CStrW m_building;

	//SP properties
	float m_staminaCurr;
	float m_staminaMax;

	// HP properties
	float m_healthCurr;
	float m_healthMax;

	// Rank properties
	CStr m_rankName;

	// Stance name
	CStr m_stanceName;

	// LOS
	int m_los;

	// If the object is a territory centre, this points to its territory
	CTerritory* m_associatedTerritory;

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
	CVector3D m_orientation;
	CVector3D m_orientation_previous;
	CVector3D m_graphics_orientation;
	
	CVector2D m_orientation_unclamped;

	CVector3D m_rallyPoint;	// valid iff ENT_HAS_RALLY_POINT

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
	std::set<CStr8> m_actorSelections;

	int m_lastState;	// used in animation FSM

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

	std::vector<bool> m_sectorValues;
	//Slight optimization for aura rendering
	std::vector< std::vector<CVector2D> > m_unsnappedPoints;

private:
	CEntity( CEntityTemplate* base, CVector3D position, float orientation, const std::set<CStr8>& actorSelections, const CStrW* building = 0 );

	uint processGotoHelper( CEntityOrder* current, size_t timestep_milli, HEntity& collide );

	bool processContactAction( CEntityOrder* current, size_t timestep_millis, CEntityOrder::EOrderType transition, SEntityAction* action );
	bool processContactActionNoPathing( CEntityOrder* current, size_t timestep_millis, const CStr& animation, CScriptEvent* contactEvent, SEntityAction* action );

	bool processGeneric( CEntityOrder* current, size_t timestep_milli );
	bool processGenericNoPathing( CEntityOrder* current, size_t timestep_milli );

	bool processProduce( CEntityOrder* order );

	bool processGotoNoPathing( CEntityOrder* current, size_t timestep_milli );
	bool processGoto( CEntityOrder* current, size_t timestep_milli );
	bool processGotoWaypoint( CEntityOrder* current, size_t timestep_milli, bool contact );

	bool processPatrol( CEntityOrder* current, size_t timestep_milli );

	float chooseMovementSpeed( float distance );
	
	bool shouldRun( float distance );		// Given our distance to a target, can we be running?

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
	bool Initialize();
	void initAuraData();

	// Process tick.
	void Tick();

	// Calculate distances along the terrain

	float distance2D( float x, float z )
	{
		float dx = x - m_position.X;
		float dz = z - m_position.Z;
		return sqrt( dx*dx + dz*dz );
	}

	float distance2D( CEntity* other )
	{
		return distance2D( other->m_position.X, other->m_position.Z );
	}

	float distance2D( CVector2D p )
	{
		return distance2D( p.x, p.y );
	}

private:
	// The player that owns this entity.
	// (Player is set publicly via CUnit::SetPlayerID)
	CPlayer* m_player;
	void SetPlayer(CPlayer* player);
public:
	// Retrieve the player associated with this entity.
	CPlayer* GetPlayer() { return m_player; }

	// Update collision patch (move ourselves to a new one if necessary)
	void updateCollisionPatch();

	float getAnchorLevel( float x, float z );

	void snapToGround();
	void updateActorTransforms();
	void updateXZOrientation();

	// Getter and setter for the class sets
	jsval getClassSet();
	void setClassSet( jsval value );
	void rebuildClassSet();

	// Things like selection circles and debug info - possibly move to gui if/when it becomes responsible for (and capable of) it.
	void render();
	void renderSelectionOutline( float alpha = 1.0f );
	void renderAuras();
	void renderBars();
	void renderBarBorders();
	void renderHealthBar();
	void renderStaminaBar();
	void renderRank();
	void renderRallyPoint();
	
	// Utility functions for rendering:

	// Draw rectangle around the given centre, aligned with the given axes
	void drawRect(CVector3D& centre, CVector3D& up, CVector3D& right, float x1, float y1, float x2, float y2);
	void drawBar(CVector3D& centre, CVector3D& up, CVector3D& right, 
		float x1, float y1, float x2, float y2,
		SColour col1, SColour col2, float currVal, float maxVal);

	CVector2D getScreenCoords( float height );

	// After a collision, recalc the path to the next fixed waypoint.
	void repath();

	//Calculate stamina points
	void CalculateRegen(float timestep);

	// Reset properties after the entity-template we use changes.
	void loadBase();
	static void initAttributes(const CEntity* _this);

	void reorient(); // Orientation
	void teleport(); // Fixes things if the position is changed by something externally.
	void stanceChanged(); // Sets m_stance to the right CStance object when our stance property is changed through scripts
	void checkSelection(); // In case anyone tries to select/deselect this through JavaScript.
	void checkGroup(); // Groups
	void checkExtant(); // Existence

	void clearOrders();
	void popOrder();	//Use this if and order has finished instead of m_orderQueue.pop_front()
	void pushOrder( CEntityOrder& order );

	void DispatchNotification( CEntityOrder order, int type );
	int DestroyNotifier( CEntity* target );	//Stop notifier from sending to us
	void DestroyAllNotifiers();

	int findSector( int divs, float angle, float maxAngle, bool negative=true );
	jsval FlattenTerrain( JSContext* cx, uintN argc, jsval* argv );

	CEntityFormation* GetFormation();
	jsval GetFormationPenalty( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenaltyBase( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenaltyType( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationPenaltyVal( JSContext* cx, uintN argc, jsval* argv );

	jsval GetFormationBonus( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationBonusBase( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationBonusType( JSContext* cx, uintN argc, jsval* argv );
	jsval GetFormationBonusVal( JSContext* cx, uintN argc, jsval* argv );

	void DispatchFormationEvent( int type );

	jsval RegisterDamage( JSContext* cx, uintN argc, jsval* argv );
	jsval RegisterOrderChange( JSContext* cx, uintN argc, jsval* argv );
	jsval GetAttackDirections( JSContext* cx, uintN argc, jsval* argv );

	jsval FindSector( JSContext* cx, uintN argc, jsval* argv );
	// Script constructor

	static JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );

	// Script-bound functions

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );

	bool Kill( JSContext* cx, uintN argc, jsval* argv );
	jsval GetSpawnPoint( JSContext* cx, uintN argc, jsval* argv );

	inline jsval HasRallyPoint( JSContext* cx, uintN argc, jsval* argv );
	inline jsval GetRallyPoint( JSContext* cx, uintN argc, jsval* argv );
	inline jsval SetRallyPoint( JSContext* cx, uintN argc, jsval* argv );

	jsval AddAura( JSContext* cx, uintN argc, jsval* argv );
	jsval RemoveAura( JSContext* cx, uintN argc, jsval* argv );

	jsval SetActionParams( JSContext* cx, uintN argc, jsval* argv );
	jsval TriggerRun( JSContext* cx, uintN argc, jsval* argv );
	jsval SetRun( JSContext* cx, uintN argc, jsval* argv );
	jsval IsRunning( JSContext* cx, uintN argc, jsval* argv );
	jsval GetRunState( JSContext* cx, uintN argc, jsval* argv );
	
	jsval OnDamaged( JSContext* cx, uintN argc, jsval* argv );

	jsval GetVisibleEntities( JSContext* cx, uintN argc, jsval* argv );

	float GetDistance( JSContext* cx, uintN argc, jsval* argv );

	bool RequestNotification( JSContext* cx, uintN argc, jsval* argv );
	//Just in case we want to explicitly check the listeners without waiting for the order to be pushed
	bool ForceCheckListeners( JSContext* cx, uintN argc, jsval* argv );
	int GetCurrentRequest( JSContext* cx, uintN argc, jsval* argv );
	void CheckListeners( int type, CEntity *target );
	jsval DestroyAllNotifiers( JSContext* cx, uintN argc, jsval* argv );
	jsval DestroyNotifier( JSContext* cx, uintN argc, jsval* argv );

	jsval JSI_GetPlayer();
	void JSI_SetPlayer(jsval val);

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

	jsval GetHeight( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return ToJSVal(m_position.Y);
	}

	static void ScriptingInit();

	// Functions that call script code
	int GetAttackAction( HEntity target );

	NO_COPY_CTOR(CEntity);
};

// General entity globals


// In its current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE

extern int SELECTION_CIRCLE_POINTS;
extern int SELECTION_BOX_POINTS;
extern int SELECTION_SMOOTHNESS_UNIFIED;
extern int AURA_CIRCLE_POINTS;

#endif
