/*
Event.h

Game event (scheduled messages) definitions

Mark Thompson (markt@0ad.wildfiregames.com)

Last modified: 19 January 2004 (Mark Thompson)

--Overview--

Contains the class definitions for serializable event messages.
--Usage--

TODO

--Examples--

TODO

--More info--

TDD at http://forums.wildfiregames.com/0ad

*/

#ifndef EVENT_INCLUDED
#define EVENT_INCLUDED

#include "assert.h"
#include "stdio.h"
#include "Network/Serialization.h"
#include "Entity.h"
#include "Network/NetMessage.h"

class CEvent : public CNetMessage
{
public:
	virtual uint GetSerializedLength() = 0;
	virtual u8* Serialize( u8* buffer ) = 0;
};


#endif // EVENT_INCLUDED

	
