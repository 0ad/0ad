#include "MessagePasser.h"

#include "ps/ThreadUtil.h"
#include <queue>

template <typename T> class MessagePasserImpl : public AtlasMessage::MessagePasser<T>
{
public:
	MessagePasserImpl();
	virtual void Add(T* msg);
	virtual T* Retrieve();
	virtual bool IsEmpty();

	void SetTrace(bool t);

private:
	CMutex m_Mutex;
	std::queue<T*> m_Queue;
	bool m_Trace;
};
