// EntityMessage.h
//
// Last modified: 22 May 04, Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
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

#ifndef MESSAGING_INCLUDED
#define MESSAGING_INCLUDED

struct CMessage
{
	enum EMessageType
	{
		EMSG_TICK,
		EMSG_INIT
	} type;
	CMessage( EMessageType _type )
	{
		type = _type;
	}
};

#endif
