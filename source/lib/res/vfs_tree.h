struct TMountPoint;

class TDir;

struct TFile
{
	// required:
	const TMountPoint* mount_point;
	// allocated and owned by caller (mount code)

	time_t mtime;
	off_t size;

	// these can also be extracted from TMountPoint,
	// but better cache coherency when accessing them here.
	u32 pri : 16;
	u32 in_archive : 1;

	// note: this is basically the constructor (C++ can't call it directly
	// since this object is stored in a union)
	void init()
	{
		mount_point = 0;
		mtime = 0;
		size = 0;
	}
};

// keep in sync with vfs.h vfsDirEnt!
struct TDirent
{
	const char* name;
	off_t size;
	time_t mtime;
};

enum TreeLookupFlags
{
	LF_CREATE_MISSING = 1,
	LF_START_DIR      = 2
};



extern void tree_init();
extern void tree_clear();
extern void tree_display();

extern TFile* tree_add_file(TDir* dir, const char* name);
extern TDir* tree_add_dir(TDir* parent, const char* path, const TMountPoint*);


// starting at VFS root, traverse <path> and pass back information
// for its last directory component.
//
// if <flags> & LF_CREATE_MISSING, all missing subdirectory components are
//   added to the VFS.
// if <flags> & LF_START_DIR, traversal starts at *pdir
//   (used when looking up paths relative to a mount point).
// if <exact_path> != 0, it receives a copy of <path> with the exact
//   case of each component as returned by the OS (useful for calling
//   external case-sensitive code). must hold at least VFS_MAX_PATH chars.
//
// <path> can be to a file or dir (in which case it must end in '/',
// to make sure the last component is treated as a directory).
//
// return 0 on success, or a negative error code
// (in which case output params are undefined).
extern int tree_lookup_dir(const char* path, TDir** pdir, uint flags = 0, char* exact_path = 0);


// pass back file information for <path> (relative to VFS root).
//
// if <flags> & LF_CREATE_MISSING, the file is added to VFS unless
//   a higher-priority file of the same name already exists
//   (used by VFile_reload when opening for writing).
// if <exact_path> != 0, it receives a copy of <path> with the exact
//   case of each component as returned by the OS (useful for calling
//   external case-sensitive code). must hold at least VFS_MAX_PATH chars.
//
// return 0 on success, or a negative error code
// (in which case output params are undefined).
extern int tree_lookup(const char* path, TFile** pfile, uint flags = 0, char* exact_path = 0);

extern int tree_open_dir(const char* path_slash, void** latch);
extern int tree_next_dirent(void* latch_, const char* filter, TDirent* dirent);
extern int tree_close_dir(void* latch_);
