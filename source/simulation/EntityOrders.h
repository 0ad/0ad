/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
//					  ORDER_CONTACT_ACTION:	Generic ranged action. Move towards target entity, then start
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
#include "ps/CStr.h"

#include <deque>

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
		ORDER_INVALID,					// 0
		ORDER_GOTO_NOPATHING,			// 1
		ORDER_GOTO_NOPATHING_CONTACT,	// 2
		ORDER_GOTO_SMOOTHED,			// 3
		ORDER_GOTO_COLLISION,			// 4
		ORDER_GOTO_WAYPOINT,			// 5
		ORDER_GOTO_WAYPOINT_CONTACT,	// 6
		ORDER_GOTO,						// 7
		ORDER_RUN,						// 8
		ORDER_PATROL,					// 9
		ORDER_PATH_END_MARKER,			// 10
		ORDER_CONTACT_ACTION,					// 11
		ORDER_CONTACT_ACTION_NOPATHING,		// 12
		ORDER_PRODUCE,					// 13
		ORDER_START_CONSTRUCTION,		// 14
		ORDER_SET_RALLY_POINT,			// 15
		ORDER_SET_STANCE,				// 16
		ORDER_NOTIFY_REQUEST,			// 17
		ORDER_LAST						// 18
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

	// NMT_PLACE_OBJECT
	HEntity m_new_obj;

	// NMT_GOTO
	// NMT_FORMATION_GOTO
	// NMT_RUN
	// NMT_PATROL
	// NMT_ADD_WAYPOINT
	CVector2D m_target_location;

	// NMT_CONTACT_ACTION
	// NMT_FORMATION_CONTACT_ACTION
	// NMT_NOTIFY_REQUEST
	HEntity m_target_entity;
	int m_action;

	// NMT_PRODUCE, NMT_SET_STANCE
	CStrW m_name;

	// NMT_PRODUCE
	int m_produce_type;

	// NMT_CONTACT_ACTION
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
