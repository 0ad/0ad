struct VFILE
{
	int fd;

	Handle hz;	// position within archive (ZFILE)
	Handle ha;	// archive (ZARCHIVE)
};

#define VFS_MAX_PATH 40

extern int vfs_set_root(const char* argv0, const char* root);
extern int vfs_mount(const char* path);
extern int vfs_umount(const char* path);

extern Handle vfs_map(const char* fn);	// actual path in real FS

extern Handle vfs_open(const char* fn);
extern int vfs_close(Handle h);

// size: in  - maximum byte count
//       out - bytes actually read
extern int vfs_read(Handle h, void*& p, size_t& size, size_t ofs = 0);

extern u32 vfs_start_read(Handle hf, size_t& ofs, void** buf = 0);
extern int vfs_finish_read(u32 slot, void*& p, size_t& size);