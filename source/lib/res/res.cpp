#include "precompiled.h"

#include "res.h"

#ifdef _WIN32
#include "sysdep/win/wfam.h"
#else
#include <fam.h>
#endif


int res_reload(const char* const fn)
{
	return h_reload(fn);
}


static FAMConnection fc;
static bool initialized;


int res_mount(const char* const mount_point, const char* const name, const uint pri)
{
	if(!initialized)
	{
		CHECK_ERR(FAMOpen2(&fc, "lib_res"));
		atexit2(FAMClose, (uintptr_t)&fc);
		initialized = true;
	}

	CHECK_ERR(vfs_mount(mount_point, name, pri));

	// if is directory
	// get full path
	// convert to native

	static FAMRequest req;
	FAMMonitorDirectory(&fc, "d:\\projects\\0ad\\cvs\\binaries\\data\\mods\\official\\", &req, 0);
//FAMCancelMonitor(&fc, &req);
	// add request somewhere - have to be able to cancel watch

	return 0;
}


int res_reload_changed_files()
{
	if(!initialized)
		return -1;

	FAMEvent e;
	while(FAMPending(&fc) > 0)
		if(FAMNextEvent(&fc, &e) == 0)
		{
			const char* sys_fn = e.filename;
		}

	return 0;
}


// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.
