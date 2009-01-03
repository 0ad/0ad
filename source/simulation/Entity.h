// Entity.h
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
//								 use CEntity::Kill(). If Kill() releases the final handle,
//								 the entity is placed in the reaper queue and is deleted immediately
//								 prior to its next update cycle.
//
//			CUnit* m_actor: is the visible representation of this entity.
//
//			SnapToGround(): Called every frame, this will ensure the entity never takes flight.
//			UpdateActorTransforms(): Must be called every time the position of this entity changes.
//			Also remember to update the collision object if you alter the position directly.
//

#ifndef INCLUDED_ENTITY
#define INCLUDED_ENTITY

#include <deque>
#include "scripting/ScriptableComplex.h"
#include "ps/Vector2D.h"
#include "maths/Vector3D.h"
#include "EntityOrders.h"
#include "EntityHandles.h"
#include "ScriptObject.h"
#include "EntitySupport.h"
#include "scripting/DOMEvent.h"
#include "scripting/ScriptCustomTypes.h"
#include "ps/scripting/JSCollection.h"

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
	NONCOPYABLE(CEntity);

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
	ssize_t m_grouped;	// group index or -1 if not grouped
	ssize_t m_formation;	// Index of which formation we're in or -1
	ssize_t m_formationSlot;	// The slot of the above formation

	int ent_flags;
	bool entf_get(int desired_flag) const { return (ent_flags & desired_flag) != 0; }
	void entf_set(int desired_flag) { ent_flags |= desired_flag; }
	void entf_clear(int desired_flag) { ent_flags &= ~desired_flag; }
	void entf_set_to(int desired_flag, bool value)
	{
		ent_flags &= ~desired_flag;
		const int mask = value? ~0u : 0;
		ent_flags |= desired_flag & mask;
	}

	// If this unit is still active in the gameworld - i.e. not a corpse.
	bool m_extant;

	// If this is false, the unit will not be drawn and cannot be interacted with using the mouse.
	bool m_visible;

	int m_frameCheck;	//counts the frame

	double m_lastCombatTime;
	double m_lastRunTime;

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

	// LOS distance/range
	ssize_t m_los;

	// If the object is a territory centre, this points to its territory
	CTerritory* m_associatedTerritory;

	// Territorial limit
	CStrW m_buildingLimitCategory;

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
	CVector3D m_orientation_smoothed; // used for slow graphical-only rotation - tends towards m_orientation
	CVector3D m_orientation_previous; // previous smoothed value
	CVector3D m_graphics_orientation;
	
	CVector2D m_orientation_unclamped;

	CVector2D m_rallyPoint;	// valid iff ENT_HAS_RALLY_POINT

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
	static const int NOT_IN_CYCLE = -1;
	int m_fsm_cyclepos; // -cycle_length....cycle_length

	CEntityOrders m_orderQueue;
	std::deque<CEntityListener> m_listeners;

	std::vector<CEntity*> m_notifiers;
	int m_currentNotification;	//Current order in the form of a notification code
	int m_currentRequest;	//Notification we our notifiers are sending

	std::vector<bool> m_sectorValues;
	//Slight optimization for aura rendering
	std::vector< std::vector<CVector2D> > m_unsnappedPoints;
	
	//Kai: add id to identify the entity in the polygon soup
	//	   needed for update/remove entities in the triangulation.
	int m_dcdtId;
	void removeObstacle();

private:
	CEntity( CEntityTemplate* base, CVector3D position, float orientation, const std::set<CStr8>& actorSelections, const CStrW* building = 0 );

	// (returns EGotoSituation, but we don't want to expose that)
	int ProcessGotoHelper( CEntityOrder* current, int timestep_millis, HEntity& collide, float& timeLeft );

	bool ProcessContactAction( CEntityOrder* current, int timestep_millis, bool repath_if_needed );
	bool ProcessContactActionNoPathing( CEntityOrder* current, int timestep_millis );

	bool ProcessProduce( CEntityOrder* order );

	bool ProcessGotoNoPathing( CEntityOrder* current, int timestep_millis );
	bool ProcessGotoNoPathingContact( CEntityOrder* current, int timestep_millis );
	bool ProcessGoto( CEntityOrder* current, int timestep_millis );
	bool ProcessGotoWaypoint( CEntityOrder* current, int timestep_millis, bool contact );

	bool ProcessPatrol( CEntityOrder* current, int timestep_millis );

	float ChooseMovementSpeed( float distance );
	
	bool ShouldRun( float distance );		// Given our distance to a target, can we be running?

	void UpdateOrders( int timestep_millis );

