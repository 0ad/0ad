#ifndef MESSAGEPASSER_H__
#define MESSAGEPASSER_H__

namespace AtlasMessage
{
	
struct IMessage;
struct QueryMessage;

class MessagePasser
{
public:
	virtual void Add(IMessage*)=0; // takes ownership of IMessage object
	virtual IMessage* Retrieve()=0;
	virtual void Query(QueryMessage*)=0; // blocks; caller retains ownership
};

extern MessagePasser* g_MessagePasser;

#define POST_MESSAGE(type) AtlasMessage::g_MessagePasser->Add(new AtlasMessage::m##type)

}

#endif // MESSAGEPASSER_H__
