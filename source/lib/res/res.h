#include "res/h_mgr.h"
#include "res/vfs.h"
#include "res/tex.h"
#include "res/mem.h"
#include "res/font.h"


extern int res_reload(const char* fn);

extern int res_watch_dir(const char* dir);

extern int res_reload_changed_files();