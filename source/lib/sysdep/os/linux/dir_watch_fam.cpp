/* Copyright (c) 2012 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"

#include <string>

#include "lib/config2.h"
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/dir_watch.h"
#include "ps/CLogger.h"

#if !CONFIG2_FAM

// stub implementations

Status dir_watch_Add(const OsPath& UNUSED(path), PDirWatch& UNUSED(dirWatch))
{
	return INFO::OK;
}

Status dir_watch_Poll(DirWatchNotifications& UNUSED(notifications))
{
	return INFO::OK;
}

#else

#include <fam.h>

// FAMEvent is large (~4KB), so define a smaller structure to store events
struct NotificationEvent
{
	std::string filename;
	void *userdata;
	FAMCodes code;
};

// To avoid deadlocks and slow synchronous reads, it's necessary to use a
// separate thread for reading events from FAM.
// So we just spawn a thread to push events into this list, then swap it out
// when someone calls dir_watch_Poll.
// (We assume STL memory allocation is thread-safe.)
static std::vector<NotificationEvent> g_notifications;
static pthread_t g_event_loop_thread;

// Mutex must wrap all accesses of g_notifications
// while the event loop thread is running
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

// trool; -1 = init failed and all operations will be aborted silently.
// this is so that each dir_* call doesn't complain if the system's
// FAM is broken or unavailable.
static int initialized = 0;

static FAMConnection fc;

struct DirWatch
{
	DirWatch()
		: reqnum(-1)
	{
	}

	~DirWatch()
	{
		ENSURE(initialized > 0);

		FAMRequest req;
		req.reqnum = reqnum;
		FAMCancelMonitor(&fc, &req);
	}

	OsPath path;
	int reqnum;
};


// for atexit
static void fam_deinit()
{
	FAMClose(&fc);

	pthread_cancel(g_event_loop_thread);
	// NOTE: POSIX threads are (by default) only cancellable inside particular
	// functions (like 'select'), so this should safely terminate while it's
	// in select/FAMNextEvent/etc (and won't e.g. cancel while it's holding the
	// mutex)

	// Wait for the thread to finish
	pthread_join(g_event_loop_thread, NULL);
}

static void fam_event_loop_process_events()
{
	while(FAMPending(&fc) > 0)
	{
		FAMEvent e;
		if(FAMNextEvent(&fc, &e) < 0)
		{
			debug_printf(L"FAMNextEvent error");
			return;
		}

		NotificationEvent ne;
		ne.filename = e.filename;
		ne.userdata = e.userdata;
		ne.code = e.code;

		pthread_mutex_lock(&g_mutex);
		g_notifications.push_back(ne);
		pthread_mutex_unlock(&g_mutex);
	}
}

static void* fam_event_loop(void*)
{
	int famfd = FAMCONNECTION_GETFD(&fc);

	while(true)
	{
		fd_set fdrset;
		FD_ZERO(&fdrset);
		FD_SET(famfd, &fdrset);

		// Block with select until there's events waiting
		// (Mustn't just block inside FAMNextEvent since fam will deadlock)
		while(select(famfd+1, &fdrset, NULL, NULL, NULL) < 0)
		{
			if(errno == EINTR)
			{
				// interrupted - try again
				FD_ZERO(&fdrset);
				FD_SET(famfd, &fdrset);
			}
			else if(errno == EBADF)
			{
				// probably just lost the connection to FAM - kill the thread
				debug_printf(L"lost connection to FAM");
				return NULL;
			}
			else
			{
				// oops
				debug_printf(L"select error %d", errno);
				return NULL;
			}
		}

		if(FD_ISSET(famfd, &fdrset))
			fam_event_loop_process_events();
	}
}

Status dir_watch_Add(const OsPath& path, PDirWatch& dirWatch)
{
	// init already failed; don't try again or complain
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN

	if(!initialized)
	{
		if(FAMOpen2(&fc, "lib_res"))
		{
			initialized = -1;
			LOGERROR(L"Error initializing FAM; hotloading will be disabled");
			return ERR::FAIL;	// NOWARN
		}
		
		if (pthread_create(&g_event_loop_thread, NULL, &fam_event_loop, NULL))
		{
			initialized = -1;
			LOGERROR(L"Error creating FAM event loop thread; hotloading will be disabled");
			return ERR::FAIL;	// NOWARN
		}

		initialized = 1;
		atexit(fam_deinit);
	}

	PDirWatch tmpDirWatch(new DirWatch);

	// NOTE: It would be possible to use FAMNoExists iff we're building with Gamin
	// (not FAM), to avoid a load of boring notifications when we add a directory,
	// but it would only save tens of milliseconds of CPU time, so it's probably
	// not worthwhile

	FAMRequest req;
	if(FAMMonitorDirectory(&fc, OsString(path).c_str(), &req, tmpDirWatch.get()) < 0)
	{
		debug_warn(L"res_watch_dir failed!");
		WARN_RETURN(ERR::FAIL);	// no way of getting error code?
	}

	dirWatch.swap(tmpDirWatch);
	dirWatch->path = path;
	dirWatch->reqnum = req.reqnum;

	return INFO::OK;
}



Status dir_watch_Poll(DirWatchNotifications& notifications)
{
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN
	if(!initialized) // XXX Fix Atlas instead of suppressing the warning
		return ERR::FAIL; //WARN_RETURN(ERR::LOGIC);

	std::vector<NotificationEvent> polled_notifications;

	pthread_mutex_lock(&g_mutex);
	g_notifications.swap(polled_notifications);
	pthread_mutex_unlock(&g_mutex);

	for(size_t i = 0; i < polled_notifications.size(); ++i)
	{
		DirWatchNotification::EType type;
		switch(polled_notifications[i].code)
		{
		case FAMChanged:
			type = DirWatchNotification::Changed;
			break;
		case FAMCreated:
			type = DirWatchNotification::Created;
			break;
		case FAMDeleted:
			type = DirWatchNotification::Deleted;
			break;
		default:
			continue;
		}
		DirWatch* dirWatch = (DirWatch*)polled_notifications[i].userdata;
		OsPath pathname = dirWatch->path / polled_notifications[i].filename;
		notifications.push_back(DirWatchNotification(pathname, type));
	}

	// nothing new; try again later
	return INFO::OK;
}

#endif	// CONFIG2_FAM
