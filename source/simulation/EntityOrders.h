// EntityOrders.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity orders structure.
//
// Usage: All orders at this point use the location component of the union.
//		  Orders are: ORDER_GOTO_NOPATHING: Attempts to reach the given destination via a line-of-sight
//											system. Do not create an order of this type directly; it is
//											used to return a path of line segments from the pathfinder.
//					  ORDER_GOTO:			Attempts to reach the given destination. Uses the pathfinder
//											to... er... find the path.
//											Create this order when a standard movement or movement waypoint
//											order is required.
//					  ORDER_PATROL:			As ORDER_GOTO, but pushes the patrol order onto the back of the
//											order queue after it's executed. In this way, the entity will
//											circle round a list of patrol points.
//											Create this order when a standard patrol order is required.
//
//		  Entities which exhaust all orders from their queue go to idle status; there is no specific order
//			  type for this status.

#ifndef ENTITY_ORDER_INCLUDED
#define ENTITY_ORDER_INCLUDED

#define ORDER_MAX_DATA 1

#include "EntityHandles.h"

struct SOrderData
{
	union
	{
		struct
		{
			float x;
			float y;
		} location;
		u64 data;  // miscellaneous
	};
	HEntity entity;
};

class CEntityOrder
{
public:
	enum
	{
		ORDER_GOTO_NOPATHING,
		ORDER_GOTO,
		ORDER_PATROL
	} m_type;
	SOrderData m_data[ORDER_MAX_DATA];
};

#endif