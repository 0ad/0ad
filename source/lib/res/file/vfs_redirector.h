/**
 * =========================================================================
 * File        : vfs_redirector.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_REDIRECTOR
#define INCLUDED_VFS_REDIRECTOR

#include "file.h"
struct FileIo;

struct FileProvider_VTbl
{
	// FOURCC that is checked on each access to ensure this is a valid vtbl.
	u32 magic;
	// note: no need to store name of this provider for debugging purposes;
	// that can be deduced from the function pointers below.

	// directory entry enumeration
	// note: these don't really fit in with the other methods.
	// they make sense for both the VFS tree as well as the concrete
	// file providers underlying it. due to this overlap and to allow
	// file.cpp's next_ent function to access dir_filtered_next_ent,
	// it is included anyway.
	LibError (*dir_open)(const char* dir, DirIterator* di);
	LibError (*dir_next_ent)(DirIterator* di, DirEnt* ent);
	LibError (*dir_close)(DirIterator* di);

	// file objects
	LibError (*file_open)(const char* V_path, uint flags, TFile* tf, File* f);
	LibError (*file_close)(File* f);
	LibError (*file_validate)(const File* f);

	// IO
	LibError (*io_issue)(File* f, off_t ofs, size_t size, void* buf, FileIo* io);
	int      (*io_has_completed)(FileIo* io);
	LibError (*io_wait)(FileIo* io, void*& p, size_t& size);
	LibError (*io_discard)(FileIo* io);
	LibError (*io_validate)(const FileIo* io);
	ssize_t  (*io)(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

	// file mapping
	LibError (*map)(File* f, void*& p, size_t& size);
	LibError (*unmap)(File* f);
};


extern LibError xdir_open(const char* dir, DirIterator* di);
extern LibError xdir_next_ent(DirIterator* di, DirEnt* ent);
extern LibError xdir_close(DirIterator* di);

extern bool     xfile_is_open(const File* f);
extern LibError xfile_open(const char* V_path, uint flags, TFile* tf, File* f);
extern LibError xfile_close(File* f);
extern LibError xfile_validate(const File* f);

extern LibError xfile_io_issue(File* f, off_t ofs, size_t size, void* buf, FileIo* io);
extern int      xfile_io_has_completed(FileIo* io);
extern LibError xfile_io_wait(FileIo* io, void*& p, size_t& size);
extern LibError xfile_io_discard(FileIo* io);
extern LibError xfile_io_validate(const FileIo* io);
extern ssize_t  xfile_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

extern LibError xfile_map(File* f, void*& p, size_t& size);
extern LibError xfile_unmap(File* f);

#endif	//	#ifndef INCLUDED_VFS_REDIRECTOR
