#ifndef MESSAGEPASSER_H__
#define MESSAGEPASSER_H__

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

#endif // MESSAGEPASSER_H__
