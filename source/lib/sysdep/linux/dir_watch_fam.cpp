#include "precompiled.h"

#include <map>
#include <string>

#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/dir_watch.h"
#include "ps/CLogger.h"

#include <fam.h>

static FAMConnection fc;

// trool; -1 = init failed and all operations will be aborted silently.
// this is so that each dir_* call doesn't complain if the system's
// FAM is broken or unavailable.
static int initialized = 0;

static std::map<intptr_t, std::string> dirs;

// for atexit
static void fam_deinit()
{
	FAMClose(&fc);
}

LibError dir_add_watch(const char* const n_full_path, intptr_t* const watch)
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

	FAMRequest req;
	if(FAMMonitorDirectory(&fc, n_full_path, &req, (void*)0) < 0)
	{
		*watch = -1;
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

	*watch = (intptr_t)req.reqnum;
	dirs[*watch] = n_full_path;
	return INFO::OK;
}


LibError dir_cancel_watch(const intptr_t watch)
{
	if(initialized == -1)
		return ERR::FAIL;	// NOWARN
	if(!initialized)
		WARN_RETURN(ERR::LOGIC);

	FAMRequest req;
	req.reqnum = (int)watch;
	RETURN_ERR(FAMCancelMonitor(&fc, &req));
	return INFO::OK;
}

LibError dir_get_changed_file(char* fn)
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
			if (e.code == FAMChanged || e.code == FAMCreated || e.code == FAMDeleted)
			{
				char n_path[PATH_MAX];
				const char* dir = dirs[e.fr.reqnum].c_str();
				snprintf(n_path, PATH_MAX, "%s%c%s", dir, SYS_DIR_SEP, e.filename);
				return ERR::AGAIN;
			}
		}
	}

	// just nothing new; try again later
	return ERR::AGAIN;	// NOWARN
}
