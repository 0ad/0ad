/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * support routines for texture codecs
 */

#include "precompiled.h"
#include "tex_codec.h"

#include <string.h>
#include <stdlib.h>

#include "lib/allocators/shared_ptr.h" // ArrayDeleter
#include "tex.h"

// Statically allocate all of the codecs...
TexCodecDds DdsCodec;
TexCodecPng PngCodec;
TexCodecJpg JpgCodec;
TexCodecTga TgaCodec;
TexCodecPng BmpCodec;
// Codecs will be searched in this order
static const ITexCodec *codecs[] = {(ITexCodec *)&DdsCodec, (ITexCodec *)&PngCodec,
	(ITexCodec *)&JpgCodec, (ITexCodec *)&TgaCodec, (ITexCodec *)&BmpCodec};
static const int codecs_len = sizeof(codecs) / sizeof(ITexCodec*);

// find codec that recognizes the desired output file extension,
// or return ERR::TEX_UNKNOWN_FORMAT if unknown.
// note: does not raise a warning because it is used by
// tex_is_known_extension.
Status tex_codec_for_filename(const OsPath& extension, const ITexCodec** c)
{
	for(int i = 0; i < codecs_len; ++i)
	{
		// we found it
		if(codecs[i]->is_ext(extension)) {
			*c = codecs[i];
			return INFO::OK;
		}
	}

	return ERR::TEX_UNKNOWN_FORMAT;	// NOWARN
}


// find codec that recognizes the header's magic field
Status tex_codec_for_header(const u8* file, size_t file_size, const ITexCodec** c)
{
	// we guarantee at least 4 bytes for is_hdr to look at
	if(file_size < 4)
		WARN_RETURN(ERR::TEX_INCOMPLETE_HEADER);
	
	for(int i = 0; i < codecs_len; ++i)
	{
		// we found it
		if(codecs[i]->is_hdr(file)) {
			*c = codecs[i];
			return INFO::OK;
		}
	}

	WARN_RETURN(ERR::TEX_UNKNOWN_FORMAT);
}

Status tex_codec_transform(Tex* t, size_t transforms)
{
	Status ret = INFO::TEX_CODEC_CANNOT_HANDLE;

	// find codec that understands the data, and transform
	for(int i = 0; i < codecs_len; ++i)
	{
		Status err = codecs[i]->transform(t, transforms);
		// success
		if(err == INFO::OK)
			return INFO::OK;
		// something went wrong
		else if(err != INFO::TEX_CODEC_CANNOT_HANDLE)
		{
			ret = err;
			DEBUG_WARN_ERR(ERR::LOGIC);	// codec indicates error
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
// used by PNG and JPG codecs.
//
// note: we don't allocate the data param ourselves because this function is
// needed for encoding, too (where data is already present).
std::vector<RowPtr> tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch, size_t src_flags, size_t dst_orientation)
{
	const bool flip = !tex_orientations_match(src_flags, dst_orientation);

	std::vector<RowPtr> rows(h);

	// determine start position and direction
	RowPtr pos        = flip? data+pitch*(h-1) : data;
	const ssize_t add = flip? -(ssize_t)pitch : (ssize_t)pitch;
	const RowPtr end  = flip? data-pitch : data+pitch*h;

	for(size_t i = 0; i < h; i++)
	{
		rows[i] = pos;
		pos += add;
	}

	ENSURE(pos == end);
	return rows;
}


Status tex_codec_write(Tex* t, size_t transforms, const void* hdr, size_t hdr_size, DynArray* da)
{
	RETURN_STATUS_IF_ERR(t->transform(transforms));

	void* img_data = t->get_data(); const size_t img_size = t->img_size();
	RETURN_STATUS_IF_ERR(da_append(da, hdr, hdr_size));
	RETURN_STATUS_IF_ERR(da_append(da, img_data, img_size));
	return INFO::OK;
}
