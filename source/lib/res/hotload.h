
extern int res_reload(const char* fn);


// the following functions must be called from the same thread!
// (wdir_watch limitation)


extern int res_watch_dir(const char* path, intptr_t* watch);

extern int res_cancel_watch(const intptr_t watch);

extern int res_reload_changed_files();
