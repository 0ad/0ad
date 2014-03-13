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
 * support routines for 2d texture access/writing.
 */

#include "precompiled.h"
#include "tex.h"

#include <math.h>
#include <stdlib.h>
#include <algorithm>

#include "lib/timer.h"
#include "lib/bits.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/sysdep/cpu.h"

#include "tex_codec.h"


static const StatusDefinition texStatusDefinitions[] = {
	{ ERR::TEX_FMT_INVALID, L"Invalid/unsupported texture format" },
	{ ERR::TEX_INVALID_COLOR_TYPE, L"Invalid color type" },
	{ ERR::TEX_NOT_8BIT_PRECISION, L"Not 8-bit channel precision" },
	{ ERR::TEX_INVALID_LAYOUT, L"Unsupported texel layout, e.g. right-to-left" },
	{ ERR::TEX_COMPRESSED, L"Unsupported texture compression" },
	{ WARN::TEX_INVALID_DATA, L"Warning: invalid texel data encountered" },
	{ ERR::TEX_INVALID_SIZE, L"Texture size is incorrect" },
	{ INFO::TEX_CODEC_CANNOT_HANDLE, L"Texture codec cannot handle the given format" }
};
STATUS_ADD_DEFINITIONS(texStatusDefinitions);


//-----------------------------------------------------------------------------
// validation
//-----------------------------------------------------------------------------

// be careful not to use other tex_* APIs here because they call us.
Status Tex::validate() const
{
	if(m_Flags & TEX_UNDEFINED_FLAGS)
		WARN_RETURN(ERR::_1);

	// pixel data (only check validity if the image is still in memory;
	// ogl_tex frees the data after uploading to GL)
	if(m_Data)
	{
		// file size smaller than header+pixels.
		// possible causes: texture file header is invalid,
		// or file wasn't loaded completely.
		if(m_DataSize < m_Ofs + m_Width*m_Height*m_Bpp/8)
			WARN_RETURN(ERR::_2);
	}

	// bits per pixel
	// (we don't bother checking all values; a sanity check is enough)
	if(m_Bpp % 4 || m_Bpp > 32)
		WARN_RETURN(ERR::_3);

	// flags
	// .. DXT value
	const size_t dxt = m_Flags & TEX_DXT;
	if(dxt != 0 && dxt != 1 && dxt != DXT1A && dxt != 3 && dxt != 5)
		WARN_RETURN(ERR::_4);
	// .. orientation
	const size_t orientation = m_Flags & TEX_ORIENTATION;
	if(orientation == (TEX_BOTTOM_UP|TEX_TOP_DOWN))
		WARN_RETURN(ERR::_5);

	return INFO::OK;
}

#define CHECK_TEX(t) RETURN_STATUS_IF_ERR((t->validate()))


// check if the given texture format is acceptable: 8bpp grey,
// 24bpp color or 32bpp color+alpha (BGR / upside down are permitted).
// basically, this is the "plain" format understood by all codecs and
// tex_codec_plain_transform.
Status tex_validate_plain_format(size_t bpp, size_t flags)
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

