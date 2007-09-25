/**
 * =========================================================================
 * File        : tex.cpp
 * Project     : 0 A.D.
 * Description : support routines for 2d texture access/writing.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "tex.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

#include "lib/timer.h"
#include "lib/bits.h"
#include "lib/res/res.h"

#include "tex_codec.h"


ERROR_ASSOCIATE(ERR::TEX_FMT_INVALID, "Invalid/unsupported texture format", -1);
ERROR_ASSOCIATE(ERR::TEX_INVALID_COLOR_TYPE, "Invalid color type", -1);
ERROR_ASSOCIATE(ERR::TEX_NOT_8BIT_PRECISION, "Not 8-bit channel precision", -1);
ERROR_ASSOCIATE(ERR::TEX_INVALID_LAYOUT, "Unsupported texel layout, e.g. right-to-left", -1);
ERROR_ASSOCIATE(ERR::TEX_COMPRESSED, "Unsupported texture compression", -1);
ERROR_ASSOCIATE(WARN::TEX_INVALID_DATA, "Warning: invalid texel data encountered", -1);
ERROR_ASSOCIATE(ERR::TEX_INVALID_SIZE, "Texture size is incorrect", -1);
ERROR_ASSOCIATE(INFO::TEX_CODEC_CANNOT_HANDLE, "Texture codec cannot handle the given format", -1);


//-----------------------------------------------------------------------------
// validation
//-----------------------------------------------------------------------------

// be careful not to use other tex_* APIs here because they call us.
LibError tex_validate(const Tex* t)
{
	// pixel data
	size_t tex_file_size;
	void* tex_file = mem_get_ptr(t->hm, &tex_file_size);
	// .. only check validity if the image is still in memory.
	//    (e.g. ogl_tex frees the data after uploading to GL)
	if(tex_file)
	{
		// file size smaller than header+pixels.
		// possible causes: texture file header is invalid,
		// or file wasn't loaded completely.
		if(tex_file_size < t->ofs + t->w*t->h*t->bpp/8)
			WARN_RETURN(ERR::_1);
	}

	// bits per pixel
	// (we don't bother checking all values; a sanity check is enough)
	if(t->bpp % 4 || t->bpp > 32)
		WARN_RETURN(ERR::_2);

	// flags
	// .. DXT value
	const uint dxt = t->flags & TEX_DXT;
	if(dxt != 0 && dxt != 1 && dxt != DXT1A && dxt != 3 && dxt != 5)
		WARN_RETURN(ERR::_3);
	// .. orientation
	const uint orientation = t->flags & TEX_ORIENTATION;
	if(orientation == (TEX_BOTTOM_UP|TEX_TOP_DOWN))
		WARN_RETURN(ERR::_4);

	return INFO::OK;
}

#define CHECK_TEX(t) RETURN_ERR(tex_validate(t))


// check if the given texture format is acceptable: 8bpp grey,
// 24bpp color or 32bpp color+alpha (BGR / upside down are permitted).
// basically, this is the "plain" format understood by all codecs and
// tex_codec_plain_transform.
LibError tex_validate_plain_format(uint bpp, uint flags)
{
	const bool alpha   = (flags & TEX_ALPHA  ) != 0;
	const bool grey    = (flags & TEX_GREY   ) != 0;
	const bool dxt     = (flags & TEX_DXT    ) != 0;
	const bool mipmaps = (flags & TEX_MIPMAPS) != 0;

	if(dxt || mipmaps)
		WARN_RETURN(ERR::TEX_FMT_INVALID);

	// grey must be 8bpp without alpha, or it's invalid.
	if(grey)
	{
		if(bpp == 8 && !alpha)
			return INFO::OK;
		WARN_RETURN(ERR::TEX_FMT_INVALID);
	}

	if(bpp == 24 && !alpha)
		return INFO::OK;
	if(bpp == 32 && alpha)
		return INFO::OK;

	WARN_RETURN(ERR::TEX_FMT_INVALID);
}


//-----------------------------------------------------------------------------
// mipmaps
//-----------------------------------------------------------------------------

void tex_util_foreach_mipmap(uint w, uint h, uint bpp, const u8* RESTRICT data,
	int levels_to_skip, uint data_padding, MipmapCB cb, void* RESTRICT cbData)
{
	uint level_w = w, level_h = h;
	const u8* level_data = data;

	// we iterate through the loop (necessary to skip over image data),
	// but do not actually call back until the requisite number of
	// levels have been skipped (i.e. level == 0).
	int level = -(int)levels_to_skip;
	if(levels_to_skip == -1)
		level = 0;

	// until at level 1x1:
	for(;;)
	{
		// used to skip past this mip level in <data>
		const size_t level_data_size = (size_t)(round_up(level_w, data_padding) * round_up(level_h, data_padding) * bpp/8);

		if(level >= 0)
			cb((uint)level, level_w, level_h, level_data, level_data_size, cbData);

		level_data += level_data_size;

		// 1x1 reached - done
		if(level_w == 1 && level_h == 1)
			break;
		level_w /= 2;
		level_h /= 2;
		// if the texture is non-square, one of the dimensions will become
		// 0 before the other. to satisfy OpenGL's expectations, change it
		// back to 1.
		if(level_w == 0) level_w = 1;
		if(level_h == 0) level_h = 1;
		level++;

		// special case: no mipmaps, we were only supposed to call for
		// the base level
		if(levels_to_skip == TEX_BASE_LEVEL_ONLY)
			break;
	}
}


struct CreateLevelData
{
	uint num_components;

	uint prev_level_w;
	uint prev_level_h;
	const u8* prev_level_data;
	size_t prev_level_data_size;
};

// uses 2x2 box filter
static void create_level(uint level, uint level_w, uint level_h,
	const u8* RESTRICT level_data, size_t level_data_size, void* RESTRICT cbData)
{
	CreateLevelData* cld = (CreateLevelData*)cbData;
	const size_t src_w = cld->prev_level_w;
	const size_t src_h = cld->prev_level_h;
	const u8* src = cld->prev_level_data;
	u8* dst = (u8*)level_data;

	// base level - must be copied over from source buffer
	if(level == 0)
	{
		debug_assert(level_data_size == cld->prev_level_data_size);
		cpu_memcpy(dst, src, level_data_size);
	}
	else
	{
		const uint num_components = cld->num_components;
		const size_t dx = num_components, dy = dx*src_w;

		// special case: image is too small for 2x2 filter
		if(cld->prev_level_w == 1 || cld->prev_level_h == 1)
		{
			// image is either a horizontal or vertical line.
			// their memory layout is the same (packed pixels), so no special
			// handling is needed; just pick max dimension.
			for(uint y = 0; y < std::max(src_w, src_h); y += 2)
			{
				for(uint i = 0; i < num_components; i++)
				{
					*dst++ = (src[0]+src[dx]+1)/2;
					src += 1;
				}

				src += dx;	// skip to next pixel (since box is 2x2)
			}
		}
		// normal
		else
		{
			for(uint y = 0; y < src_h; y += 2)
			{
				for(uint x = 0; x < src_w; x += 2)
				{
					for(uint i = 0; i < num_components; i++)
					{
						*dst++ = (src[0]+src[dx]+src[dy]+src[dx+dy]+2)/4;
						src += 1;
					}

					src += dx;	// skip to next pixel (since box is 2x2)
				}

				src += dy;	// skip to next row (since box is 2x2)
			}
		}

		debug_assert(dst == level_data + level_data_size);
		debug_assert(src == cld->prev_level_data + cld->prev_level_data_size);
	}

	cld->prev_level_data = level_data;
	cld->prev_level_data_size = level_data_size;
	cld->prev_level_w = level_w;
	cld->prev_level_h = level_h;
}


static LibError add_mipmaps(Tex* t, uint w, uint h, uint bpp,
	void* new_data, size_t data_size)
{
	// this code assumes the image is of POT dimension; we don't
	// go to the trouble of implementing image scaling because
	// the only place this is used (ogl_tex_upload) requires POT anyway.
	if(!is_pow2(w) || !is_pow2(h))
		WARN_RETURN(ERR::TEX_INVALID_SIZE);
	t->flags |= TEX_MIPMAPS;	// must come before tex_img_size!
	const size_t mipmap_size = tex_img_size(t);
	Handle hm;
	const u8* mipmap_data = (const u8*)mem_alloc(mipmap_size, 4*KiB, 0, &hm);
	if(!mipmap_data)
		WARN_RETURN(ERR::NO_MEM);
	CreateLevelData cld = { bpp/8, w, h, (const u8*)new_data, data_size };
	tex_util_foreach_mipmap(w, h, bpp, mipmap_data, 0, 1, create_level, &cld);
	mem_free_h(t->hm);
	t->hm = hm;
	t->ofs = 0;

	return INFO::OK;
}


//-----------------------------------------------------------------------------
// pixel format conversion (transformation)
//-----------------------------------------------------------------------------

TIMER_ADD_CLIENT(tc_plain_transform);

// handles BGR and row flipping in "plain" format (see below).
//
// called by codecs after they get their format-specific transforms out of
// the way. note that this approach requires several passes over the image,
// but is much easier to maintain than providing all<->all conversion paths.
//
// somewhat optimized (loops are hoisted, cache associativity accounted for)
static LibError plain_transform(Tex* t, uint transforms)
{
TIMER_ACCRUE(tc_plain_transform);

	// (this is also called directly instead of through ogl_tex, so
	// we need to validate)
	CHECK_TEX(t);

	// extract texture info
	const uint w = t->w, h = t->h, bpp = t->bpp, flags = t->flags;
	u8* const data = tex_get_data(t);
	const size_t data_size = tex_img_size(t);

	// sanity checks (not errors, we just can't handle these cases)
	// .. unknown transform
	if(transforms & ~(TEX_BGR|TEX_ORIENTATION|TEX_MIPMAPS))
		return INFO::TEX_CODEC_CANNOT_HANDLE;
	// .. data is not in "plain" format
	RETURN_ERR(tex_validate_plain_format(bpp, flags));
	// .. nothing to do
	if(!transforms)
		return INFO::OK;

	// allocate copy of the image data.
	// rationale: L1 cache is typically A2 => swapping in-place with a
	// line buffer leads to thrashing. we'll assume the whole texture*2
	// fits in cache, allocate a copy, and transfer directly from there.
	//
	// this is necessary even when not flipping because the initial Tex.hm
	// (which is a FileIOBuf) is read-only.
	Handle hm;
	void* new_data = mem_alloc(data_size, 4*KiB, 0, &hm);
	if(!new_data)
		WARN_RETURN(ERR::NO_MEM);
	cpu_memcpy(new_data, data, data_size);

	// setup row source/destination pointers (simplifies outer loop)
	u8* dst = (u8*)new_data;
	const u8* src = (const u8*)new_data;
	const size_t pitch = w * bpp/8;
	// .. avoid y*pitch multiply in row loop; instead, add row_ofs.
	ssize_t row_ofs = (ssize_t)pitch;

	// flipping rows (0,1,2 -> 2,1,0)
	if(transforms & TEX_ORIENTATION)
	{
		src = (const u8*)data+data_size-pitch;	// last row
		row_ofs = -(ssize_t)pitch;
	}

	// no BGR convert necessary
	if(!(transforms & TEX_BGR))
	{
		for(uint y = 0; y < h; y++)
		{
			cpu_memcpy(dst, src, pitch);
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

	mem_free_h(t->hm);
	t->hm = hm;
	t->ofs = 0;

	if(!(t->flags & TEX_MIPMAPS) && transforms & TEX_MIPMAPS)
		RETURN_ERR(add_mipmaps(t, w, h, bpp, new_data, data_size));

	CHECK_TEX(t);
	return INFO::OK;
}


TIMER_ADD_CLIENT(tc_transform);

// change <t>'s pixel format by flipping the state of all TEX_* flags
// that are set in transforms.
LibError tex_transform(Tex* t, uint transforms)
{
	TIMER_ACCRUE(tc_transform);
	CHECK_TEX(t);

	const uint target_flags = t->flags ^ transforms;
	uint remaining_transforms;
	for(;;)
	{
		remaining_transforms = target_flags ^ t->flags;
		// we're finished (all required transforms have been done)
		if(remaining_transforms == 0)
			return INFO::OK;

		LibError ret = tex_codec_transform(t, remaining_transforms);
		if(ret != INFO::OK)
			break;
	}

	// last chance
	RETURN_ERR(plain_transform(t, remaining_transforms));
	return INFO::OK;
}


// change <t>'s pixel format to the new format specified by <new_flags>.
// (note: this is equivalent to tex_transform(t, t->flags^new_flags).
LibError tex_transform_to(Tex* t, uint new_flags)
{
	// tex_transform takes care of validating <t>
	const uint transforms = t->flags ^ new_flags;
	return tex_transform(t, transforms);
}


//-----------------------------------------------------------------------------
// image orientation
//-----------------------------------------------------------------------------

// see "Default Orientation" in docs.

static int global_orientation = TEX_TOP_DOWN;

// set the orientation (either TEX_BOTTOM_UP or TEX_TOP_DOWN) to which
// all loaded images will automatically be converted
// (excepting file formats that don't specify their orientation, i.e. DDS).
void tex_set_global_orientation(int o)
{
	debug_assert(o == TEX_TOP_DOWN || o == TEX_BOTTOM_UP);
	global_orientation = o;
}


static void flip_to_global_orientation(Tex* t)
{
	// (can't use normal CHECK_TEX due to void return)
	WARN_ERR(tex_validate(t));

	uint orientation = t->flags & TEX_ORIENTATION;
	// if codec knows which way around the image is (i.e. not DDS):
	if(orientation)
	{
		// flip image if necessary
		uint transforms = orientation ^ global_orientation;
		WARN_ERR(plain_transform(t, transforms));
	}

	// indicate image is at global orientation. this is still done even
	// if the codec doesn't know: the default orientation should be chosen
	// to make that work correctly (see "Default Orientation" in docs).
	t->flags = (t->flags & ~TEX_ORIENTATION) | global_orientation;

	// (can't use normal CHECK_TEX due to void return)
	WARN_ERR(tex_validate(t));
}


// indicate if the orientation specified by <src_flags> matches
// dst_orientation (if the latter is 0, then the global_orientation).
// (we ask for src_flags instead of src_orientation so callers don't
// have to mask off TEX_ORIENTATION)
bool tex_orientations_match(uint src_flags, uint dst_orientation)
{
	const uint src_orientation = src_flags & TEX_ORIENTATION;
	if(dst_orientation == 0)
		dst_orientation = global_orientation;
	return (src_orientation == dst_orientation);
}


//-----------------------------------------------------------------------------
// misc. API
//-----------------------------------------------------------------------------

// indicate if <filename>'s extension is that of a texture format
// supported by tex_load. case-insensitive.
//
// rationale: tex_load complains if the given file is of an
// unsupported type. this API allows users to preempt that warning
// (by checking the filename themselves), and also provides for e.g.
// enumerating only images in a file picker.
// an alternative might be a flag to suppress warning about invalid files,
// but this is open to misuse.
bool tex_is_known_extension(const char* filename)
{
	const TexCodecVTbl* dummy;
	// found codec for it => known extension
	if(tex_codec_for_filename(filename, &dummy) == INFO::OK)
		return true;

	return false;
}


// store the given image data into a Tex object; this will be as if
// it had been loaded via tex_load.
//
// rationale: support for in-memory images is necessary for
//   emulation of glCompressedTexImage2D and useful overall.
//   however, we don't want to  provide an alternate interface for each API;
//   these would have to be changed whenever fields are added to Tex.
//   instead, provide one entry point for specifying images.
// note: since we do not know how <img> was allocated, the caller must do
//   so (after calling tex_free, which is required regardless of alloc type).
//
// we need only add bookkeeping information and "wrap" it in
// our Tex struct, hence the name.
LibError tex_wrap(uint w, uint h, uint bpp, uint flags, void* img, Tex* t)
{
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	// note: we can't use tex_img_size because that requires all
	// Tex fields to be valid, but this calculation must be done first.
	const size_t img_size = w*h*bpp/8;
	t->hm = mem_wrap(img, img_size, 0, 0, 0, 0, 0, (void*)&tex_wrap);
	RETURN_ERR(t->hm);

	// the exact value of img is lost, since the handle references the
	// allocation and disregards the offset within it given by <img>.
	// fix that up by setting t->ofs.
	void* reported_ptr = mem_get_ptr(t->hm);
	t->ofs = (u8*)img - (u8*)reported_ptr;

	CHECK_TEX(t);
	return INFO::OK;
}


// free all resources associated with the image and make further
// use of it impossible.
LibError tex_free(Tex* t)
{
	// do not validate <t> - this is called from tex_load if loading
	// failed, so not all fields may be valid.

	LibError ret = mem_free_h(t->hm);

	// do not zero out the fields! that could lead to trouble since
	// ogl_tex_upload followed by ogl_tex_free is legit, but would
	// cause OglTex_validate to fail (since its Tex.w is == 0).
	return ret;
}


//-----------------------------------------------------------------------------
// getters
//-----------------------------------------------------------------------------

// returns a pointer to the image data (pixels), taking into account any
// header(s) that may come before it. see Tex.hm comment above.
u8* tex_get_data(const Tex* t)
{
	// (can't use normal CHECK_TEX due to u8* return value)
	WARN_ERR(tex_validate(t));

	u8* p = (u8*)mem_get_ptr(t->hm);
	if(!p)
		return 0;
	return p + t->ofs;
}


static void add_level_size(uint UNUSED(level), uint UNUSED(level_w), uint UNUSED(level_h),
	const u8* RESTRICT UNUSED(level_data), size_t level_data_size, void* RESTRICT cbData)
{
	size_t* ptotal_size = (size_t*)cbData;
	*ptotal_size += level_data_size;
}

// return total byte size of the image pixels. (including mipmaps!)
// this is preferable to calculating manually because it's
// less error-prone (e.g. confusing bits_per_pixel with bytes).
size_t tex_img_size(const Tex* t)
{
	// (can't use normal CHECK_TEX due to size_t return value)
	WARN_ERR(tex_validate(t));

	const int levels_to_skip = (t->flags & TEX_MIPMAPS)? 0 : TEX_BASE_LEVEL_ONLY;
	const uint data_padding = (t->flags & TEX_DXT)? 4 : 1;
	size_t out_size = 0;
	tex_util_foreach_mipmap(t->w, t->h, t->bpp, 0, levels_to_skip,
		data_padding, add_level_size, &out_size);
	return out_size;
}


// return the minimum header size (i.e. offset to pixel data) of the
// file format indicated by <fn>'s extension (that is all it need contain:
// e.g. ".bmp"). returns 0 on error (i.e. no codec found).
// this can be used to optimize calls to tex_write: when allocating the
// buffer that will hold the image, allocate this much extra and
// pass the pointer as base+hdr_size. this allows writing the header
// directly into the output buffer and makes for zero-copy IO.
size_t tex_hdr_size(const char* fn)
{
	const TexCodecVTbl* c;
	CHECK_ERR(tex_codec_for_filename(fn, &c));
	return c->hdr_size(0);
}


//-----------------------------------------------------------------------------
// read/write from memory and disk
//-----------------------------------------------------------------------------

LibError tex_decode(const u8* data, size_t data_size, MEM_DTOR dtor, Tex* t)
{
	const TexCodecVTbl* c;
	RETURN_ERR(tex_codec_for_header(data, data_size, &c));

	// make sure the entire header is available
	const size_t min_hdr_size = c->hdr_size(0);
	if(data_size < min_hdr_size)
		WARN_RETURN(ERR::RES_INCOMPLETE_HEADER);
	const size_t hdr_size = c->hdr_size(data);
	if(data_size < hdr_size)
		WARN_RETURN(ERR::RES_INCOMPLETE_HEADER);

	// wrap pointer into a Handle; required for Tex.hm.
	// rationale: a Handle protects the texture memory from being
	// accidentally free-d.
	Handle hm = mem_wrap((void*)data, data_size, 0, 0, 0, dtor, 0, (void*)tex_decode);

	t->hm = hm;
	t->ofs = hdr_size;

	// for orthogonality, encode and decode both receive the memory as a
	// DynArray. package data into one and free it again after decoding:
	DynArray da;
	RETURN_ERR(da_wrap_fixed(&da, (u8*)data, data_size));

	RETURN_ERR(c->decode(&da, t));

	// note: not reached if decode fails. that's not a problem;
	// this call just zeroes <da> and could be left out.
	(void)da_free(&da);

	// sanity checks
	if(!t->w || !t->h || t->bpp > 32)
		WARN_RETURN(ERR::TEX_FMT_INVALID);
	// .. note: can't use data_size - decode may have decompressed the image.
	size_t hm_size;
	(void)mem_get_ptr(t->hm, &hm_size);
	if(hm_size < t->ofs + tex_img_size(t))
		WARN_RETURN(ERR::TEX_INVALID_SIZE);

	flip_to_global_orientation(t);

	return INFO::OK;
}


LibError tex_encode(Tex* t, const char* fn, DynArray* da)
{
	CHECK_TEX(t);
	CHECK_ERR(tex_validate_plain_format(t->bpp, t->flags));

	// we could be clever here and avoid the extra alloc if our current
	// memory block ensued from the same kind of texture file. this is
	// most likely the case if in_img == <hm's user pointer> + c->hdr_size(0).
	// this would make for zero-copy IO.

	const size_t max_out_size = tex_img_size(t)*4 + 256*KiB;
	RETURN_ERR(da_alloc(da, max_out_size));

	const TexCodecVTbl* c;
	CHECK_ERR(tex_codec_for_filename(fn, &c));

	// encode into <da>
	LibError err = c->encode(t, da);
	if(err < 0)
	{
		(void)da_free(da);
		WARN_RETURN(err);
	}

	return INFO::OK;
}



// MEM_DTOR -> file_buf_free adapter (used for mem_wrap-ping FileIOBuf)
static void file_buf_dtor(void* p, size_t UNUSED(size), uintptr_t UNUSED(ctx))
{
	(void)file_buf_free((FileIOBuf)p);
}

// load the specified image from file into the given Tex object.
// currently supports BMP, TGA, JPG, JP2, PNG, DDS.
LibError tex_load(const char* fn, Tex* t, uint file_flags)
{
	// load file
	FileIOBuf file; size_t file_size;
	RETURN_ERR(vfs_load(fn, file, file_size, file_flags));

	LibError ret = tex_decode(file, file_size, file_buf_dtor, t);
	if(ret < 0)
	{
		(void)tex_free(t);
		WARN_RETURN(ret);
	}

	// do not free hm! it either still holds the image data (i.e. texture
	// wasn't compressed) or was replaced by a new buffer for the image data.

	CHECK_TEX(t);
	return INFO::OK;
}


// write the specified texture to disk.
// note: <t> cannot be made const because the image may have to be
// transformed to write it out in the format determined by <fn>'s extension.
LibError tex_write(Tex* t, const char* fn)
{
	DynArray da;
	RETURN_ERR(tex_encode(t, fn, &da));

	// write to disk
	LibError ret = INFO::OK;
	{
	const size_t sector_aligned_size = round_up(da.cur_size, file_sector_size);
	(void)da_set_size(&da, sector_aligned_size);
	const ssize_t bytes_written = vfs_store(fn, da.base, da.pos);
	if(bytes_written > 0)
		debug_assert(bytes_written == (ssize_t)da.pos);
	else
		ret = (LibError)bytes_written;
	}

	(void)da_free(&da);
	return ret;
}
