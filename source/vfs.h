
extern int vfs_add_search_path(const char* path);
extern int vfs_remove_first_path();

extern Handle vfs_open(const char* fn);
extern int vfs_close(Handle h);
extern int vfs_read(Handle h, void*& p, size_t& size, size_t ofs = 0);