void tex_util_foreach_mipmap(size_t w, size_t h, size_t bpp, const u8* pixels, int levels_to_skip, size_t data_padding, MipmapCB cb, void* RESTRICT cbData)
{
	ENSURE(levels_to_skip >= 0 || levels_to_skip == TEX_BASE_LEVEL_ONLY);

	size_t level_w = w, level_h = h;
	const u8* level_data = pixels;

	// we iterate through the loop (necessary to skip over image data),
	// but do not actually call back until the requisite number of
	// levels have been skipped (i.e. level == 0).
	int level = (levels_to_skip == TEX_BASE_LEVEL_ONLY)? 0 : -levels_to_skip;

	// until at level 1x1:
	for(;;)
	{
		// used to skip past this mip level in <data>
		const size_t level_dataSize = (size_t)(round_up(level_w, data_padding) * round_up(level_h, data_padding) * bpp/8);

		if(level >= 0)
			cb((size_t)level, level_w, level_h, level_data, level_dataSize, cbData);

		level_data += level_dataSize;

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
	size_t num_components;

	size_t prev_level_w;
	size_t prev_level_h;
	const u8* prev_level_data;
	size_t prev_level_dataSize;
};

// uses 2x2 box filter
static void create_level(size_t level, size_t level_w, size_t level_h, const u8* RESTRICT level_data, size_t level_dataSize, void* RESTRICT cbData)
{
	CreateLevelData* cld = (CreateLevelData*)cbData;
	const size_t src_w = cld->prev_level_w;
	const size_t src_h = cld->prev_level_h;
	const u8* src = cld->prev_level_data;
	u8* dst = (u8*)level_data;

	// base level - must be copied over from source buffer
	if(level == 0)
	{
		ENSURE(level_dataSize == cld->prev_level_dataSize);
		memcpy(dst, src, level_dataSize);
	}
	else
	{
		const size_t num_components = cld->num_components;
		const size_t dx = num_components, dy = dx*src_w;

		// special case: image is too small for 2x2 filter
		if(cld->prev_level_w == 1 || cld->prev_level_h == 1)
		{
			// image is either a horizontal or vertical line.
			// their memory layout is the same (packed pixels), so no special
			// handling is needed; just pick max dimension.
			for(size_t y = 0; y < std::max(src_w, src_h); y += 2)
			{
				for(size_t i = 0; i < num_components; i++)
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
			for(size_t y = 0; y < src_h; y += 2)
			{
				for(size_t x = 0; x < src_w; x += 2)
				{
					for(size_t i = 0; i < num_components; i++)
					{
						*dst++ = (src[0]+src[dx]+src[dy]+src[dx+dy]+2)/4;
						src += 1;
					}

					src += dx;	// skip to next pixel (since box is 2x2)
				}

				src += dy;	// skip to next row (since box is 2x2)
			}
		}

		ENSURE(dst == level_data + level_dataSize);
		ENSURE(src == cld->prev_level_data + cld->prev_level_dataSize);
	}

	cld->prev_level_data = level_data;
	cld->prev_level_dataSize = level_dataSize;
	cld->prev_level_w = level_w;
	cld->prev_level_h = level_h;
}


static Status add_mipmaps(Tex* t, size_t w, size_t h, size_t bpp, void* newData, size_t dataSize)
{
	// this code assumes the image is of POT dimension; we don't
	// go to the trouble of implementing image scaling because
	// the only place this is used (ogl_tex_upload) requires POT anyway.
	if(!is_pow2(w) || !is_pow2(h))
		WARN_RETURN(ERR::TEX_INVALID_SIZE);
	t->m_Flags |= TEX_MIPMAPS;	// must come before tex_img_size!
	const size_t mipmap_size = t->img_size();
	shared_ptr<u8> mipmapData;
	AllocateAligned(mipmapData, mipmap_size);
	CreateLevelData cld = { bpp/8, w, h, (const u8*)newData, dataSize };
	tex_util_foreach_mipmap(w, h, bpp, mipmapData.get(), 0, 1, create_level, &cld);
	t->m_Data = mipmapData;
	t->m_DataSize = mipmap_size;
	t->m_Ofs = 0;

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
static Status plain_transform(Tex* t, size_t transforms)
{
TIMER_ACCRUE(tc_plain_transform);

	// (this is also called directly instead of through ogl_tex, so
	// we need to validate)
	CHECK_TEX(t);

	// extract texture info
	const size_t w = t->m_Width, h = t->m_Height, bpp = t->m_Bpp;
	const size_t flags = t->m_Flags;
	u8* const srcStorage = t->get_data();

	// sanity checks (not errors, we just can't handle these cases)
	// .. unknown transform
	if(transforms & ~(TEX_BGR|TEX_ORIENTATION|TEX_MIPMAPS|TEX_ALPHA))
		return INFO::TEX_CODEC_CANNOT_HANDLE;
	// .. data is not in "plain" format
	RETURN_STATUS_IF_ERR(tex_validate_plain_format(bpp, flags));
	// .. nothing to do
	if(!transforms)
		return INFO::OK;

	const size_t srcSize = t->img_size();
	size_t dstSize = srcSize;

	if(transforms & TEX_ALPHA)
	{
		// add alpha channel
		if(bpp == 24)
		{
			dstSize = (srcSize / 3) * 4;
			t->m_Bpp = 32;
		}
		// remove alpha channel
		else if(bpp == 32)
		{
			return INFO::TEX_CODEC_CANNOT_HANDLE;
		}
		// can't have alpha with grayscale
		else
		{
			return INFO::TEX_CODEC_CANNOT_HANDLE;
		}
	}

	// allocate copy of the image data.
	// rationale: L1 cache is typically A2 => swapping in-place with a
	// line buffer leads to thrashing. we'll assume the whole texture*2
	// fits in cache, allocate a copy, and transfer directly from there.
	//
	// this is necessary even when not flipping because the initial data
	// is read-only.
	shared_ptr<u8> dstStorage;
	AllocateAligned(dstStorage, dstSize);

	// setup row source/destination pointers (simplifies outer loop)
	u8* dst = (u8*)dstStorage.get();
	const u8* src;
	const size_t pitch = w * bpp/8;	// source bpp (not necessarily dest bpp)
	// .. avoid y*pitch multiply in row loop; instead, add row_ofs.
	ssize_t row_ofs = (ssize_t)pitch;

	// flipping rows (0,1,2 -> 2,1,0)
	if(transforms & TEX_ORIENTATION)
	{
		src = (const u8*)srcStorage+srcSize-pitch;	// last row
		row_ofs = -(ssize_t)pitch;
	}
	// adding/removing alpha channel (can't convert in-place)
	else if(transforms & TEX_ALPHA)
	{
		src = (const u8*)srcStorage;
	}
	// do other transforms in-place
	else
	{
		src = (const u8*)dstStorage.get();
		memcpy(dstStorage.get(), srcStorage, srcSize);
	}

	// no conversion necessary
	if(!(transforms & (TEX_BGR | TEX_ALPHA)))
	{
		if(src != dst)	// avoid overlapping memcpy if not flipping rows
		{
			for(size_t y = 0; y < h; y++)
			{
				memcpy(dst, src, pitch);
				dst += pitch;
				src += row_ofs;
			}
		}
	}
	// RGB -> BGRA, BGR -> RGBA
	else if(bpp == 24 && (transforms & TEX_ALPHA) && (transforms & TEX_BGR))
	{
		for(size_t y = 0; y < h; y++)
		{
			for(size_t x = 0; x < w; x++)
			{
				// need temporaries in case src == dst (i.e. not flipping)
				const u8 b = src[0], g = src[1], r = src[2];
				dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = 0xFF;
				dst += 4;
				src += 3;
			}
			src += row_ofs - pitch;	// flip? previous row : stay
		}
	}
	// RGB -> RGBA, BGR -> BGRA
	else if(bpp == 24 && (transforms & TEX_ALPHA) && !(transforms & TEX_BGR))
	{
		for(size_t y = 0; y < h; y++)
		{
			for(size_t x = 0; x < w; x++)
			{
				// need temporaries in case src == dst (i.e. not flipping)
				const u8 r = src[0], g = src[1], b = src[2];
				dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = 0xFF;
				dst += 4;
				src += 3;
			}
			src += row_ofs - pitch;	// flip? previous row : stay
		}
	}
	// RGB <-> BGR
	else if(bpp == 24 && !(transforms & TEX_ALPHA))
	{
		for(size_t y = 0; y < h; y++)
		{
			for(size_t x = 0; x < w; x++)
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
	else if(bpp == 32 && !(transforms & TEX_ALPHA))
	{
		for(size_t y = 0; y < h; y++)
		{
			for(size_t x = 0; x < w; x++)
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
	else
	{
		debug_warn(L"unsupported transform");
		return INFO::TEX_CODEC_CANNOT_HANDLE;
	}

	t->m_Data = dstStorage;
	t->m_DataSize = dstSize;
	t->m_Ofs = 0;

	if(!(t->m_Flags & TEX_MIPMAPS) && transforms & TEX_MIPMAPS)
		RETURN_STATUS_IF_ERR(add_mipmaps(t, w, h, bpp, dstStorage.get(), dstSize));

	CHECK_TEX(t);
	return INFO::OK;
}


TIMER_ADD_CLIENT(tc_transform);

// change the pixel format by flipping the state of all TEX_* flags
// that are set in transforms.
Status Tex::transform(size_t transforms)
{
	TIMER_ACCRUE(tc_transform);
	CHECK_TEX(this);

	const size_t target_flags = m_Flags ^ transforms;
	size_t remaining_transforms;
	for(;;)
	{
		remaining_transforms = target_flags ^ m_Flags;
		// we're finished (all required transforms have been done)
		if(remaining_transforms == 0)
			return INFO::OK;

		Status ret = tex_codec_transform(this, remaining_transforms);
		if(ret != INFO::OK)
			break;
	}

	// last chance
	RETURN_STATUS_IF_ERR(plain_transform(this, remaining_transforms));
	return INFO::OK;
}


// change the pixel format to the new format specified by <new_flags>.
// (note: this is equivalent to transform(t, t->flags^new_flags).
Status Tex::transform_to(size_t new_flags)
{
	// transform takes care of validating
	const size_t transforms = m_Flags ^ new_flags;
	return transform(transforms);
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
	ENSURE(o == TEX_TOP_DOWN || o == TEX_BOTTOM_UP);
	global_orientation = o;
}


static void flip_to_global_orientation(Tex* t)
{
	// (can't use normal CHECK_TEX due to void return)
	WARN_IF_ERR(t->validate());

	size_t orientation = t->m_Flags & TEX_ORIENTATION;
	// if codec knows which way around the image is (i.e. not DDS):
	if(orientation)
	{
		// flip image if necessary
		size_t transforms = orientation ^ global_orientation;
		WARN_IF_ERR(plain_transform(t, transforms));
	}

	// indicate image is at global orientation. this is still done even
	// if the codec doesn't know: the default orientation should be chosen
	// to make that work correctly (see "Default Orientation" in docs).
	t->m_Flags = (t->m_Flags & ~TEX_ORIENTATION) | global_orientation;

	// (can't use normal CHECK_TEX due to void return)
	WARN_IF_ERR(t->validate());
}


// indicate if the orientation specified by <src_flags> matches
// dst_orientation (if the latter is 0, then the global_orientation).
// (we ask for src_flags instead of src_orientation so callers don't
// have to mask off TEX_ORIENTATION)
bool tex_orientations_match(size_t src_flags, size_t dst_orientation)
{
	const size_t src_orientation = src_flags & TEX_ORIENTATION;
	if(dst_orientation == 0)
		dst_orientation = global_orientation;
	return (src_orientation == dst_orientation);
}


//-----------------------------------------------------------------------------
// misc. API
//-----------------------------------------------------------------------------

// indicate if <filename>'s extension is that of a texture format
// supported by Tex::load. case-insensitive.
//
// rationale: Tex::load complains if the given file is of an
// unsupported type. this API allows users to preempt that warning
// (by checking the filename themselves), and also provides for e.g.
// enumerating only images in a file picker.
// an alternative might be a flag to suppress warning about invalid files,
// but this is open to misuse.
bool tex_is_known_extension(const VfsPath& pathname)
{
	const ITexCodec* dummy;
	// found codec for it => known extension
	const OsPath extension = pathname.Extension();
	if(tex_codec_for_filename(extension, &dummy) == INFO::OK)
		return true;

	return false;
}


// store the given image data into a Tex object; this will be as if
// it had been loaded via Tex::load.
//
// rationale: support for in-memory images is necessary for
//   emulation of glCompressedTexImage2D and useful overall.
//   however, we don't want to  provide an alternate interface for each API;
//   these would have to be changed whenever fields are added to Tex.
//   instead, provide one entry point for specifying images.
//
// we need only add bookkeeping information and "wrap" it in
// our Tex struct, hence the name.
Status Tex::wrap(size_t w, size_t h, size_t bpp, size_t flags, const shared_ptr<u8>& data, size_t ofs)
{
	m_Width    = w;
	m_Height   = h;
	m_Bpp      = bpp;
	m_Flags    = flags;
	m_Data     = data;
	m_DataSize = ofs + w*h*bpp/8;
	m_Ofs      = ofs;

	CHECK_TEX(this);
	return INFO::OK;
}


// free all resources associated with the image and make further
// use of it impossible.
void Tex::free()
{
	// do not validate - this is called from Tex::load if loading
	// failed, so not all fields may be valid.

	m_Data.reset();

	// do not zero out the fields! that could lead to trouble since
	// ogl_tex_upload followed by ogl_tex_free is legit, but would
	// cause OglTex_validate to fail (since its Tex.w is == 0).
}


//-----------------------------------------------------------------------------
// getters
//-----------------------------------------------------------------------------

// returns a pointer to the image data (pixels), taking into account any
// header(s) that may come before it.
u8* Tex::get_data()
{
	// (can't use normal CHECK_TEX due to u8* return value)
	WARN_IF_ERR(validate());

	u8* p = m_Data.get();
	if(!p)
		return 0;
	return p + m_Ofs;
}

// returns colour of 1x1 mipmap level
u32 Tex::get_average_colour() const
{
	// require mipmaps
	if(!(m_Flags & TEX_MIPMAPS))
		return 0;

	// find the total size of image data
	size_t size = img_size();

	// compute the size of the last (1x1) mipmap level
	const size_t data_padding = (m_Flags & TEX_DXT)? 4 : 1;
	size_t last_level_size = (size_t)(data_padding * data_padding * m_Bpp/8);

	// construct a new texture based on the current one,
	// but only include the last mipmap level
	// do this so that we can use the general conversion methods for the pixel data
	Tex basetex = *this;
	uint8_t *data = new uint8_t[last_level_size];
	memcpy(data, m_Data.get() + m_Ofs + size - last_level_size, last_level_size);
	boost::shared_ptr<uint8_t> sdata(data);
	basetex.wrap(1, 1, m_Bpp, m_Flags, sdata, 0);

	// convert to BGRA
	WARN_IF_ERR(basetex.transform_to(TEX_BGR | TEX_ALPHA));

	// extract components into u32
	ENSURE(basetex.m_DataSize >= basetex.m_Ofs+4);
	u8 b = basetex.m_Data.get()[basetex.m_Ofs];
	u8 g = basetex.m_Data.get()[basetex.m_Ofs+1];
	u8 r = basetex.m_Data.get()[basetex.m_Ofs+2];
	u8 a = basetex.m_Data.get()[basetex.m_Ofs+3];
	return b + (g << 8) + (r << 16) + (a << 24);
}


static void add_level_size(size_t UNUSED(level), size_t UNUSED(level_w), size_t UNUSED(level_h), const u8* RESTRICT UNUSED(level_data), size_t level_dataSize, void* RESTRICT cbData)
{
	size_t* ptotal_size = (size_t*)cbData;
	*ptotal_size += level_dataSize;
}

// return total byte size of the image pixels. (including mipmaps!)
// this is preferable to calculating manually because it's
// less error-prone (e.g. confusing bits_per_pixel with bytes).
size_t Tex::img_size() const
{
	// (can't use normal CHECK_TEX due to size_t return value)
	WARN_IF_ERR(validate());

	const int levels_to_skip = (m_Flags & TEX_MIPMAPS)? 0 : TEX_BASE_LEVEL_ONLY;
	const size_t data_padding = (m_Flags & TEX_DXT)? 4 : 1;
	size_t out_size = 0;
	tex_util_foreach_mipmap(m_Width, m_Height, m_Bpp, 0, levels_to_skip, data_padding, add_level_size, &out_size);
	return out_size;
}


// return the minimum header size (i.e. offset to pixel data) of the
// file format indicated by <fn>'s extension (that is all it need contain:
// e.g. ".bmp"). returns 0 on error (i.e. no codec found).
// this can be used to optimize calls to tex_write: when allocating the
// buffer that will hold the image, allocate this much extra and
// pass the pointer as base+hdr_size. this allows writing the header
// directly into the output buffer and makes for zero-copy IO.
size_t tex_hdr_size(const VfsPath& filename)
{
	const ITexCodec* c;
	
	const OsPath extension = filename.Extension();
	WARN_RETURN_STATUS_IF_ERR(tex_codec_for_filename(extension, &c));
	return c->hdr_size(0);
}


//-----------------------------------------------------------------------------
// read/write from memory and disk
//-----------------------------------------------------------------------------

Status Tex::decode(const shared_ptr<u8>& Data, size_t DataSize)
{
	const ITexCodec* c;
	RETURN_STATUS_IF_ERR(tex_codec_for_header(Data.get(), DataSize, &c));

	// make sure the entire header is available
	const size_t min_hdr_size = c->hdr_size(0);
	if(DataSize < min_hdr_size)
		WARN_RETURN(ERR::TEX_INCOMPLETE_HEADER);
	const size_t hdr_size = c->hdr_size(Data.get());
	if(DataSize < hdr_size)
		WARN_RETURN(ERR::TEX_INCOMPLETE_HEADER);

	m_Data = Data;
	m_DataSize = DataSize;
	m_Ofs = hdr_size;

	RETURN_STATUS_IF_ERR(c->decode((rpU8)Data.get(), DataSize, this));

	// sanity checks
	if(!m_Width || !m_Height || m_Bpp > 32)
		WARN_RETURN(ERR::TEX_FMT_INVALID);
	if(m_DataSize < m_Ofs + img_size())
		WARN_RETURN(ERR::TEX_INVALID_SIZE);

	flip_to_global_orientation(this);

	CHECK_TEX(this);

	return INFO::OK;
}


Status Tex::encode(const OsPath& extension, DynArray* da)
{
	CHECK_TEX(this);
	WARN_RETURN_STATUS_IF_ERR(tex_validate_plain_format(m_Bpp, m_Flags));

	// we could be clever here and avoid the extra alloc if our current
	// memory block ensued from the same kind of texture file. this is
	// most likely the case if in_img == get_data() + c->hdr_size(0).
	// this would make for zero-copy IO.

	const size_t max_out_size = img_size()*4 + 256*KiB;
	RETURN_STATUS_IF_ERR(da_alloc(da, max_out_size));

	const ITexCodec* c;
	WARN_RETURN_STATUS_IF_ERR(tex_codec_for_filename(extension, &c));

	// encode into <da>
	Status err = c->encode(this, da);
	if(err < 0)
	{
		(void)da_free(da);
		WARN_RETURN(err);
	}

	return INFO::OK;
}
