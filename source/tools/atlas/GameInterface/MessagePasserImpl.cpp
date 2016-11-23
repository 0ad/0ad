/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include <cstdio>

#include "MessagePasserImpl.h"
#include "Messages.h"

#include "lib/timer.h"
#include "lib/rand.h"
#include "lib/posix/posix_filesystem.h"

using namespace AtlasMessage;


MessagePasserImpl::MessagePasserImpl()
: m_Trace(false), m_Semaphore(NULL)
{
	int tries = 0;
	while (tries++ < 16) // some arbitrary cut-off point to avoid infinite loops
	{
		static char name[64];
		sprintf_s(name, ARRAY_SIZE(name), "/wfg-atlas-msgpass-%d-%d",
				(int)rand(1, 1000), (int)(time(0)%1000));
		sem_t* sem = sem_open(name, O_CREAT | O_EXCL, 0700, 0);

		// This cast should not be necessary, but apparently SEM_FAILED is not
		// a value of a pointer type
		if (sem == (sem_t*)SEM_FAILED || !sem)
		{
			int err = errno;
			if (err == EEXIST)
			{
				// Semaphore already exists - try another one
				continue;
			}
			// Otherwise, it's a probably-fatal error
			debug_warn(L"sem_open failed");
			break;
		}
		// Succeeded - use this semaphore
		m_Semaphore = sem;
		m_SemaphoreName = name;
		break;
	}

	if (! m_Semaphore)
	{
		debug_warn(L"Failed to create semaphore for Atlas - giving up");
		// We will probably crash later - maybe we could fall back on sem_init, if this
		// ever fails in practice
	}
}

MessagePasserImpl::~MessagePasserImpl()
{
	if (m_Semaphore)
	{
		// Clean up
		sem_close(m_Semaphore);
		sem_unlink(m_SemaphoreName.c_str());
	}
}

void MessagePasserImpl::Add(IMessage* msg)
{
	ENSURE(msg);
	ENSURE(msg->GetType() == IMessage::Message);

	if (m_Trace)
		debug_printf("%8.3f add message: %s\n", timer_Time(), msg->GetName());

	{
		CScopeLock lock(m_Mutex);
		m_Queue.push(msg);
	}
}

IMessage* MessagePasserImpl::Retrieve()
{
	// (It should be fairly easy to use a more efficient thread-safe queue,
	// since there's only one thread adding items and one thread consuming;
	// but it's not worthwhile yet.)

	IMessage* msg = NULL;

	{
		CScopeLock lock(m_Mutex);
		if (! m_Queue.empty())
		{
			msg = m_Queue.front();
			m_Queue.pop();
		}
	}

	if (m_Trace && msg)
		debug_printf("%8.3f retrieved message: %s\n", timer_Time(), msg->GetName());

	return msg;
}

void MessagePasserImpl::Query(QueryMessage* qry, void(* UNUSED(timeoutCallback) )())
{
	ENSURE(qry);
	ENSURE(qry->GetType() == IMessage::Query);

	if (m_Trace)
		debug_printf("%8.3f add query: %s\n", timer_Time(), qry->GetName());

	// Set the semaphore, so we can block until the query has been handled
	qry->m_Semaphore = static_cast<void*>(m_Semaphore);

	{
		CScopeLock lock(m_Mutex);
		m_Queue.push(qry);
	}

	// Wait until the query handler has handled the query and called sem_post:


	// The following code was necessary to avoid deadlock, but it still breaks
	// in some cases (e.g. when Atlas issues a query before its event loop starts
	// running) and doesn't seem to be the simplest possible solution.
	// So currently we're trying to not do anything like that at all, and
	// just stop the game making windows (which is what seems (from experience) to
	// deadlock things) by overriding ah_display_error. Hopefully it'll work like
	// that, and the redundant code below/elsewhere can be removed, but it's
	// left in here in case it needs to be reinserted in the future to make it
	// work.
	// (See http://www.wildfiregames.com/forum/index.php?s=&showtopic=10236&view=findpost&p=174617)

// 	// At least on Win32, it is necessary for the UI thread to run its event
// 	// loop to avoid deadlocking the system (particularly when the game
// 	// tries to show a dialog box); so timeoutCallback is called whenever we
// 	// think it's necessary for that to happen.
//
// #if OS_WIN
// 	// On Win32, use MsgWaitForMultipleObjects, which waits on the semaphore
// 	// but is also interrupted by incoming Windows-messages.
//	// while (0 != (err = sem_msgwait_np(psem)))
//
// 	while (0 != (err = sem_wait(psem)))
// #else
// 	// TODO: On non-Win32, I have no idea whether the same problem exists; but
// 	// it might do, so call the callback every few seconds just in case it helps.
// 	struct timespec abs_timeout;
// 	clock_gettime(CLOCK_REALTIME, &abs_timeout);
// 	abs_timeout.tv_sec += 2;
// 	while (0 != (err = sem_timedwait(psem, &abs_timeout)))
// #endif

	while (0 != sem_wait(m_Semaphore))
	{
		// If timed out, call callback and try again
// 		if (errno == ETIMEDOUT)
// 			timeoutCallback();
// 		else
		// Keep retrying while EINTR, but other errors are probably fatal
		if (errno != EINTR)
		{
			debug_warn(L"Semaphore wait failed");
			return; // (leaks the semaphore)
		}
	}

	// Clean up
	qry->m_Semaphore = NULL;
}

bool MessagePasserImpl::IsEmpty()
{
	CScopeLock lock(m_Mutex);
	return m_Queue.empty();
}

void MessagePasserImpl::SetTrace(bool t)
{
	m_Trace = t;
}
