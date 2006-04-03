#include "precompiled.h"

#include <string.h>
#include <stdlib.h>

#include "tex_codec.h"
#include "tex.h"

static const TexCodecVTbl* codecs;

// add this vtbl to the codec list. called at NLSO init time by the
// TEX_CODEC_REGISTER in each codec file. note that call order and therefore
// order in the list is undefined, but since each codec only steps up if it
// can handle the given format, this is not a problem.
//
// returns int to alloc calling from a macro at file scope.
int tex_codec_register(TexCodecVTbl* c)
{
	debug_assert(c);

	// insert at front of list.
	c->next = codecs;
	codecs = c;

	return 0;	// (assigned to dummy variable)
}


// find codec that recognizes the desired output file extension,
// or return ERR_UNKNOWN_FORMAT if unknown.
// note: does not raise a warning because it is used by
// tex_is_known_extension.
LibError tex_codec_for_filename(const char* fn, const TexCodecVTbl** c)
{
	const char* ext = strrchr(fn, '.');
	if(!ext)
		return ERR_UNKNOWN_FORMAT;
	ext++;	// skip '.'

	for(*c = codecs; *c; *c = (*c)->next)
	{
		// we found it
		if((*c)->is_ext(ext))
			return ERR_OK;
	}

	return ERR_UNKNOWN_FORMAT;
}


// find codec that recognizes the header's magic field
LibError tex_codec_for_header(const u8* file, size_t file_size, const TexCodecVTbl** c)
{
	// we guarantee at least 4 bytes for is_hdr to look at
	if(file_size < 4)
		WARN_RETURN(ERR_INCOMPLETE_HEADER);

	for(*c = codecs; *c; *c = (*c)->next)
	{
		// we found it
		if((*c)->is_hdr(file))
			return ERR_OK;
	}

	WARN_RETURN(ERR_UNKNOWN_FORMAT);
}


LibError tex_codec_transform(Tex* t, uint transforms)
{
	LibError ret = ERR_TEX_CODEC_CANNOT_HANDLE;

	// find codec that understands the data, and transform
	for(const TexCodecVTbl* c = codecs; c; c = c->next)
	{
		LibError err = c->transform(t, transforms);
		// success
		if(err == ERR_OK)
			return ERR_OK;
		// something went wrong
		else if(err != ERR_TEX_CODEC_CANNOT_HANDLE)
		{
			ret = err;
			debug_warn("codec indicates error");
		}
	}

	return ret;
}


//-----------------------------------------------------------------------------
// helper functions used by codecs
//-----------------------------------------------------------------------------

// allocate an array of row pointers that point into the given texture data.
// <file_orientation> indicates whether the file format is top-down or
// bottom-up; the row array is inverted if necessary to match global
// orienatation. (this is more efficient than "transforming" later)
//
// used by PNG and JPG codecs; caller must free() rows when done.
//
// note: we don't allocate the data param ourselves because this function is
// needed for encoding, too (where data is already present).
LibError tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch,
	uint src_flags, uint dst_orientation, RowArray& rows)
{
	const bool flip = !tex_orientations_match(src_flags, dst_orientation);

	rows = (RowArray)malloc(h * sizeof(RowPtr));
	if(!rows)
		WARN_RETURN(ERR_NO_MEM);

	// determine start position and direction
	RowPtr pos        = flip? data+pitch*(h-1) : data;
	const ssize_t add = flip? -(ssize_t)pitch : (ssize_t)pitch;
	const RowPtr end  = flip? data-pitch : data+pitch*h;

	for(size_t i = 0; i < h; i++)
	{
		rows[i] = pos;
		pos += add;
	}

	debug_assert(pos == end);
	return ERR_OK;
}


LibError tex_codec_write(Tex* t, uint transforms, const void* hdr, size_t hdr_size, DynArray* da)
{
	RETURN_ERR(tex_transform(t, transforms));

	void* img_data = tex_get_data(t); const size_t img_size = tex_img_size(t);
	RETURN_ERR(da_append(da, hdr, hdr_size));
	RETURN_ERR(da_append(da, img_data, img_size));
	return ERR_OK;
}
