#include "res/h_mgr.h"
#include "res/vfs.h"
#include "res/tex.h"
#include "res/mem.h"
#include "res/font.h"


extern int res_reload(const char* fn);


// the following functions must be called from the same thread!
// (wfam limitation)

extern int res_mount(const char* mount_point, const char* name, uint pri);

extern int res_reload_changed_files();