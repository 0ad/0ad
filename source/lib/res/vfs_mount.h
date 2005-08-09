#ifndef VFS_MOUNT_H__
#define VFS_MOUNT_H__

#include "handle.h"

extern void mount_init();
extern void mount_shutdown();

struct Mount;



enum MountType;

struct TFile;
#include "file.h"
#include "zip.h"

struct XIo
{
	enum MountType type;	// internal use only
	union XIoUnion
	{
		FileIo fio;
		ZipIo zio;
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
		File f;
		ZFile zf;
	}
	u;
};


// given a Mount, return the actual location (portable path) of
// <V_path>. used by vfs_realpath and VFile_reopen.
extern int x_realpath(const Mount* m, const char* V_exact_path, char* P_real_path);

extern int x_open(const Mount* m, const char* V_exact_path, int flags, TFile* tf, XFile* xf);
extern int x_close(XFile* xf);

extern bool x_is_open(const XFile* xf);
extern off_t x_size(const XFile* xf);
extern uint x_flags(const XFile* xf);
extern void x_set_flags(XFile* xf, uint flags);

extern int x_io(XFile* xf, off_t ofs, size_t size, void* buf, FileIOCB cb, uintptr_t ctx);;

extern int x_map(XFile* xf, void*& p, size_t& size);
extern int x_unmap(XFile* xf);

extern int x_io_start(XFile* xf, off_t ofs, size_t size, void* buf, XIo* xio);
extern int x_io_complete(XIo* xio);
extern int x_io_wait(XIo* xio, void*& p, size_t& size);
extern int x_io_close(XIo* xio);













struct TDir;
extern int mount_populate(TDir* td, const Mount* m);


//
// accessor routines that obviate the need to access Mount fields directly:
//

extern bool mount_should_replace(const Mount* m_old, const Mount* m_new,
	bool files_are_identical);

extern char mount_get_type(const Mount* m);

#endif	// #ifndef VFS_MOUNT_H__
