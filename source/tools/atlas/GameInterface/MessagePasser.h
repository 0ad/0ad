#ifndef INCLUDED_MESSAGEPASSER
#define INCLUDED_MESSAGEPASSER

#include "SharedMemory.h"

namespace AtlasMessage
{
	
struct IMessage;
struct QueryMessage;

class MessagePasser
{
public:
	virtual void Add(IMessage*)=0;
		// takes ownership of IMessage object

	virtual IMessage* Retrieve()=0;

	virtual void Query(QueryMessage*, void(*timeoutCallback)())=0;
		// blocks; caller retains ownership of QueryMessage object
};

extern MessagePasser* g_MessagePasser;

#define POST_MESSAGE(type, data) AtlasMessage::g_MessagePasser->Add(SHAREABLE_NEW(AtlasMessage::m##type, data))

}

#endif // INCLUDED_MESSAGEPASSER
