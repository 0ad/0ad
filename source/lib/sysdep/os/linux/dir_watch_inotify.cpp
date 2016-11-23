/* Copyright (c) 2015 Wildfire Games
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

#include "lib/sysdep/dir_watch.h"
#include "lib/sysdep/sysdep.h"
#include "ps/CLogger.h"

#include <map>
#include <string>
#include <sys/inotify.h>


struct NotificationEvent
{
	std::string filename;
	uint32_t code;
	int wd;
};

// To avoid deadlocks and slow synchronous reads, it's necessary to use a
// separate thread for reading events from inotify.
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
// inotify is broken or unavailable.
static int initialized = 0;

// Inotify file descriptor
static int inotifyfd;

// With inotify, using a map seems to be a good alternative to FAM's userdata
typedef std::map<int, PDirWatch> DirWatchMap;
static DirWatchMap g_paths;

struct DirWatch
{
	DirWatch()
		: reqnum(-1)
	{
	}

	~DirWatch()
	{
		ENSURE(initialized > 0);
		inotify_rm_watch(inotifyfd, reqnum);
	}

	OsPath path;
	int reqnum;
};

// for atexit
static void inotify_deinit()
{
	close(inotifyfd);

#ifdef __BIONIC__
	#warning TODO: pthread_cancel not supported on Bionic
#else
	pthread_cancel(g_event_loop_thread);
#endif
	// NOTE: POSIX threads are (by default) only cancellable inside particular
	// functions (like 'select'), so this should safely terminate while it's
	// in select/etc (and won't e.g. cancel while it's holding the
	// mutex)

	// Wait for the thread to finish
	pthread_join(g_event_loop_thread, NULL);
}

static void inotify_event_loop_process_events()
{
	// Buffer for reading the events.
	// Need to be careful about overflow here.
	char buffer[65535] = {0};

	// Event iterator
	ssize_t buffer_i = 0;

	// Total size of all the events
	ssize_t r;

	// Size & struct for the current event
	size_t event_size;
	struct inotify_event *pevent;

	r = read(inotifyfd, buffer, 65535);
	if(r <= 0)
		return;

	while(buffer_i < r)
	{
		NotificationEvent ne;
		pevent = (struct inotify_event *) &buffer[buffer_i];

		event_size = offsetof(struct inotify_event, name) + pevent->len;
		ne.wd = pevent->wd;
		ne.filename = pevent->name;
		ne.code = pevent->mask;

 		pthread_mutex_lock(&g_mutex);
 		g_notifications.push_back(ne);
 		pthread_mutex_unlock(&g_mutex);

		buffer_i += event_size;
	}
}

static void* inotify_event_loop(void*)
{
	while(true)
	{
		fd_set fdrset;
		FD_ZERO(&fdrset);
		FD_SET(inotifyfd, &fdrset);
		errno = 0;
		// Block with select until there's events waiting
		while(select(inotifyfd+1, &fdrset, NULL, NULL, NULL) < 0)
		{
			if(errno == EINTR)
			{
				// interrupted - try again
				FD_ZERO(&fdrset);
				FD_SET(inotifyfd, &fdrset);
			}
			else if(errno == EBADF)
			{
				// probably just lost the connection to inotify - kill the thread
				debug_printf("inotify_event_loop: Invalid file descriptor inotifyfd=%d\n", inotifyfd);
				return NULL;
			}
			else
			{
				// oops
				debug_printf("inotify_event_loop: select error errno=%d\n", errno);
				return NULL;
			}
			errno = 0;
		}
		if(FD_ISSET(inotifyfd, &fdrset))
			inotify_event_loop_process_events();
	}
}

Status dir_watch_Add(const OsPath& path, PDirWatch& dirWatch)
{
	char resolved[PATH_MAX + 1];

	// init already failed; don't try again or complain
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN

	if(!initialized)
	{
		errno = 0;
		if((inotifyfd = inotify_init()) < 0)
		{
			// Check for error ?
			int err = errno;
			initialized = -1;
			LOGERROR("Error initializing inotify file descriptor; hotloading will be disabled, errno=%d", err);
			errno = err;
			return StatusFromErrno();	// NOWARN
		}

		errno = 0;
		int ret = pthread_create(&g_event_loop_thread, NULL, &inotify_event_loop, NULL);
		if (ret != 0)
		{
			initialized = -1;
			LOGERROR("Error creating inotify event loop thread; hotloading will be disabled, err=%d", ret);
			errno = ret;
			return StatusFromErrno();	// NOWARN
		}

		initialized = 1;
		atexit(inotify_deinit);
	}

	PDirWatch tmpDirWatch(new DirWatch);
	errno = 0;
	int wd = inotify_add_watch(inotifyfd, realpath(OsString(path).c_str(), resolved), IN_CREATE | IN_DELETE | IN_CLOSE_WRITE);
	if (wd < 0)
		WARN_RETURN(StatusFromErrno());

	dirWatch.swap(tmpDirWatch);
	dirWatch->path = path;
	dirWatch->reqnum = wd;
	g_paths.insert(std::make_pair(wd, dirWatch));

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
		// TODO: code is actually a bitmask, so this is slightly incorrect
		switch(polled_notifications[i].code)
		{
		case IN_CLOSE_WRITE:
			type = DirWatchNotification::Changed;
			break;
		case IN_CREATE:
			type = DirWatchNotification::Created;
			break;
		case IN_DELETE:
			type = DirWatchNotification::Deleted;
			break;
		default:
			continue;
		}

		DirWatchMap::iterator it = g_paths.find(polled_notifications[i].wd);
		if(it != g_paths.end())
		{
			OsPath filename = Path(OsString(it->second->path).append(polled_notifications[i].filename));
			notifications.push_back(DirWatchNotification(filename, type));
		}
		else
		{
			debug_printf("dir_watch_Poll: Notification with invalid watch descriptor wd=%d\n", polled_notifications[i].wd);
		}
	}

	// nothing new; try again later
	return INFO::OK;
}

