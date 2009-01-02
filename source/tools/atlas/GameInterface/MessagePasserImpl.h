#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include "ps/CStr.h"
#include <queue>

class MessagePasserImpl : public AtlasMessage::MessagePasser, noncopyable
{
public:
	MessagePasserImpl();
	~MessagePasserImpl();
	virtual void Add(AtlasMessage::IMessage* msg);
	virtual AtlasMessage::IMessage* Retrieve();
	virtual void Query(AtlasMessage::QueryMessage* qry, void(*timeoutCallback)());

	bool IsEmpty();

	void SetTrace(bool t);

private:
	CMutex m_Mutex;
	CStr m_SemaphoreName;
	sem_t* m_Semaphore;
	std::queue<AtlasMessage::IMessage*> m_Queue;
	bool m_Trace;
};
