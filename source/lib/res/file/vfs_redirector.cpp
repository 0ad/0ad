/**
 * =========================================================================
 * File        : vfs_redirector.cpp
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

#include "precompiled.h"

#include "file_internal.h"
#include "lib/byte_order.h"	// FOURCC

static const u32 vtbl_magic = FOURCC('F','P','V','T');


// HACK: these thunks and the vtbls are implemented here,
// although they belong in their respective provider's source file.
// this is currently necessary because vfs_mount doesn't yet
// abstract away the file provider (it's hardcoded for files+archives).

LibError afile_open_vfs(const char* fn, uint flags, TFile* tf,
	File* f)	// out
{
	const uintptr_t memento = tfile_get_memento(tf);
	const Mount* m = tfile_get_mount(tf);
	const Handle ha = mount_get_archive(m);
	return afile_open(ha, fn, memento, flags, f);
}

LibError file_open_vfs(const char* V_path, uint flags, TFile* tf,
	File* f)	// out
{
	char N_path[PATH_MAX];
	const Mount* m = tfile_get_mount(tf);
	RETURN_ERR(mount_realpath(V_path, m, N_path));
	RETURN_ERR(file_open(N_path, flags|FILE_DONT_SET_FN, f));
	// file_open didn't set fc.atom_fn due to FILE_DONT_SET_FN.
	f->atom_fn = file_make_unique_fn_copy(V_path);
	return INFO::OK;
}

static const FileProvider_VTbl archive_vtbl =
{
	vtbl_magic,
	0,0,0,	// not supported for archives ATM
	afile_open_vfs, afile_close, afile_validate,
	afile_io_issue, afile_io_has_completed, afile_io_wait, afile_io_discard, afile_io_validate,
	afile_read,
	afile_map, afile_unmap
};

static const FileProvider_VTbl file_vtbl =
{
	vtbl_magic,
	dir_open, dir_next_ent, dir_close,
	file_open_vfs, file_close, file_validate,
	file_io_issue, file_io_has_completed, file_io_wait, file_io_discard, file_io_validate,
	file_io,
	file_map, file_unmap
};

// see FileProvider_VTbl decl for details on why this is so empty.
static const FileProvider_VTbl tree_vtbl =
{
	vtbl_magic,
	tree_dir_open, tree_dir_next_ent, tree_dir_close,
	0, 0, 0,
	0, 0, 0, 0, 0,
	0,
	0, 0
};



// rationale for not using virtual functions for file_open vs afile_open:
// it would spread out the implementation of each function and makes
// keeping them in sync harder. we will very rarely add new sources and
// all these functions are in one spot anyway.


static LibError vtbl_validate(const FileProvider_VTbl* vtbl)
{
	if(!vtbl)
		WARN_RETURN(ERR::INVALID_PARAM);
	if(vtbl->magic != vtbl_magic)
		WARN_RETURN(ERR::CORRUPTED);
	return INFO::OK;
}

#define CHECK_VTBL(type) RETURN_ERR(vtbl_validate(type))


//
// directory entry enumeration
//

LibError xdir_open(const char* dir, DirIterator* di)
{
// HACK: it is unclear ATM how to set this properly. assume tree_dir_* is
// the only user ATM.
di->type = &tree_vtbl;
	CHECK_VTBL(di->type);
	return di->type->dir_open(dir, di);
}

LibError xdir_next_ent(DirIterator* di, DirEnt* ent)
{
	CHECK_VTBL(di->type);
	return di->type->dir_next_ent(di, ent);
}

LibError xdir_close(DirIterator* di)
{
	CHECK_VTBL(di->type);
	return di->type->dir_close(di);
}


//
// file object
//

bool xfile_is_open(const File* f)
{
	// not currently in use
	if(f->type == 0)
		return false;

	WARN_ERR(vtbl_validate(f->type));
	return true;
}

LibError xfile_open(const char* V_path, uint flags, TFile* tf, File* f)
{
	// find out who is providing this file
	const Mount* m = tfile_get_mount(tf);
	debug_assert(m != 0);

// HACK: see decl of vtbls. ideally vtbl would already be stored in
// Mount, but that's not implemented yet.
char c = mount_get_type(m);
const FileProvider_VTbl* vtbl = (c == 'F')? &file_vtbl : &archive_vtbl;

	CHECK_VTBL(vtbl);
	RETURN_ERR(vtbl->file_open(V_path, flags, tf, f));

	// success
	// note: don't assign these unless we succeed to avoid the
	// false impression that all is well.
	f->type = vtbl;
	return INFO::OK;
}

LibError xfile_close(File* f)
{
	// we must not complain if the file is not open. this happens if
	// attempting to open a nonexistent file: h_mgr automatically calls
	// the dtor after reload fails.
	// note: this takes care of checking the vtbl.
	if(!xfile_is_open(f))
		return INFO::OK;
	LibError ret = f->type->file_close(f);
	f->type = 0;
	return ret;
}

LibError xfile_validate(const File* f)
{
	CHECK_VTBL(f->type);
	return f->type->file_validate(f);
}


//
// IO
//

LibError xfile_io_issue(File* f, off_t ofs, size_t size, void* buf, FileIo* io)
{
	io->type = f->type;
	CHECK_VTBL(io->type);
	return io->type->io_issue(f, ofs, size, buf, io);
}

int xfile_io_has_completed(FileIo* io)
{
	CHECK_VTBL(io->type);
	return io->type->io_has_completed(io);
}

LibError xfile_io_wait(FileIo* io, void*& p, size_t& size)
{
	CHECK_VTBL(io->type);
	return io->type->io_wait(io, p, size);
}

LibError xfile_io_discard(FileIo* io)
{
	CHECK_VTBL(io->type);
	return io->type->io_discard(io);
}

LibError xfile_io_validate(const FileIo* io)
{
	CHECK_VTBL(io->type);
	return io->type->io_validate(io);
}

ssize_t xfile_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf, FileIOCB cb, uintptr_t ctx)
{
	CHECK_VTBL(f->type);
	// notes:
	// - for archive file: vfs_open makes sure it wasn't opened for writing
	// - normal file: let file_io alloc the buffer if the caller didn't
	//   (i.e. p = 0), because it knows about alignment / padding requirements
	return f->type->io(f, ofs, size, pbuf, cb, ctx);
}


//
// file mapping
//

LibError xfile_map(File* f, void*& p, size_t& size)
{
	CHECK_VTBL(f->type);
	return f->type->map(f, p, size);
}

LibError xfile_unmap(File* f)
{
	CHECK_VTBL(f->type);
	return f->type->unmap(f);
}
