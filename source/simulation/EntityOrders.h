// EntityOrders.h
// 
// Entity orders structure.
//
// Usage: All orders at this point use the location component of the union.
//		  Orders are: ORDER_GOTO_NOPATHING: Attempts to reach the given destination via a line-of-sight
//					  ORDER_GOTO_SMOOTED:	system. Do not create an order of these types directly; it is
//											used to return a path of line segments from the pathfinder.
//											_SMOOTHED flags to the entity state-control that it's OK to
//											smooth the corner between segments. _NOPATHING just does
//											zero-radius turns.
//					  ORDER_GOTO_COLLISION: When the coldet system is trying to get us out of a collision,
//											it generates these intermediate waypoints. We don't really have
//											any reason to go to this specific point, so if a better way
//											comes along, this order can be deleted.
//					  ORDER_GOTO:			Attempts to reach the given destination. Uses the pathfinder
//											to... er... find the path.
//											Create this order when a standard movement or movement waypoint
//											order is required.
//					  ORDER_PATROL:			As ORDER_GOTO, but pushes the patrol order onto the back of the
//											order queue after it's executed. In this way, the entity will
//											circle round a list of patrol points.
//											Create this order when a standard patrol order is required.
//					  ORDER_GENERIC:		Generic ranged action. Move towards target entity, then start
//											performing an action (call a JS event handler every few seconds).
//											If we collide with something (=> line-of-sight tracking no longer
//											sufficient) spawns a ORDER_GOTO to target's location and pushes it
//											immediately in front of this order.
//
//		  Entities which exhaust all orders from their queue go to idle status; there is no specific order
//			  type for this status.

#ifndef INCLUDED_ENTITYORDERS
#define INCLUDED_ENTITYORDERS

#define ORDER_MAX_DATA 2

#include "EntityHandles.h"
#include "ps/Vector2D.h"

#include <deque>

// An order data field, which could represent different things depending on the type of order.
struct SOrderData
{
	CVector2D location;
	HEntity entity;
	CStrW string;
	u64 data;  // could be recast as a double or int
};

class CEntityListener
{
public:
	enum EType
	{
		NOTIFY_NONE = 0x00,

		NOTIFY_GOTO = 0x01,
		NOTIFY_RUN = 0x02,
		NOTIFY_FOLLOW = 0x03,	//GOTO | RUN

		NOTIFY_ATTACK = 0x04,
		NOTIFY_DAMAGE = 0x08,
		NOTIFY_COMBAT = 0x0C,	//ATTACK | DAMAGE

		NOTIFY_ESCORT = 0x0F,	//GOTO | ATTACK | DAMAGE
		NOTIFY_HEAL = 0x10,
		NOTIFY_GATHER = 0x20,
		
		NOTIFY_IDLE = 0x40, 
		NOTIFY_ORDER_CHANGE = 0x80,	//this isn't counted in NOTIFY_ALL
		NOTIFY_ALL = 0x7F

	}	m_type;
	
	CEntity* m_sender;
};

class CEntityOrder
{
public:
	enum EOrderType
	{
		ORDER_INVALID,				// 0
		ORDER_GOTO_NOPATHING,		// 1
		ORDER_GOTO_SMOOTHED,		// 2
		ORDER_GOTO_COLLISION,		// 3
		ORDER_GOTO_WAYPOINT,		// 4
		ORDER_GOTO_WAYPOINT_CONTACT,// 5
		ORDER_GOTO,					// 6
		ORDER_RUN,					// 7
		ORDER_PATROL,				// 8
		ORDER_PATH_END_MARKER,		// 9
		ORDER_GENERIC,				// 10
		ORDER_GENERIC_NOPATHING,	// 11
		ORDER_PRODUCE,				// 12
		ORDER_START_CONSTRUCTION,	// 13
		ORDER_NOTIFY_REQUEST,		// 14
		ORDER_LAST					// 15
	};
	EOrderType m_type;

	enum EOrderSource
	{
		SOURCE_PLAYER,
		SOURCE_UNIT_AI,
		SOURCE_TRIGGERS
	};
	EOrderSource m_source;

	// all commands involving pathfinder (i.e. all :P)
	float m_pathfinder_radius;

	// NMT_PlaceObject
	HEntity m_new_obj;

	// NMT_Goto
	// NMT_FormationGoto
	// NMT_Run
	// NMT_Patrol
	// NMT_AddWaypoint
	CVector2D m_target_location;

	// NMT_Generic
	// NMT_FormationGeneric
	// NMT_NotifyRequest
	HEntity m_target_entity;
	int m_action;

	// NMT_Produce
	CStrW m_produce_name;
	int m_produce_type;

	// NMT_Generic
	bool m_run;

	CEntityOrder(): m_type(ORDER_INVALID), m_source(SOURCE_PLAYER) {}

	CEntityOrder(EOrderType type, EOrderSource source=SOURCE_PLAYER)
		: m_type(type), m_source(source) {}
};

typedef std::deque<CEntityOrder> CEntityOrders;
typedef CEntityOrders::iterator CEntityOrderIt;
typedef CEntityOrders::const_iterator CEntityOrderCIt;
typedef CEntityOrders::const_reverse_iterator CEntityOrderCRIt;


#endif
