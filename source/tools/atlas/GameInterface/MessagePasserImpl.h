#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include <queue>

template <typename T> class MessagePasserImpl : public AtlasMessage::MessagePasser<T>
{
public:
	virtual void Add(T* msg);
	virtual T* Retrieve();

	virtual void Query(T&);
	virtual void QueryDone();

private:
	CMutex m_Mutex;
	std::queue<T*> m_Queue;
};
