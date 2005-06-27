#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include <queue>

class MessagePasserImpl : public AtlasMessage::MessagePasser
{
public:
	virtual void Add(AtlasMessage::IMessage* msg);
	virtual AtlasMessage::IMessage* Retrieve();

	virtual void Query(AtlasMessage::IMessage&);
	virtual void QueryDone();

private:
	CMutex m_Mutex;
	std::queue<AtlasMessage::IMessage*> m_Queue;
};
 