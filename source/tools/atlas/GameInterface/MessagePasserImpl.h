#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include <queue>

class MessagePasserImpl : public AtlasMessage::MessagePasser
{
public:
	MessagePasserImpl();
	virtual void Add(AtlasMessage::IMessage* msg);
	virtual AtlasMessage::IMessage* Retrieve();
	virtual void Query(AtlasMessage::QueryMessage* qry, void(*timeoutCallback)());

	bool IsEmpty();

	void SetTrace(bool t);

private:
	CMutex m_Mutex;
	std::queue<AtlasMessage::IMessage*> m_Queue;
	bool m_Trace;
};
