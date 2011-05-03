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
 * PNG codec using libpng.
 */

#include "precompiled.h"

#include "lib/external_libraries/png.h"

#include "lib/byte_order.h"
#include "tex_codec.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/timer.h"

#if MSC_VERSION

// squelch "dtor / setjmp interaction" warnings.
// all attempts to resolve the underlying problem failed; apparently
// the warning is generated if setjmp is used at all in C++ mode.
// (png_*_impl have no code that would trigger ctors/dtors, nor are any
// called in their prolog/epilog code).
# pragma warning(disable: 4611)

#endif	// MSC_VERSION


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------


// pass data from PNG file in memory to libpng
static void io_read(png_struct* png_ptr, u8* data, png_size_t length)
{
	DynArray* da = (DynArray*)png_get_io_ptr(png_ptr);
	if(da_read(da, data, length) != 0)
		png_error(png_ptr, "io_read failed");
}


// write libpng output to PNG file
static void io_write(png_struct* png_ptr, u8* data, png_size_t length)
{
	DynArray* da = (DynArray*)png_get_io_ptr(png_ptr);
	if(da_append(da, data, length) != 0)
		png_error(png_ptr, "io_write failed");
}


static void io_flush(png_structp UNUSED(png_ptr))
{
}



//-----------------------------------------------------------------------------

static Status png_transform(Tex* UNUSED(t), size_t UNUSED(transforms))
{
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr).

// split out of png_decode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static Status png_decode_impl(DynArray* da, png_structp png_ptr, png_infop info_ptr, Tex* t)
{
	png_set_read_fn(png_ptr, da, io_read);

	// read header and determine format
	png_read_info(png_ptr, info_ptr);
	png_uint_32 w, h;
	int bit_depth, colour_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &colour_type, 0, 0, 0);
	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);
	const u32 bpp = (u32)(pitch/w * 8);

	size_t flags = 0;
	if(bpp == 32)
		flags |= TEX_ALPHA;
	if(colour_type == PNG_COLOR_TYPE_GRAY)
		flags |= TEX_GREY;

	// make sure format is acceptable
	if(bit_depth != 8)
		WARN_RETURN(ERR::TEX_NOT_8BIT_PRECISION);
	if(colour_type & PNG_COLOR_MASK_PALETTE)
		WARN_RETURN(ERR::TEX_INVALID_COLOR_TYPE);

	const size_t img_size = pitch * h;
	shared_ptr<u8> data;
	AllocateAligned(data, img_size, pageSize);

	std::vector<RowPtr> rows = tex_codec_alloc_rows(data.get(), h, pitch, TEX_TOP_DOWN, 0);
	png_read_image(png_ptr, (png_bytepp)&rows[0]);
	png_read_end(png_ptr, info_ptr);

	// success; make sure all data was consumed.
	ENSURE(da->pos == da->cur_size);

	// store image info
	t->data  = data;
	t->dataSize = img_size;
	t->ofs   = 0;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	return INFO::OK;
}


// split out of png_encode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static Status png_encode_impl(Tex* t, png_structp png_ptr, png_infop info_ptr, DynArray* da)
{
	const png_uint_32 w = (png_uint_32)t->w, h = (png_uint_32)t->h;
	const size_t pitch = w * t->bpp / 8;

	int colour_type;
	switch(t->flags & (TEX_GREY|TEX_ALPHA))
	{
	case TEX_GREY|TEX_ALPHA:
		colour_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
	case TEX_GREY:
		colour_type = PNG_COLOR_TYPE_GRAY;
		break;
	case TEX_ALPHA:
		colour_type = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	default:
		colour_type = PNG_COLOR_TYPE_RGB;
		break;
	}

	png_set_write_fn(png_ptr, da, io_write, io_flush);
	png_set_IHDR(png_ptr, info_ptr, w, h, 8, colour_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	u8* data = tex_get_data(t);
	std::vector<RowPtr> rows = tex_codec_alloc_rows(data, h, pitch, t->flags, TEX_TOP_DOWN);

	// PNG is native RGB.
	const int png_transforms = (t->flags & TEX_BGR)? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

	png_set_rows(png_ptr, info_ptr, (png_bytepp)&rows[0]);
	png_write_png(png_ptr, info_ptr, png_transforms, 0);

	return INFO::OK;
}



static bool png_is_hdr(const u8* file)
{
	// don't use png_sig_cmp, so we don't pull in libpng for
	// this check alone (it might not actually be used).
	return *(u32*)file == FOURCC('\x89','P','N','G');
}


static bool png_is_ext(const OsPath& extension)
{
	return extension == L".png";
}


static size_t png_hdr_size(const u8* UNUSED(file))
{
	return 0;	// libpng returns decoded image data; no header
}


TIMER_ADD_CLIENT(tc_png_decode);

// limitation: palette images aren't supported
static Status png_decode(DynArray* RESTRICT da, Tex* RESTRICT t)
{
TIMER_ACCRUE(tc_png_decode);

	Status ret = ERR::FAIL;
	png_infop info_ptr = 0;

	// allocate PNG structures; use default stderr and longjmp error handlers
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		WARN_RETURN(ERR::FAIL);
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
		goto fail;
	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
		goto fail;
	}

	ret = png_decode_impl(da, png_ptr, info_ptr, t);

fail:
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	
	return ret;
}


// limitation: palette images aren't supported
static Status png_encode(Tex* RESTRICT t, DynArray* RESTRICT da)
{
	Status ret = ERR::FAIL;
	png_infop info_ptr = 0;

	// allocate PNG structures; use default stderr and longjmp error handlers
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		WARN_RETURN(ERR::FAIL);
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
		goto fail;

	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
		goto fail;
	}

	ret = png_encode_impl(t, png_ptr, info_ptr, da);

	// shared cleanup
fail:
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return ret;
}

TEX_CODEC_REGISTER(png);
