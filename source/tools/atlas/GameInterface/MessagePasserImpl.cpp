#include "precompiled.h"

#include "MessagePasserImpl.h"
#include "Messages.h"

#include "lib/timer.h"

using namespace AtlasMessage;


template<typename T> MessagePasserImpl<T>::MessagePasserImpl()
: m_Trace(false)
{
}

template<typename T> void MessagePasserImpl<T>::Add(T* msg)
{
	debug_assert(msg);

	if (m_Trace)
		debug_printf("%8.3f add message: %s\n", get_time(), msg->GetType());

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

//	if (m_Trace && msg) debug_printf("%8.3f retrieved message: %s\n", get_time(), msg->GetType());

	return msg;
}

template <typename T> bool MessagePasserImpl<T>::IsEmpty()
{
	m_Mutex.Lock();
	bool empty = m_Queue.empty();
	m_Mutex.Unlock();
	return empty;
}

template <typename T> void MessagePasserImpl<T>::SetTrace(bool t)
{
	m_Trace = t;
}

// Explicit instantiation:
template MessagePasserImpl<mCommand>;
template MessagePasserImpl<mInput>;