public:
	~CEntity();

	// Handle-to-self.
	HEntity me;

	// Updates gameplay information for the specified timestep
	void Update( int timestep_millis );
	// Updates graphical information for a point between the last and current
	// simulation frame; should be 0 <= relativeoffset <= 1 (else it'll be
	// clamped)
	void Interpolate( float relativeoffset );

	// Forces update of actor information during next call to 'interpolate'.
	// (Necessary when terrain might move underneath the actor.)
	void InvalidateActor();

	// Updates auras
	void UpdateAuras( int timestep_millis );

	// Exit auras we're currently in (useful for example when we change state)
	void ExitAuras();

	// Removes entity from the gameworld and deallocates it, but not necessarily immediately.
	// The keepActor parameter specifies whether to remove the unit's actor immediately (for
	// units we want removed immediately, e.g. in Alkas) or to keep it and play the death
	// animation (for units that die of "natural causes").
	void Kill(bool keepActor = false);

	// Process initialization
	bool Initialize();
	void initAuraData();

	// Process tick.
// 	void Tick(); // (see comment in CEntityManager::UpdateAll)

	// Calculate distances along the terrain

	float Distance2D( float x, float z )
	{
		float dx = x - m_position.X;
		float dz = z - m_position.Z;
		return sqrt( dx*dx + dz*dz );
	}

	float Distance2D( CEntity* other )
	{
		return Distance2D( other->m_position.X, other->m_position.Z );
	}

	float Distance2D( CVector2D p )
	{
		return Distance2D( p.x, p.y );
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
	void UpdateCollisionPatch();

	float GetAnchorLevel( float x, float z );

	void SnapToGround();
	void UpdateActorTransforms();
	void UpdateXZOrientation();

	// Getter and setter for the class sets
	jsval GetClassSet();
	void SetClassSet( jsval value );
	void RebuildClassSet();

	// Things like selection circles and debug info - possibly move to gui if/when it becomes responsible for (and capable of) it.
	void Render();
	void RenderSelectionOutline( float alpha = 1.0f );
	void RenderAuras();
	void RenderBars();
	void RenderBarBorders();
	void RenderHealthBar();
	void RenderStaminaBar();
	void RenderRank();
	void RenderRallyPoint();
	
	// Utility functions for rendering:

	// Draw rectangle around the given centre, aligned with the given axes
	void DrawRect(CVector3D& centre, CVector3D& up, CVector3D& right, float x1, float y1, float x2, float y2);
	void DrawBar(CVector3D& centre, CVector3D& up, CVector3D& right, 
		float x1, float y1, float x2, float y2,
		SColour col1, SColour col2, float currVal, float maxVal);

	CVector2D GetScreenCoords( float height );

	// After a collision, recalc the path to the next fixed waypoint.
	void Repath();

	//Calculate stamina points
	void CalculateRegen(float timestep);

	// Reset properties after the entity-template we use changes.
	void LoadBase();
	static void InitAttributes(const CEntity* _this);

	void Reorient(); // Orientation
	void Teleport(); // Fixes things if the position is changed by something externally.
	void StanceChanged(); // Sets m_stance to the right CStance object when our stance property is changed through scripts
	void CheckSelection(); // In case anyone tries to select/deselect this through JavaScript.
	void CheckGroup(); // Groups
	void CheckExtant(); // Existence

	void ClearOrders();
	void PopOrder();	//Use this if an order has finished instead of m_orderQueue.pop_front()
	void PushOrder( const CEntityOrder& order );

	void DispatchNotification( const CEntityOrder& order, int type );
	int DestroyNotifier( CEntity* target );	//Stop notifier from sending to us
	void DestroyAllNotifiers();

	int FindSector( int divs, float angle, float maxAngle, bool negative=true );
	jsval_t FlattenTerrain( JSContext* cx, uintN argc, jsval* argv );

	CEntityFormation* GetFormation();
	jsval_t GetFormationPenalty( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationPenaltyBase( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationPenaltyType( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationPenaltyVal( JSContext* cx, uintN argc, jsval* argv );

	jsval_t GetFormationBonus( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationBonusBase( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationBonusType( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetFormationBonusVal( JSContext* cx, uintN argc, jsval* argv );

	void DispatchFormationEvent( int type );

	jsval_t RegisterDamage( JSContext* cx, uintN argc, jsval* argv );
	jsval_t RegisterOrderChange( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetAttackDirections( JSContext* cx, uintN argc, jsval* argv );

	jsval_t FindSector( JSContext* cx, uintN argc, jsval* argv );

	// Script constructor
	static JSBool Construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );

	// Script-bound functions

	jsval_t ToString( JSContext* cx, uintN argc, jsval* argv );

	bool Kill( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetSpawnPoint( JSContext* cx, uintN argc, jsval* argv );

	jsval_t HasRallyPoint( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetRallyPoint( JSContext* cx, uintN argc, jsval* argv );
	jsval_t SetRallyPointAtCursor( JSContext* cx, uintN argc, jsval* argv );

	jsval_t AddAura( JSContext* cx, uintN argc, jsval* argv );
	jsval_t RemoveAura( JSContext* cx, uintN argc, jsval* argv );

	jsval_t SetActionParams( JSContext* cx, uintN argc, jsval* argv );
	jsval_t TriggerRun( JSContext* cx, uintN argc, jsval* argv );
	jsval_t SetRun( JSContext* cx, uintN argc, jsval* argv );
	jsval_t IsRunning( JSContext* cx, uintN argc, jsval* argv );
	jsval_t GetRunState( JSContext* cx, uintN argc, jsval* argv );
	
	jsval_t OnDamaged( JSContext* cx, uintN argc, jsval* argv );

	jsval_t GetVisibleEntities( JSContext* cx, uintN argc, jsval* argv );

	float GetDistance( JSContext* cx, uintN argc, jsval* argv );

	bool RequestNotification( JSContext* cx, uintN argc, jsval* argv );
	//Just in case we want to explicitly check the listeners without waiting for the order to be pushed
	bool ForceCheckListeners( JSContext* cx, uintN argc, jsval* argv );
	int GetCurrentRequest( JSContext* cx, uintN argc, jsval* argv );
	void CheckListeners( int type, CEntity *target );
	jsval_t DestroyAllNotifiers( JSContext* cx, uintN argc, jsval* argv );
	jsval_t DestroyNotifier( JSContext* cx, uintN argc, jsval* argv );

	jsval JSI_GetPlayer();
	void JSI_SetPlayer(jsval val);

	bool IsInFormation( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return ( m_formation != 0 ? true : false );
	}

	bool Order( JSContext* cx, uintN argc, jsval* argv, CEntityOrder::EOrderSource source, bool Queued );

	// TODO: Replace these variants of Order() with a single function, and update scripts accordingly.
	inline bool OrderSingle( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, CEntityOrder::SOURCE_PLAYER, false ) );
	}
	inline bool OrderQueued( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, CEntityOrder::SOURCE_PLAYER, true ) );
	}
	inline bool OrderFromTriggers( JSContext* cx, uintN argc, jsval* argv )
	{
		return( Order( cx, argc, argv, CEntityOrder::SOURCE_TRIGGERS, true ) );
	}

	bool IsIdle( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return( m_orderQueue.empty() );
	}

	bool IsDestroyed( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return( entf_get(ENTF_DESTROYED) );
	}

	bool HasClass( JSContext* cx, uintN argc, jsval* argv )
	{
		debug_assert( argc >= 1 );
		return( m_classes.IsMember( ToPrimitive<CStrW>( cx, argv[0] ) ) );
	}

	jsval_t TerminateOrder( JSContext* UNUSED(cx), uintN argc, jsval* argv )
	{
		debug_assert( argc >= 1);
		if ( ToPrimitive<bool>( argv[0] ) )
			m_orderQueue.clear();
		else
			m_orderQueue.pop_front();
		return JSVAL_VOID;
	}

	jsval_t GetHeight( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
	{
		return ToJSVal(m_position.Y);
	}

	static void ScriptingInit();

	// Functions that call script code
	int GetAttackAction( HEntity target );
};

typedef CJSCollection<HEntity, &CEntity::JSI_class> EntityCollection;

// General entity globals


// In its current incarnation, inefficient but pretty
#define SELECTION_TERRAIN_CONFORMANCE

extern int SELECTION_CIRCLE_POINTS;
extern int SELECTION_BOX_POINTS;
extern int SELECTION_SMOOTHNESS_UNIFIED;
extern int AURA_CIRCLE_POINTS;

#endif
