// 2d texture format codecs
//
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <math.h>
#include <stdlib.h>

#include <algorithm>

#include "lib.h"
#include "../res.h"
#include "tex.h"
#include "tex_codec.h"


#define ERR_TOO_SHORT -4


// be careful not to use other tex_* APIs here because they call us.
int tex_validate(const uint line, const Tex* t)
{
	const char* msg = 0;
	int err = -1;

	// texture data
	size_t tex_file_size;
	void* tex_file = mem_get_ptr(t->hm, &tex_file_size);
	// .. only check validity if the image is still in memory.
	//    (e.g. ogl_tex frees the data after uploading to GL)
	if(tex_file)
	{
		// possible causes: texture file header is invalid,
		// or file wasn't loaded completely.
		if(tex_file_size < t->ofs + t->w*t->h*t->bpp/8)
			msg = "file size smaller than header+texels";
	}

	// bits per pixel
	// (we don't bother checking all values; a sanity check is enough)
	if(t->bpp % 4 || t->bpp > 32)
		msg = "invalid bpp? should be one of {4,8,16,24,32}";

	// flags
	// .. DXT
	const uint dxt = t->flags & TEX_DXT;
	if(dxt != 0 && dxt != 1 && dxt != DXT1A && dxt != 3 && dxt != 5)
		msg = "invalid DXT in flags";
	// .. orientation
	const uint orientation = t->flags & TEX_ORIENTATION;
	if(orientation == (TEX_BOTTOM_UP|TEX_TOP_DOWN))
		msg = "invalid orientation in flags";

	if(msg)
	{
		debug_printf("tex_validate at line %d failed: %s (error code %d)\n", line, msg, err);
		debug_warn("tex_validate failed");
		return err;
	}

	return 0;
}

#define CHECK_TEX(t) CHECK_ERR(tex_validate(__LINE__, t))



// rationale for default: see tex_set_global_orientation
static int global_orientation = TEX_TOP_DOWN;



// check if the given texture format is acceptable: 8bpp grey,
// 24bpp color or 32bpp color+alpha (BGR / upside down are permitted).
// basically, this is the "plain" format understood by all codecs and
// tex_codec_plain_transform.
// return 0 if ok or a negative error code.
static int validate_format(uint bpp, uint flags)
{
	const bool alpha   = (flags & TEX_ALPHA  ) != 0;
	const bool grey    = (flags & TEX_GREY   ) != 0;
	const bool dxt     = (flags & TEX_DXT    ) != 0;
	const bool mipmaps = (flags & TEX_MIPMAPS) != 0;

	if(dxt || mipmaps)
		return ERR_TEX_FMT_INVALID;

	// grey must be 8bpp without alpha, or it's invalid.
	if(grey)
	{
		if(bpp == 8 && !alpha)
			return 0;
		return ERR_TEX_FMT_INVALID;
	}

	if(bpp == 24 && !alpha)
		return 0;
	if(bpp == 32 && alpha)
		return 0;

	return ERR_TEX_FMT_INVALID;
}


