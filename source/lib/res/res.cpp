#include "precompiled.h"

#include "res.h"

int res_reload(const char* const fn)
{
	return h_reload(fn);
}





// purpose of this routine (intended to be called once a frame):
// file notification may come at any time. by forcing the reloads
// to take place here, we don't require everything to be thread-safe.