/* Copyright (C) 2009 Wildfire Games.
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

#include <string>

#include "lib/sysdep/sysdep.h"
#include "lib/path_util.h"
#include "lib/wchar.h"
#include "lib/sysdep/dir_watch.h"
#include "ps/CLogger.h"

#include <fam.h>

struct NotificationEvent
{
	char* filename;
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
		debug_assert(initialized > 0);

		FAMRequest req;
		req.reqnum = reqnum;
		FAMCancelMonitor(&fc, &req);
	}

	fs::wpath path;
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
		ne.filename = strndup(e.filename, PATH_MAX);
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

	while (true)
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
				debug_printf(L"select error %d", errno); // TODO: be sure debug_printf is threadsafe
				return NULL;
			}
		}

		if(FD_ISSET(famfd, &fdrset))
			fam_event_loop_process_events();
	}
}

LibError dir_watch_Add(const fs::wpath& path, PDirWatch& dirWatch)
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

	const fs::path path_c = path_from_wpath(path);
	FAMRequest req;
	if(FAMMonitorDirectory(&fc, path_c.string().c_str(), &req, tmpDirWatch.get()) < 0)
	{
		debug_warn(L"res_watch_dir failed!");
		WARN_RETURN(ERR::FAIL);	// no way of getting error code?
	}

	dirWatch.swap(tmpDirWatch);
	dirWatch->path = path;
	dirWatch->reqnum = req.reqnum;

	return INFO::OK;
}



LibError dir_watch_Poll(DirWatchNotifications& notifications)
{
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN
	if(!initialized) // XXX Fix Atlas instead of supressing the warning
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
		fs::wpath pathname = dirWatch->path/wstring_from_utf8(polled_notifications[i].filename);
		notifications.push_back(DirWatchNotification(pathname, type));
		free(polled_notifications[i].filename);
	}

	// nothing new; try again later
	return INFO::OK;
}
