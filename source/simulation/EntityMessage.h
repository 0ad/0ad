// EntityMessage.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity message structure.
//
// Usage: Currently, does not support any data to be included with messages.
//		  Message types are currently: EMSG_TICK: Unused.
//									   EMSG_INIT: When a new entity is instantiated.
//												  At map loading, do not issue this message immediately
//												  for each entity as it is loaded; instead, wait for all
//												  entities to be created, then issue this message to all
//												  of them simultaneously.
//									   EMSG_ORDER:To push a message into the entity's order queue

#ifndef MESSAGING_INCLUDED
#define MESSAGING_INCLUDED

#include "EntityOrders.h"

struct CMessage
{
	enum EMessageType
	{
		EMSG_TICK,
		EMSG_INIT,
		EMSG_ORDER,
	} type;
	CMessage( EMessageType _type )
	{
		type = _type;
	}
};

struct CMessageOrder : public CMessage
{
	CMessageOrder( CEntityOrder& _order, bool _queue = false ) : CMessage( EMSG_ORDER ), order( _order ), queue( _queue ) {}
	CEntityOrder order;
	bool queue;
};

#endif
