#include "precompiled.h"

#include <map>
#include <string>

#include "lib.h"
#include "res/file/file.h"

#include <fam.h>

static FAMConnection fc;
static bool initialized = false;

static std::map<intptr_t, std::string> dirs;

// for atexit
static void fam_deinit()
{
	FAMClose(&fc);
}

LibError dir_add_watch(const char* const n_full_path, intptr_t* const watch)
{
	if(!initialized)
	{
		CHECK_ERR(FAMOpen2(&fc, "lib_res"));
		initialized = true;
		atexit(fam_deinit);
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
	if(!initialized)
		WARN_RETURN(ERR_LOGIC);

	FAMRequest req;
	req.reqnum = (int)watch;
	RETURN_ERR(FAMCancelMonitor(&fc, &req));
	return ERR_OK;
}

int dir_get_changed_file(char* fn)
{
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

	return ERR_AGAIN;	// NOWARN
}
