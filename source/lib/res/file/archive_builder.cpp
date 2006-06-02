/**
 * =========================================================================
 * File        : archive_builder.cpp
 * Project     : 0 A.D.
 * Description : 
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
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

#include "lib/lib.h"
#include "lib/timer.h"
#include "file_internal.h"

// un-nice dependencies:
#include "ps/Loader.h"
#include <zlib.h>

static inline bool file_type_is_uncompressible(const char* fn)
{
	const char* ext = path_extension(fn);

	// this is a selection of file types that are certainly not
	// further compressible. we need not include every type under the sun -
	// this is only a slight optimization that avoids wasting time
	// compressing files. the real decision as to cmethod is made based
	// on attained compression ratio.
	static const char* uncompressible_exts[] =
	{
		"zip", "rar",
		"jpg", "jpeg", "png",
		"ogg", "mp3"
	};

	for(uint i = 0; i < ARRAY_SIZE(uncompressible_exts); i++)
	{
		if(!stricmp(ext+1, uncompressible_exts[i]))
			return true;
	}

	return false;
}


struct CompressParams
{
	bool attempt_compress;
	uintptr_t ctx;
	u32 crc;
};

static LibError compress_cb(uintptr_t cb_ctx, const void* block, size_t size, size_t* bytes_processed)
{
	CompressParams* p = (CompressParams*)cb_ctx;

	// comp_feed already makes note of total #bytes fed, and we need
	// vfs_io to return the uc size (to check if all data was read).
	*bytes_processed = size;

	// update checksum
	p->crc = crc32(p->crc, (const Bytef*)block, (uInt)size);

	if(p->attempt_compress)
		(void)comp_feed(p->ctx, block, size);
	return INFO_CB_CONTINUE;
}


// final decision on whether to store the file as compressed,
// given the observed compressed/uncompressed sizes.
static bool should_store_compressed(size_t ucsize, size_t csize)
{
	const float ratio = (float)ucsize / csize;
	const ssize_t bytes_saved = (ssize_t)ucsize - (ssize_t)csize;
	UNUSED2(bytes_saved);

	// tiny - store compressed regardless of savings.
	// rationale:
	// - CPU cost is negligible and overlapped with IO anyway;
	// - reading from compressed files uses less memory because we
	//   don't need to allocate space for padding in the final buffer.
	if(ucsize < 512)
		return true;

	// large high-entropy file - store uncompressed.
	// rationale:
	// - any bigger than this and CPU time becomes a problem: it isn't
	//   necessarily hidden by IO time anymore.
	if(ucsize >= 32*KiB && ratio < 1.02f)
		return false;

	// TODO: any other cases?
	// we currently store everything else compressed.
	return true;
}

static LibError read_and_compress_file(const char* atom_fn, uintptr_t ctx,
	ArchiveEntry& ent, void*& file_contents, FileIOBuf& buf)	// out
{
	struct stat s;
	RETURN_ERR(vfs_stat(atom_fn, &s));
	const size_t ucsize = s.st_size;
	// skip 0-length files.
	// rationale: zip.cpp needs to determine whether a CDFH entry is
	// a file or directory (the latter are written by some programs but
	// not needed - they'd only pollute the file table).
	// it looks like checking for ucsize=csize=0 is the safest way -
	// relying on file attributes (which are system-dependent!) is
	// even less safe.
	// we thus skip 0-length files to avoid confusing them with dirs.
	if(!ucsize)
		return INFO_SKIPPED;

	const bool attempt_compress = !file_type_is_uncompressible(atom_fn);
	if(attempt_compress)
	{
		RETURN_ERR(comp_reset(ctx));
		RETURN_ERR(comp_alloc_output(ctx, ucsize));
	}

	// read file into newly allocated buffer. if attempt_compress, also
	// compress the file into another buffer while waiting for IOs.
	size_t ucsize_read;
	const uint flags = 0;
	CompressParams params = { attempt_compress, ctx, 0 };
	RETURN_ERR(vfs_load(atom_fn, buf, ucsize_read, flags, compress_cb, (uintptr_t)&params));
	debug_assert(ucsize_read == ucsize);

	// if we compressed the file trial-wise, check results and
	// decide whether to store as such or not (based on compression ratio)
	bool store_compressed = false;
	void* cdata = 0; size_t csize = 0;
	if(attempt_compress)
	{
		LibError ret = comp_finish(ctx, &cdata, &csize);
		if(ret < 0)
		{
			file_buf_free(buf);
			return ret;
		}

		store_compressed = should_store_compressed(ucsize, csize);
	}

	// store file info
	ent.ucsize  = (off_t)ucsize;
	ent.mtime   = s.st_mtime;
	// .. ent.ofs is set by zip_archive_add_file
	ent.flags   = 0;
	ent.atom_fn = atom_fn;
	ent.crc     = params.crc;
	if(store_compressed)
	{
		ent.method = CM_DEFLATE;
		ent.csize  = (off_t)csize;
		file_contents = cdata;
	}
	else
	{
		ent.method = CM_NONE;
		ent.csize  = (off_t)ucsize;
		file_contents = (void*)buf;
	}

	// note: no need to free cdata - it is owned by the
	// compression context and can be reused.

	return ERR_OK;
}



LibError archive_build_init(const char* P_archive_filename, Filenames V_fns,
							ArchiveBuildState* ab)
{
	RETURN_ERR(zip_archive_create(P_archive_filename, &ab->za));
	ab->ctx = comp_alloc(CT_COMPRESSION, CM_DEFLATE);
	ab->V_fns = V_fns;

	// count number of files (needed to estimate progress)
	for(ab->num_files = 0; ab->V_fns[ab->num_files]; ab->num_files++) {}

	ab->i = 0;
	return ERR_OK;
}


int archive_build_continue(ArchiveBuildState* ab)
{
	const double end_time = get_time() + 200e-3;

	for(;;)
	{
		const char* V_fn = ab->V_fns[ab->i];
		if(!V_fn)
			break;

		ArchiveEntry ent; void* file_contents; FileIOBuf buf;
		if(read_and_compress_file(V_fn, ab->ctx, ent, file_contents, buf) == ERR_OK)
		{
			(void)zip_archive_add_file(ab->za, &ent, file_contents);
			(void)file_buf_free(buf);
		}

		ab->i++;
		LDR_CHECK_TIMEOUT((int)ab->i, (int)ab->num_files);
	}

	// note: this is currently known to fail if there are no files in the list
	// - zlib.h says: Z_DATA_ERROR is returned if freed prematurely.
	// safe to ignore.
	comp_free(ab->ctx); ab->ctx = 0;
	(void)zip_archive_finish(ab->za);

	return ERR_OK;
}


void archive_build_cancel(ArchiveBuildState* ab)
{
	// note: the GUI may call us even though no build was ever in progress.
	// be sure to make all steps no-op if <ab> is zeroed (initial state) or
	// no build is in progress.

	comp_free(ab->ctx); ab->ctx = 0;
	if(ab->za)
		(void)zip_archive_finish(ab->za);
	memset(ab, 0, sizeof(*ab));
}


LibError archive_build(const char* P_archive_filename, Filenames V_fns)
{
	ArchiveBuildState ab;
	RETURN_ERR(archive_build_init(P_archive_filename, V_fns, &ab));
	for(;;)
	{
		int ret = archive_build_continue(&ab);
		RETURN_ERR(ret);
		if(ret == ERR_OK)
			return ERR_OK;
	}
}
