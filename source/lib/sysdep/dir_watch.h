
// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
extern int dir_add_watch(const char* const path, intptr_t* const watch);

extern int dir_cancel_watch(const intptr_t watch);

extern int dir_get_changed_file(char* fn);
