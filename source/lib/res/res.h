#include "res/h_mgr.h"
#include "res/vfs.h"
#include "res/mem.h"


extern int res_reload(const char* fn);


// the following functions must be called from the same thread!
// (wfam limitation)


extern int res_watch_dir(const char* const path, intptr_t* const watch);

extern int res_cancel_watch(const intptr_t watch);

extern int res_reload_changed_files();
