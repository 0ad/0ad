
// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
extern int dir_add_watch(const char* path, intptr_t* watch);

extern int dir_cancel_watch(intptr_t watch);

extern int dir_get_changed_file(char* fn);
