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


int res_watch_dir(const char* const dir)
{
	if(!initialized)
	{
		CHECK_ERR(FAMOpen2(&fc, "lib_res"));
		atexit2(FAMClose, (uintptr_t)&fc);
		initialized = true;
	}

	//

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