// handles BGR and row flipping in "plain" format (see below).
//
// called by codecs after they get their format-specific transforms out of
// the way. note that this approach requires several passes over the image,
// but is much easier to maintain than providing all<->all conversion paths.
//
// somewhat optimized (loops are hoisted, cache associativity accounted for)
static int plain_transform(Tex* t, uint transforms)
{
	CHECK_TEX(t);

	// extract texture info
	const uint w = t->w, h = t->h, bpp = t->bpp, flags = t->flags;
	u8* const img = tex_get_data(t);

	// sanity checks (not errors, we just can't handle these cases)
	// .. unknown transform
	if(transforms & ~(TEX_BGR|TEX_ORIENTATION))
		return TEX_CODEC_CANNOT_HANDLE;
	// .. img is not in "plain" format
	if(validate_format(bpp, flags) != 0)
		return TEX_CODEC_CANNOT_HANDLE;
	// .. nothing to do
	if(!transforms)
		return 0;

	// setup row source/destination pointers (simplifies outer loop)
	u8* dst = img;
	const u8* src = img;
	const size_t pitch = w * bpp/8;
	ssize_t row_ofs = (ssize_t)pitch;
	// avoid y*pitch multiply in row loop; instead, add row_ofs.
	void* clone_img = 0;
	// flipping rows (0,1,2 -> 2,1,0)
	if(transforms & TEX_ORIENTATION)
	{
		// L1 cache is typically A2 => swapping in-place with a line buffer
		// leads to thrashing. we'll assume the whole texture*2 fits in cache,
		// allocate a copy, and transfer directly from there.
		//
		// note: we don't want to return a new buffer: the user assumes
		// buffer address will remain unchanged.
		const size_t size = h*pitch;
		clone_img = mem_alloc(size, 32*KiB);
		if(!clone_img)
			return ERR_NO_MEM;
		memcpy(clone_img, img, size);
		src = (const u8*)clone_img+size-pitch;	// last row
		row_ofs = -(ssize_t)pitch;
	}

	// no BGR convert necessary
	if(!(transforms & TEX_BGR))
	{
		for(uint y = 0; y < h; y++)
		{
			memcpy(dst, src, pitch);
			dst += pitch;
			src += row_ofs;
		}
	}
	// RGB <-> BGR
	else if(bpp == 24)
	{
		for(uint y = 0; y < h; y++)
		{
			for(uint x = 0; x < w; x++)
			{
				// need temporaries in case src == dst (i.e. not flipping)
				const u8 b = src[0], g = src[1], r = src[2];
				dst[0] = r; dst[1] = g; dst[2] = b;
				dst += 3;
				src += 3;
			}
			src += row_ofs - pitch;	// flip? previous row : stay
		}
	}
	// RGBA <-> BGRA
	else if(bpp == 32)
	{
		for(uint y = 0; y < h; y++)
		{
			for(uint x = 0; x < w; x++)
			{
				// need temporaries in case src == dst (i.e. not flipping)
				const u8 b = src[0], g = src[1], r = src[2], a = src[3];
				dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
				dst += 4;
				src += 4;
			}
			src += row_ofs - pitch;	// flip? previous row : stay
		}
	}

	if(clone_img)
		mem_free(clone_img);

	return 0;
}


//-----------------------------------------------------------------------------
// support routines for codecs
//-----------------------------------------------------------------------------

// should be a tight bound because we iterate this many times (for convenience)
static const uint MAX_CODECS = 8;
static const TexCodecVTbl* codecs[MAX_CODECS];

// add this vtbl to the codec list. called at NLSO init time by the
// TEX_CODEC_REGISTER in each codec file. note that call order and therefore
// order in the list is undefined, but since each codec only steps up if it
// can handle the given format, this is not a problem.
int tex_codec_register(const TexCodecVTbl* c)
{
	debug_assert(c != 0 && "tex_codec_register(0) - why?");

	for(int i = 0; i < MAX_CODECS; i++)
	{
		// slot available
		if(codecs[i] == 0)
		{
			codecs[i] = c;
			return 0;	// success
		}
	}

	// didn't find a free slot.
	debug_warn("tex_codec_register: increase MAX_CODECS");
	return 0;	// failure, but caller ignores return value
}


// find codec that recognizes the desired output file extension
int tex_codec_for_filename(const char* fn, const TexCodecVTbl** c)
{
	const char* ext = strrchr(fn, '.');
	if(!ext)
		return ERR_UNKNOWN_FORMAT;
	ext++;	// skip '.'

	for(uint i = 0; i < MAX_CODECS; i++)
	{
		*c = codecs[i];
		// skip if 0 (e.g. if MAX_CODECS != num codecs)
		if(!*c)
			continue;
		// we found it
		if((*c)->is_ext(ext))
			return 0;
	}

	return ERR_UNKNOWN_FORMAT;
}


// find codec that recognizes the header's magic field
int tex_codec_for_header(const u8* file, size_t file_size, const TexCodecVTbl** c)
{
	// we guarantee at least 4 bytes for is_hdr to look at
	if(file_size < 4)
		return ERR_TOO_SHORT;
	for(uint i = 0; i < MAX_CODECS; i++)
	{
		*c = codecs[i];
		// skip if 0 (e.g. if MAX_CODECS != num codecs)
		if(!*c)
			continue;
		// we found it
		if((*c)->is_hdr(file))
			return 0;
	}

	return ERR_UNKNOWN_FORMAT;
}


