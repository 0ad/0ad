/**
 * =========================================================================
 * File        : tex_png.cpp
 * Project     : 0 A.D.
 * Description : PNG codec using libpng.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004 Jan Wassenberg
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

// include libpng header. we also prevent it from including windows.h, which
// would conflict with other headers. instead, WINAPI is defined here.
#if OS_WIN
# define _WINDOWS_
# define WINAPI __stdcall
# define WINAPIV __cdecl
#endif	// OS_WIN
#include <png.h>

#include "lib/byte_order.h"
#include "lib/res/res.h"
#include "tex_codec.h"
#include "lib/timer.h"

#if MSC_VERSION

// squelch "dtor / setjmp interaction" warnings.
// all attempts to resolve the underlying problem failed; apparently
// the warning is generated if setjmp is used at all in C++ mode.
// (png_*_impl have no code that would trigger ctors/dtors, nor are any
// called in their prolog/epilog code).
# pragma warning(disable: 4611)

// pull in the appropriate debug/release library
# ifdef NDEBUG
#  pragma comment(lib, "libpng13.lib")
# else
#  pragma comment(lib, "libpng13d.lib")
# endif	// NDEBUG

#endif	// MSC_VERSION


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------


// pass data from PNG file in memory to libpng
static void io_read(png_struct* png_ptr, u8* data, png_size_t length)
{
	DynArray* da = (DynArray*)png_ptr->io_ptr;
	if(da_read(da, data, length) != 0)
		png_error(png_ptr, "io_read failed");
}


// write libpng output to PNG file
static void io_write(png_struct* png_ptr, u8* data, png_size_t length)
{
	DynArray* da = (DynArray*)png_ptr->io_ptr;
	if(da_append(da, data, length) != 0)
		png_error(png_ptr, "io_write failed");
}


static void io_flush(png_structp UNUSED(png_ptr))
{
}



//-----------------------------------------------------------------------------

static LibError png_transform(Tex* UNUSED(t), uint UNUSED(transforms))
{
	return INFO_TEX_CODEC_CANNOT_HANDLE;
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr).

// split out of png_decode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static LibError png_decode_impl(DynArray* da,
	png_structp png_ptr, png_infop info_ptr,
	Handle& img_hm, RowArray& rows, Tex* t)
{
	png_set_read_fn(png_ptr, da, io_read);

	// read header and determine format
	png_read_info(png_ptr, info_ptr);
	png_uint_32 w, h;
	int bit_depth, colour_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &colour_type, 0, 0, 0);
	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);
	const u32 bpp = (u32)(pitch/w * 8);

	uint flags = 0;
	if(bpp == 32)
		flags |= TEX_ALPHA;
	if(colour_type == PNG_COLOR_TYPE_GRAY)
		flags |= TEX_GREY;

	// make sure format is acceptable
	if(bit_depth != 8)
		WARN_RETURN(ERR_TEX_NOT_8BIT_PRECISION);
	if(colour_type & PNG_COLOR_MASK_PALETTE)
		WARN_RETURN(ERR_TEX_INVALID_COLOR_TYPE);

	const size_t img_size = pitch * h;
	u8* img = (u8*)mem_alloc(img_size, 64*KiB, 0, &img_hm);
	if(!img)
		WARN_RETURN(ERR_NO_MEM);

	RETURN_ERR(tex_codec_alloc_rows(img, h, pitch, TEX_TOP_DOWN, 0, rows));

	png_read_image(png_ptr, (png_bytepp)rows);
	png_read_end(png_ptr, info_ptr);

	// success; make sure all data was consumed.
	debug_assert(da->pos == da->cur_size);

	// store image info
	// .. transparently switch handles - free the old (compressed)
	//    buffer and replace it with the decoded-image memory handle.
	mem_free_h(t->hm);	// must come after png_read_end
	t->hm    = img_hm;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	return INFO_OK;
}


// split out of png_encode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static LibError png_encode_impl(Tex* t,
	png_structp png_ptr, png_infop info_ptr,
	RowArray& rows, DynArray* da)
{
	const png_uint_32 w = t->w, h = t->h;
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
	RETURN_ERR(tex_codec_alloc_rows(data, h, pitch, t->flags, TEX_TOP_DOWN, rows));

	// PNG is native RGB.
	const int png_transforms = (t->flags & TEX_BGR)? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

	png_set_rows(png_ptr, info_ptr, (png_bytepp)rows);
	png_write_png(png_ptr, info_ptr, png_transforms, 0);

	return INFO_OK;
}



static bool png_is_hdr(const u8* file)
{
	// don't use png_sig_cmp, so we don't pull in libpng for
	// this check alone (it might not actually be used).
	return *(u32*)file == FOURCC('\x89','P','N','G');
}


static bool png_is_ext(const char* ext)
{
	return !stricmp(ext, "png");
}


static size_t png_hdr_size(const u8* UNUSED(file))
{
	return 0;	// libpng returns decoded image data; no header
}


TIMER_ADD_CLIENT(tc_png_decode);

// limitation: palette images aren't supported
static LibError png_decode(DynArray* restrict da, Tex* restrict t)
{
TIMER_ACCRUE(tc_png_decode);

	LibError err = ERR_FAIL;
	// freed when ret is reached:
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0;

	// freed if fail is reached:
	Handle img_hm = 0;	// decompressed image memory

	// allocate PNG structures; use default stderr and longjmp error handlers
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		goto fail;
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
		goto fail;

	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
fail:
		mem_free_h(img_hm);
		goto ret;
	}

	err = png_decode_impl(da, png_ptr, info_ptr, img_hm, rows, t);
	if(err < 0)
		goto fail;

ret:
	if(png_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(rows);
	return err;
}


// limitation: palette images aren't supported
static LibError png_encode(Tex* restrict t, DynArray* restrict da)
{
	LibError err = ERR_FAIL;
	// freed when ret is reached:
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0;

	// allocate PNG structures; use default stderr and longjmp error handlers
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		goto fail;
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
		goto fail;

	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
fail:
		// currently no cleanup to be done.
		goto ret;
	}

	err = png_encode_impl(t, png_ptr, info_ptr, rows, da);
	if(err < 0)
		goto fail;

	// shared cleanup
ret:
	png_destroy_write_struct(&png_ptr, &info_ptr);
	free(rows);
	return err;
}

TEX_CODEC_REGISTER(png);
