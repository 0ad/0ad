// EntityOrders.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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
//					  ORDER_ATTACK_MELEE:	Move towards target entity; start bashing it when close enough.
//											If we collide with something (=> line-of-sight tracking no longer
//											sufficient) spawns a ORDER_GOTO to target's location and pushes it
//											immediately in front of this order.
//
//		  Entities which exhaust all orders from their queue go to idle status; there is no specific order
//			  type for this status.

#ifndef ENTITY_ORDER_INCLUDED
#define ENTITY_ORDER_INCLUDED

#define ORDER_MAX_DATA 2

#include "EntityHandles.h"
#include "Vector2D.h"
#include "scripting/DOMEvent.h"

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
	enum
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
		NOTIFY_GATHER = 0x20

	}	m_type;
	
	CEntity* m_sender;
};



class CEntityOrder
{
public:
	enum
	{
		ORDER_GOTO_NOPATHING,
		ORDER_GOTO_SMOOTHED,
		ORDER_GOTO_COLLISION,
		ORDER_GOTO,
		ORDER_RUN,
		ORDER_PATROL,
		ORDER_PATH_END_MARKER,
		ORDER_GENERIC,
		ORDER_GENERIC_NOPATHING,
		ORDER_PRODUCE,
		ORDER_NOTIFY_REQUEST,
		ORDER_LAST
	} m_type;

	SOrderData m_data[ORDER_MAX_DATA];
};

#endif