// allocate an array of row pointers that point into the given texture data.
// <file_orientation> indicates whether the file format is top-down or
// bottom-up; the row array is inverted if necessary to match global
// orienatation. (this is more efficient than "transforming" later)
//
// used by PNG and JPG codecs; caller must free() rows when done.
//
// note: we don't allocate the data param ourselves because this function is
// needed for encoding, too (where data is already present).
int tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch,
	uint src_flags, uint dst_orientation, RowArray& rows)
{
	// convenience for caller: allow passing all flags.
	const uint src_orientation = src_flags & TEX_ORIENTATION;
	if(dst_orientation == 0)
		dst_orientation = global_orientation;

	rows = (RowArray)malloc(h * sizeof(RowPtr));
	if(!rows)
		return ERR_NO_MEM;

	// determine start position and direction
	const bool flip = (src_orientation ^ dst_orientation) != 0;
	RowPtr pos        = flip? data+pitch*(h-1) : data;
	const ssize_t add = flip? -(ssize_t)pitch : (ssize_t)pitch;
	const RowPtr end  = flip? data-pitch : data+pitch*h;

	for(size_t i = 0; i < h; i++)
	{
		rows[i] = pos;
		pos += add;
	}

	debug_assert(pos == end);
	return 0;
}


int tex_codec_write(Tex* t, uint transforms, const void* hdr, size_t hdr_size, DynArray* da)
{
	CHECK_TEX(t);

	RETURN_ERR(tex_transform(t, transforms));

	void* img_data = tex_get_data(t); const size_t img_size = tex_img_size(t);
	RETURN_ERR(da_append(da, hdr, hdr_size));
	RETURN_ERR(da_append(da, img_data, img_size));
	return 0;
}


//-----------------------------------------------------------------------------
// API
//-----------------------------------------------------------------------------

// switch between top-down and bottom-up orientation.
//
// the default top-down is to match the Photoshop DDS plugin's output.
// DDS is the optimized format, so we don't want to have to flip that.
// notes:
// - there's no way to tell which orientation a DDS file has;
//   we have to go with what the DDS encoder uses.
// - flipping DDS is possible without re-encoding; we'd have to shuffle
//   around the pixel values inside the 4x4 blocks.
//
// the app can change orientation, e.g. to speed up loading
// "upside-down" formats, or to match OpenGL's bottom-up convention.
void tex_set_global_orientation(int o)
{
	debug_assert(o == TEX_TOP_DOWN || o == TEX_BOTTOM_UP);
	global_orientation = o;
}


u8* tex_get_data(const Tex* t)
{
	if(tex_validate(__LINE__, t) < 0)
		return 0;

	u8* p = (u8*)mem_get_ptr(t->hm);
	if(!p)
		return 0;
	return p + t->ofs;
}

size_t tex_img_size(const Tex* t)
{
	if(tex_validate(__LINE__, t) < 0)
		return 0;

	return t->w * t->h * t->bpp/8;
}



size_t tex_hdr_size(const char* fn)
{
	const TexCodecVTbl* c;
	CHECK_ERR(tex_codec_for_filename(fn, &c));
	return c->hdr_size(0);
}


int tex_load_mem(Handle hm, Tex* t)
{
	u8* file; size_t file_size;
	CHECK_ERR(mem_get(hm, &file, &file_size));

	const TexCodecVTbl* c;
	CHECK_ERR(tex_codec_for_header(file, file_size, &c));

	// make sure enough of the file has been read
	const size_t min_hdr_size = c->hdr_size(0);
	if(file_size < min_hdr_size)
		return ERR_TOO_SHORT;
	const size_t hdr_size = c->hdr_size(file);
	if(file_size < hdr_size)
		return ERR_TOO_SHORT;


	DynArray da;
	CHECK_ERR(da_wrap_fixed(&da, file, file_size));
	t->hm = hm;
	t->ofs = hdr_size;

	int err = c->decode(&da, t);
	if(err < 0)
	{
		debug_printf("tex_load_mem (%s): %d", c->name, err);
		debug_warn("tex_load_mem failed");
		return err;
	}

	// sanity checks
	if(!t->w || !t->h || t->bpp > 32)
		return ERR_TEX_FMT_INVALID;
	// TODO: need to compare against the new t->hm (file may be compressed, cannot use file_size)
	//if(mem_size < t->ofs + tex_img_size(t))
	//	return ERR_TOO_SHORT;

	// flip image to global orientation
	uint orientation = t->flags & TEX_ORIENTATION;
	// .. but only if it knows which way around it is (DDS doesn't)
	if(orientation)
	{
		uint transforms = orientation ^ global_orientation;
		WARN_ERR(plain_transform(t, transforms));
	}

	CHECK_TEX(t);
	return 0;
}


