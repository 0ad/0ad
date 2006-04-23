/**
 * =========================================================================
 * File        : vfs_redirector.h
 * Project     : 0 A.D.
 * Description : 
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef VFS_REDIRECTOR_H__
#define VFS_REDIRECTOR_H__

struct FileProvider_VTbl
{
	// FOURCC that is checked on each access to ensure this is a valid vtbl.
	u32 magic;
	// note: no need to store name of this provider for debugging purposes;
	// that can be deduced from the function pointers below.

	// file objects
	LibError (*open)(const char* V_path, uint flags, TFile* tf, File* f);
	LibError (*close)(File* f);
	LibError (*validate)(const File* f);

	// async IO
	LibError (*io_issue)(File* f, off_t ofs, size_t size, void* buf, FileIo* xio);
	int (*io_has_completed)(FileIo* xio);
	LibError (*io_wait)(FileIo* xio, void*& p, size_t& size);
	LibError (*io_discard)(FileIo* xio);
	LibError (*io_validate)(const FileIo* xio);

	// sync IO
	ssize_t (*io)(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

	// file mapping
	LibError (*map)(File* f, void*& p, size_t& size);
	LibError (*unmap)(File* f);
};


extern bool xfile_is_open(const File* f);

extern LibError xfile_open(const char* V_path, uint flags, TFile* tf, File* f);
extern LibError xfile_close(File* f);
extern LibError xfile_validate(const File* f);

extern LibError xfile_io_validate(File* f, off_t ofs, size_t size, void* buf, FileIo* xio);
extern int xfile_io_has_completed(FileIo* xio);
extern LibError xfile_io_wait(FileIo* xio, void*& p, size_t& size);
extern LibError xfile_io_discard(FileIo* xio);
extern LibError xfile_io_validate(const FileIo* xio);

extern ssize_t xfile_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx);

extern LibError xfile_map(File* f, void*& p, size_t& size);
extern LibError xfile_unmap(File* f);

#endif	//	#ifndef VFS_REDIRECTOR_H__
