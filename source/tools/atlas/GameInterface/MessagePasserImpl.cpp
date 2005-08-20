#include "precompiled.h"

#include "MessagePasserImpl.h"

#define MESSAGE_TRACE 0

#if MESSAGE_TRACE
#include "Messages.h" // for mCommand implementation
#endif

using namespace AtlasMessage;


template<typename T> void MessagePasserImpl<T>::Add(T* msg)
{
	debug_assert(msg);

#if MESSAGE_TRACE
	debug_printf("Add %s\n", msg->GetType());
#endif
	m_Mutex.Lock();

	m_Queue.push(msg);

	m_Mutex.Unlock();
}

template <typename T> T* MessagePasserImpl<T>::Retrieve()
{
	// (It should be fairly easy to use a more efficient thread-safe queue,
	// since there's only one thread adding items and one thread consuming;
	// but it's not worthwhile yet.)

	m_Mutex.Lock();

	T* msg = NULL;
	if (! m_Queue.empty())
	{
		msg = m_Queue.front();
		m_Queue.pop();
	}

	m_Mutex.Unlock();

#if MESSAGE_TRACE
	if (msg) debug_printf("Retrieved %s\n", msg->GetType());
#endif

	return msg;
}

template <typename T> void MessagePasserImpl<T>::Query(T&)
{
}

template <typename T> void MessagePasserImpl<T>::QueryDone()
{
}

MessagePasser<mCommand>* g_MessagePasser_Command = NULL;
MessagePasser<mInput>*   g_MessagePasser_Input = NULL;

// Explicit instantiation:
template MessagePasserImpl<mCommand>;
template MessagePasserImpl<mInput>;
