#include "precompiled.h"

#include <map>
#include <string>

#include "lib/lib.h"
#include "lib/res/file/file.h"

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
		return ERR_FAIL;	// NOWARN

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
			DISPLAY_ERROR(L"Error initializing FAM; hotloading will be disabled");
			return ERR_FAIL;	// NOWARN
		}
	}

	FAMRequest req;
	if(FAMMonitorDirectory(&fc, n_full_path, &req, (void*)0) < 0)
	{
		*watch = -1;
		debug_warn("res_watch_dir failed!");
		WARN_RETURN(ERR_FAIL);	// no way of getting error code?
	}

	*watch = (intptr_t)req.reqnum;
	dirs[*watch] = n_full_path;
	return ERR_OK;
}


LibError dir_cancel_watch(const intptr_t watch)
{
	if(initialized == -1)
		return ERR_FAIL;	// NOWARN
	if(!initialized)
		WARN_RETURN(ERR_LOGIC);

	FAMRequest req;
	req.reqnum = (int)watch;
	RETURN_ERR(FAMCancelMonitor(&fc, &req));
	return ERR_OK;
}

int dir_get_changed_file(char* fn)
{
	if(initialized == -1)
		return ERR_FAIL;	// NOWARN
	if(!initialized)
		WARN_RETURN(ERR_LOGIC);

	FAMEvent e;
	while(FAMPending(&fc) > 0)
	{
		if(FAMNextEvent(&fc, &e) >= 0)
		{
			if (e.code == FAMChanged || e.code == FAMCreated || e.code == FAMDeleted)
			{
				char n_path[PATH_MAX];
				const char* dir = dirs[e.fr.reqnum].c_str();
				snprintf(n_path, PATH_MAX, "%s%c%s", dir, DIR_SEP, e.filename);
				RETURN_ERR(file_make_portable_path(n_path, fn));
				return ERR_OK;
			}
		}
	}

	// just nothing new; try again later
	return ERR_AGAIN;	// NOWARN
}
