// EntityMessage.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Entity message structure.
//
// Usage: Message types are currently: EMSG_TICK: Sent once per sim frame.
//									   EMSG_INIT: When a new entity is instantiated.
//												  At map loading, do not issue this message immediately
//												  for each entity as it is loaded; instead, wait for all
//												  entities to be created, then issue this message to all
//												  of them simultaneously.
//									   EMSG_ORDER:To push a message into the entity's order queue

/*
#ifndef MESSAGING_INCLUDED
#define MESSAGING_INCLUDED

#include "EntityOrders.h"
#include "EntitySupport.h"

struct CMessage
{
	enum EMessageType
	{
		EMSG_TICK,
		EMSG_INIT,
		EMSG_ORDER,
		EMSG_DAMAGE
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

struct CMessageDamage : public CMessage
{
	CMessageDamage( HEntity _inflictor, CDamageType _damage ) : CMessage( EMSG_DAMAGE ), inflictor( inflictor ), damage( damage ) {}
	HEntity inflictor;
	CDamageType damage;
}

#endif
*/