int tex_load(const char* fn, Tex* t)
{
	// load file
	void* p; size_t size;	// unused
	Handle hm = vfs_load(fn, p, size);
	RETURN_ERR(hm);	// (need handle below; can't test return value directly)
	int ret = tex_load_mem(hm, t);
	// do not free hm! it either still holds the image data (i.e. texture
	// wasn't compressed) or was replaced by a new buffer for the image data.
	if(ret < 0)
		memset(t, 0, sizeof(Tex));

	// <t> has already been validated.
	return ret;
}


int tex_free(Tex* t)
{
	CHECK_TEX(t);
	mem_free_h(t->hm);
	return 0;
}


int tex_transform(Tex* t, uint transforms)
{
	CHECK_TEX(t);

	// find codec that understands the data, and transform
	for(int i = 0; i < MAX_CODECS; i++)
	{
		// MAX_CODECS isn't a tight bound and we have hit a 0 entry
		if(!codecs[i])
			continue;
		int err = codecs[i]->transform(t, transforms);
		if(err == TEX_CODEC_CANNOT_HANDLE)
			continue;
		if(err == 0)
			return 0;
		debug_printf("tex_transform (%s): failed, error %d", codecs[i]->name, err);
		CHECK_ERR(err);
	}

	// last chance
	return plain_transform(t, transforms);
}


int tex_wrap(uint w, uint h, uint bpp, uint flags, void* img, Tex* t)
{
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	// note: we can't use tex_img_size because that requires all
	// Tex fields to be valid, but this calculation must be done first.
	const size_t img_size = w*h*bpp/8;
	t->hm = mem_assign(img, img_size, 0, 0, 0, 0, 0);
	RETURN_ERR(t->hm);

	// the exact value of img is lost, since the handle references the
	// allocation and disregards the offset within it given by <img>.
	// fix that up by setting t->ofs.
	void* reported_ptr = mem_get_ptr(t->hm);
	t->ofs = (u8*)img - (u8*)reported_ptr;

	CHECK_TEX(t);
	return 0;
}


int tex_write(const char* fn, uint w, uint h, uint bpp, uint flags, void* in_img)
{
	if(validate_format(bpp, flags) != 0)
		return ERR_TEX_FMT_INVALID;

	Tex t;
	RETURN_ERR(tex_wrap(w, h, bpp, flags, in_img, &t));

	// we could be clever here and avoid the extra alloc if our current
	// memory block ensued from the same kind of texture file. this is
	// most likely the case if in_img == <hm's user pointer> + c->hdr_size(0).
	// advantage is avoiding 
	DynArray da;
	const size_t max_out_size = (w*h*4 + 256*KiB)*2;
	CHECK_ERR(da_alloc(&da, max_out_size));

	const TexCodecVTbl* c;
	CHECK_ERR(tex_codec_for_filename(fn, &c));

	const size_t rounded_size = round_up(da.cur_size, FILE_BLOCK_SIZE);
	// encode
	int err = c->encode(&t, &da);
	if(err < 0)
	{
		debug_printf("tex_write (%s): %d", c->name, err);
		debug_warn("tex_writefailed");
		goto fail;
	}
	CHECK_ERR(da_set_size(&da, rounded_size));
	WARN_ERR(vfs_store(fn, da.base, da.pos));
	err = 0;

fail:
	WARN_ERR(da_free(&da));
	WARN_ERR(mem_free_h(t.hm));
	return err;
}
