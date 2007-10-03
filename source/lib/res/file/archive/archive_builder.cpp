/**
 * =========================================================================
 * File        : archive_builder.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "archive_builder.h"

#include "lib/timer.h"
#include "../file_internal.h"

// un-nice dependencies:
#include "ps/Loader.h"


// vfs_load callback that compresses the data in parallel with IO
// (for incompressible files, we just calculate the checksum)
class Compressor
{
public:
	Compressor(uintptr_t ctx, const char* atom_fn, size_t usize)
		: m_ctx(ctx)
		, m_usize(usize)
		, m_skipCompression(IsFileTypeIncompressible(atom_fn))
		, m_cdata(0), m_csize(0), m_checksum(0)
	{
		comp_reset(m_ctx);
		m_csizeBound = comp_max_output_size(m_ctx, usize);
		THROW_ERR(comp_alloc_output(m_ctx, m_csizeBound));
	}

	LibError Feed(const u8* ublock, size_t ublockSize, size_t* bytes_processed)
	{
		// comp_feed already makes note of total #bytes fed, and we need
		// vfs_io to return the usize (to check if all data was read).
		*bytes_processed = ublockSize;

		if(m_skipCompression)
		{
			// (since comp_finish returns the checksum, we only need to update this
			// when not compressing.)
			m_checksum = comp_update_checksum(m_ctx, m_checksum, ublock, ublockSize);
		}
		else
		{
			// note: we don't need the return value because comp_finish
			// will tell us the total csize.
			(void)comp_feed(m_ctx, ublock, ublockSize);
		}

		return INFO::CB_CONTINUE;
	}

	LibError Finish()
	{
		if(m_skipCompression)
			return INFO::OK;

		RETURN_ERR(comp_finish(m_ctx, &m_cdata, &m_csize, &m_checksum));
		debug_assert(m_csize <= m_csizeBound);
		return INFO::OK;
	}

	u32 Checksum() const
	{
		return m_checksum;
	}

	// final decision on whether to store the file as compressed,
	// given the observed compressed/uncompressed sizes.
	bool IsCompressionProfitable() const
	{
		// file is definitely incompressible.
		if(m_skipCompression)
			return false;

		const float ratio = (float)m_usize / m_csize;
		const ssize_t bytes_saved = (ssize_t)m_usize - (ssize_t)m_csize;
		UNUSED2(bytes_saved);

		// tiny - store compressed regardless of savings.
		// rationale:
		// - CPU cost is negligible and overlapped with IO anyway;
		// - reading from compressed files uses less memory because we
		//   don't need to allocate space for padding in the final buffer.
		if(m_usize < 512)
			return true;

		// large high-entropy file - store uncompressed.
		// rationale:
		// - any bigger than this and CPU time becomes a problem: it isn't
		//   necessarily hidden by IO time anymore.
		if(m_usize >= 32*KiB && ratio < 1.02f)
			return false;

		// we currently store everything else compressed.
		return true;
	}

	void GetOutput(const u8*& cdata, size_t& csize) const
	{
		debug_assert(!m_skipCompression);
		debug_assert(m_cdata && m_csize);
		cdata = m_cdata;
		csize = m_csize;

		// note: no need to free cdata - it is owned by the
		// compression context and can be reused.
	}

private:
	static bool IsFileTypeIncompressible(const char* fn)
	{
		const char* ext = path_extension(fn);

		// this is a selection of file types that are certainly not
		// further compressible. we need not include every type under the sun -
		// this is only a slight optimization that avoids wasting time
		// compressing files. the real decision as to cmethod is made based
		// on attained compression ratio.
		static const char* incompressible_exts[] =
		{
			"zip", "rar",
			"jpg", "jpeg", "png",
			"ogg", "mp3"
		};

		for(uint i = 0; i < ARRAY_SIZE(incompressible_exts); i++)
		{
			if(!strcasecmp(ext+1, incompressible_exts[i]))
				return true;
		}

		return false;
	}


	uintptr_t m_ctx;
	size_t m_usize;
	size_t m_csizeBound;
	bool m_skipCompression;

	u8* m_cdata;
	size_t m_csize;
	u32 m_checksum;
};

static LibError compressor_feed_cb(uintptr_t cbData,
	const u8* ublock, size_t ublockSize, size_t* bytes_processed)
{
	Compressor& compressor = *(Compressor*)cbData;
	return compressor.Feed(ublock, ublockSize, bytes_processed);
}


static LibError read_and_compress_file(const char* atom_fn, uintptr_t ctx,
	ArchiveEntry& ent, const u8*& file_contents, FileIOBuf& buf)	// out
{
	struct stat s;
	RETURN_ERR(vfs_stat(atom_fn, &s));
	const size_t usize = s.st_size;
	// skip 0-length files.
	// rationale: zip.cpp needs to determine whether a CDFH entry is
	// a file or directory (the latter are written by some programs but
	// not needed - they'd only pollute the file table).
	// it looks like checking for usize=csize=0 is the safest way -
	// relying on file attributes (which are system-dependent!) is
	// even less safe.
	// we thus skip 0-length files to avoid confusing them with directories.
	if(!usize)
		return INFO::SKIPPED;

	Compressor compressor(ctx, atom_fn, usize);

	// read file into newly allocated buffer and run compressor.
	size_t usize_read;
	const uint flags = 0;
	RETURN_ERR(vfs_load(atom_fn, buf, usize_read, flags, compressor_feed_cb, (uintptr_t)&compressor));
	debug_assert(usize_read == usize);

	LibError ret = compressor.Finish();
	if(ret < 0)
	{
		file_buf_free(buf);
		return ret;
	}

	// store file info
	ent.usize = (off_t)usize;
	ent.mtime = s.st_mtime;
	// .. ent.ofs is set by zip_archive_add_file
	ent.flags = 0;
	ent.atom_fn = atom_fn;
	ent.checksum = compressor.Checksum();
	if(compressor.IsCompressionProfitable())
	{
		ent.method = CM_DEFLATE;
		size_t csize;
		compressor.GetOutput(file_contents, csize);
		ent.csize = (off_t)csize;
	}
	else
	{
		ent.method = CM_NONE;
		ent.csize  = (off_t)usize;
		file_contents = buf;
	}

	return INFO::OK;
}


//-----------------------------------------------------------------------------

LibError archive_build_init(const char* P_archive_filename, Filenames V_fns, ArchiveBuildState* ab)
{
	RETURN_ERR(zip_archive_create(P_archive_filename, &ab->za));
	ab->ctx = comp_alloc(CT_COMPRESSION, CM_DEFLATE);
	ab->V_fns = V_fns;

	// count number of files (needed to estimate progress)
	for(ab->num_files = 0; ab->V_fns[ab->num_files]; ab->num_files++) {}

	ab->i = 0;
	return INFO::OK;
}


int archive_build_continue(ArchiveBuildState* ab)
{
	const double end_time = get_time() + 200e-3;

	for(;;)
	{
		const char* V_fn = ab->V_fns[ab->i];
		if(!V_fn)
			break;

		ArchiveEntry ent; const u8* file_contents; FileIOBuf buf;
		if(read_and_compress_file(V_fn, ab->ctx, ent, file_contents, buf) == INFO::OK)
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

	return INFO::OK;
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
		if(ret == INFO::OK)
			return INFO::OK;
	}
}
