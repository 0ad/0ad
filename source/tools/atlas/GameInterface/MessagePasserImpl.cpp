#include "precompiled.h"

#include "MessagePasserImpl.h"
#include "Messages.h"

#include "lib/timer.h"

using namespace AtlasMessage;


MessagePasserImpl::MessagePasserImpl()
: m_Trace(false)
{
}

void MessagePasserImpl::Add(IMessage* msg)
{
	debug_assert(msg);
	debug_assert(msg->GetType() == IMessage::Message);

	if (m_Trace)
		debug_printf("%8.3f add message: %s\n", get_time(), msg->GetName());

	m_Mutex.Lock();
	m_Queue.push(msg);
	m_Mutex.Unlock();
}

IMessage* MessagePasserImpl::Retrieve()
{
	// (It should be fairly easy to use a more efficient thread-safe queue,
	// since there's only one thread adding items and one thread consuming;
	// but it's not worthwhile yet.)

	m_Mutex.Lock();

	IMessage* msg = NULL;
	if (! m_Queue.empty())
	{
		msg = m_Queue.front();
		m_Queue.pop();
	}

	m_Mutex.Unlock();

//	if (m_Trace && msg) debug_printf("%8.3f retrieved message: %s\n", get_time(), msg->GetType());

	return msg;
}

void MessagePasserImpl::Query(QueryMessage* qry, void(*timeoutCallback)())
{
	debug_assert(qry);
	debug_assert(qry->GetType() == IMessage::Query);

	if (m_Trace)
		debug_printf("%8.3f add query: %s\n", get_time(), qry->GetName());

	// Initialise a semaphore, so we can block until the query has been handled
	int err;
	sem_t sem;
	err = sem_init(&sem, 0, 0);
	if (err != 0)
	{
		// Probably-fatal error
		debug_warn("sem_init failed");
		return;
	}
	qry->m_Semaphore = (void*)&sem;

	m_Mutex.Lock();
	m_Queue.push(qry);
	m_Mutex.Unlock();

	// Wait until the query handler has handled the query and called sem_post:

	// At least on Win32, it is necessary for the UI thread to run its event
	// loop to avoid deadlocking the system (particularly when the game
	// tries to show a dialog box); so timeoutCallback is called whenever we
	// think it's necessary for that to happen.

#if OS_WIN
	// On Win32, use MsgWaitForMultipleObjects, which waits on the semaphore
	// but is also interrupted by incoming Windows-messages.
	while (0 != (err = sem_msgwait_np(&sem)))
#else
	// TODO: On non-Win32, I have no idea whether the same problem exists; but
	// it might do, so call the callback every few seconds just in case it helps.
	struct timespec abs_timeout;
	clock_gettime(CLOCK_REALTIME, &abs_timeout);
	abs_timeout.tv_sec += 2;
	while (0 != (err = sem_timedwait(&sem, &abs_timeout)))
#endif
	{
		// If timed out, call callback and try again
		if (errno == ETIMEDOUT)
			timeoutCallback();
		// Keep retrying while EINTR, but other errors are probably fatal
		else if (errno != EINTR)
		{
			debug_warn("Semaphore wait failed");
			return; // (leak the semaphore)
		}
	}

	// Clean up
	qry->m_Semaphore = NULL;
	sem_destroy(&sem);
}

bool MessagePasserImpl::IsEmpty()
{
	m_Mutex.Lock();
	bool empty = m_Queue.empty();
	m_Mutex.Unlock();
	return empty;
}

void MessagePasserImpl::SetTrace(bool t)
{
	m_Trace = t;
}
