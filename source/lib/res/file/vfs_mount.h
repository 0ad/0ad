#ifndef VFS_MOUNT_H__
#define VFS_MOUNT_H__

struct Mount;	// must come before vfs_tree.h

#include "file.h"
#include "zip.h"
#include "vfs_tree.h"

extern void mount_init();
extern void mount_shutdown();


// If it was possible to forward-declare enums in gcc, this one wouldn't be in
// the header. Don't use.
enum MountType
{
	// the relative ordering of values expresses efficiency of the sources
	// (e.g. archives are faster than loose files). mount_should_replace
	// makes use of this.

	MT_NONE    = 0,
	MT_FILE    = 1,
	MT_ARCHIVE = 2
};


struct XIo
{
	enum MountType type;	// internal use only
	union XIoUnion
	{
		FileIo fio;
		AFileIo zio;
	}
	u;
};


struct XFile
{
	enum MountType type;	// internal use only

	// pointer to VFS file info storage; used to update size/mtime
	// after a newly written file is closed.
	TFile* tf;

	union XFileUnion
	{
		FileCommon fc;
		File f;
		AFile zf;
	}
	u;
};


// given a Mount, return the actual location (portable path) of
// <V_path>. used by vfs_realpath and VFile_reopen.
extern LibError x_realpath(const Mount* m, const char* V_path, char* P_real_path);

extern LibError x_open(const Mount* m, const char* V_path, int flags, TFile* tf, XFile* xf);
extern LibError x_close(XFile* xf);

extern LibError x_validate(const XFile* xf);

extern bool x_is_open(const XFile* xf);
extern off_t x_size(const XFile* xf);
extern uint x_flags(const XFile* xf);
extern void x_set_flags(XFile* xf, uint flags);

extern LibError x_io_issue(XFile* xf, off_t ofs, size_t size, void* buf, XIo* xio);
extern int x_io_has_completed(XIo* xio);
extern LibError x_io_wait(XIo* xio, void*& p, size_t& size);
extern LibError x_io_discard(XIo* xio);
extern LibError x_io_validate(const XIo* xio);

extern ssize_t x_io(XFile* xf, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

extern LibError x_map(XFile* xf, void*& p, size_t& size);
extern LibError x_unmap(XFile* xf);














//
// accessor routines that obviate the need to access Mount fields directly:
//

extern bool mount_is_archivable(const Mount* m);

extern bool mount_should_replace(const Mount* m_old, const Mount* m_new,
	size_t size_old, size_t size_new, time_t mtime_old, time_t mtime_new);

extern char mount_get_type(const Mount* m);



// stored by vfs_tree in TDir
struct RealDir
{
	// if exactly one real directory is mounted into this virtual dir,
	// this points to its location. used to add files to VFS when writing.
	//
	// the Mount is actually in the mount info and is invalid when
	// that's unmounted, but the VFS would then be rebuilt anyway.
	//
	// = 0 if no real dir mounted here; = -1 if more than one.
	const Mount* m;
#ifndef NO_DIR_WATCH
	intptr_t watch;
#endif
};

extern LibError mount_attach_real_dir(RealDir* rd, const char* P_path, const Mount* m, int flags);
extern void mount_detach_real_dir(RealDir* rd);

extern LibError mount_populate(TDir* td, RealDir* rd);


// "backs off of" all archives - closes their files and allows them to
// be rewritten or deleted (required by archive builder).
// must call mount_rebuild when done with the rewrite/deletes,
// because this call leaves the VFS in limbo!!
extern void mount_release_all_archives();


// rebuild the VFS, i.e. re-mount everything. open files are not affected.
// necessary after loose files or directories change, so that the VFS
// "notices" the changes and updates file locations. res calls this after
// dir_watch reports changes; can also be called from the console after a
// rebuild command. there is no provision for updating single VFS dirs -
// it's not worth the trouble.
extern LibError mount_rebuild();

// if <path> or its ancestors are mounted,
// return a VFS path that accesses it.
// used when receiving paths from external code.
extern LibError mount_make_vfs_path(const char* P_path, char* V_path);

#endif	// #ifndef VFS_MOUNT_H__
