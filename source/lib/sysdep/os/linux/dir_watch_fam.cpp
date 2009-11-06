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

#include <map>
#include <string>

#include "lib/sysdep/sysdep.h"
#include "lib/path_util.h"
#include "lib/wchar.h"
#include "lib/sysdep/dir_watch.h"
#include "ps/CLogger.h"

#include <fam.h>


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
		debug_assert(initialized > 0)

		FAMRequest req;
		req.reqnum = reqnum;
		FAMCancelMonitor(&fc, &req);
	}

	fs::wpath path;
	int reqnum;
};


static std::map<intptr_t, std::string> dirs;

// for atexit
static void fam_deinit()
{
	FAMClose(&fc);
}

LibError dir_watch_Add(const fs::wpath& path, PDirWatch& dirWatch)
{
	// init already failed; don't try again or complain
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN

	if(!initialized)
	{
		// success
		if(FAMOpen2(&fc, "lib_res") == 0)
		{
			initialized = 1;
			atexit(fam_deinit);
		}
		else
		{
			initialized = -1;
			LOG(CLogger::Error, "", "Error initializing FAM; hotloading will be disabled");
			return ERR::FAIL;	// NOWARN
		}
	}

	PDirWatch tmpDirWatch(new DirWatch);

	const fs::path path_c = path_from_wpath(path);
	FAMRequest req;
	if(FAMMonitorDirectory(&fc, path_c.string().c_str(), &req, tmpDirWatch.get()) < 0)
	{
		debug_warn("res_watch_dir failed!");
		WARN_RETURN(ERR::FAIL);	// no way of getting error code?
	}

	// Monitoring a directory causes a FAMExists event per file in that directory, then a
	// FAMEndExist. We have to wait for these events, else the FAM server's write buffer fills
	// up and we get deadlocked when trying to open another directory. (That happens only with
	// the original fam, and seems fixed in gamin, but we want to work with both.)
	FAMEvent e;
	do {
		if(FAMNextEvent(&fc, &e) < 0)
		{
			// Oops, failed - rather than getting stuck waiting forever for a
			// FAMEndExist event that may never come, just give up and return now.
			debug_warn("FAMNextEvent failed");
			return ERR::FAIL;
		}
		// (We might be missing some real events other than the FAMExists ones, if
		// they happen to be generated right now. That's not a critical problem, so
		// just ignore it.)
	}
	while (e.code != FAMEndExist);

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

	FAMEvent e;
	while(FAMPending(&fc) > 0)
	{
		if(FAMNextEvent(&fc, &e) >= 0)
		{
			DirWatchNotification::EType type;
			switch(e.code)
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
			DirWatch* dirWatch = (DirWatch*)e.userdata;
			fs::wpath pathname = dirWatch->path/wstring_from_string(e.filename);
			notifications.push_back(DirWatchNotification(pathname, type));
		}
	}

	// nothing new; try again later
	return INFO::OK;
}
