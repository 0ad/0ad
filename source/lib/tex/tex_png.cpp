/* Copyright (C) 2018 Wildfire Games.
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

class MemoryStream
{
public:
	MemoryStream(u8* RESTRICT data, size_t size)
		: data(data), size(size), pos(0)
	{
	}

	size_t RemainingSize() const
	{
		ASSERT(pos <= size);
		return size-pos;
	}

	void CopyTo(u8* RESTRICT dst, size_t dstSize)
	{
		memcpy(dst, data+pos, dstSize);
		pos += dstSize;
	}

private:
	u8* RESTRICT data;
	size_t size;
	size_t pos;
};


// pass data from PNG file in memory to libpng
static void io_read(png_struct* png_ptr, u8* RESTRICT data, png_size_t size)
{
	MemoryStream* stream = (MemoryStream*)png_get_io_ptr(png_ptr);
	if(stream->RemainingSize() < size)
	{
		png_error(png_ptr, "PNG: not enough input");
		return;
	}

	stream->CopyTo(data, size);
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

Status TexCodecPng::transform(Tex* UNUSED(t), size_t UNUSED(transforms)) const
{
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr).

// split out of png_decode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static Status png_decode_impl(MemoryStream* stream, png_structp png_ptr, png_infop info_ptr, Tex* t)
{
	png_set_read_fn(png_ptr, stream, io_read);

	// read header and determine format
	png_read_info(png_ptr, info_ptr);
	png_uint_32 w, h;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, &interlace_type, 0, 0);

	// (The following is based on GdkPixbuf's PNG image loader)

	// Convert the following images to 8-bit RGB/RGBA:
	// * indexed colors
	// * grayscale with alpha
	// * transparency header
	// * bit depth of 16 or less than 8
	// * interlaced
	if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
		png_set_expand(png_ptr);
	else if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	else if (bit_depth < 8)
		png_set_expand(png_ptr);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	if (interlace_type != PNG_INTERLACE_NONE)
		png_set_interlace_handling(png_ptr);

	// Update info after transformations
	png_read_update_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, &interlace_type, 0, 0);

	// make sure format is acceptable:
	// * non-zero dimensions
	// * 8-bit depth
	// * RGB, RGBA, or grayscale
	// * 1, 3 or 4 channels
	if (w == 0 || h == 0)
		WARN_RETURN(ERR::TEX_INVALID_SIZE);
	if (bit_depth != 8)
		WARN_RETURN(ERR::TEX_NOT_8BIT_PRECISION);
	if (!(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY))
		WARN_RETURN(ERR::TEX_INVALID_COLOR_TYPE);

	const int channels = png_get_channels(png_ptr, info_ptr);
	if (!(channels == 3 || channels == 4 || channels == 1))
		WARN_RETURN(ERR::TEX_FMT_INVALID);

	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);
	const u32 bpp = (u32)(pitch / w * 8);

	size_t flags = 0;
	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		flags |= TEX_ALPHA;
	if (color_type == PNG_COLOR_TYPE_GRAY)
		flags |= TEX_GREY;

	const size_t img_size = pitch * h;
	shared_ptr<u8> data;
	AllocateAligned(data, img_size, g_PageSize);

	std::vector<RowPtr> rows = tex_codec_alloc_rows(data.get(), h, pitch, TEX_TOP_DOWN, 0);
	png_read_image(png_ptr, (png_bytepp)&rows[0]);
	png_read_end(png_ptr, info_ptr);

	// success; make sure all data was consumed.
	ENSURE(stream->RemainingSize() == 0);

	// store image info and validate
	return t->wrap(w,h,bpp,flags,data,0);
}


// split out of png_encode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static Status png_encode_impl(Tex* t, png_structp png_ptr, png_infop info_ptr, DynArray* da)
{
	const png_uint_32 w = (png_uint_32)t->m_Width, h = (png_uint_32)t->m_Height;
	const size_t pitch = w * t->m_Bpp / 8;

	int color_type;
	switch(t->m_Flags & (TEX_GREY|TEX_ALPHA))
	{
	case TEX_GREY|TEX_ALPHA:
		color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
	case TEX_GREY:
		color_type = PNG_COLOR_TYPE_GRAY;
		break;
	case TEX_ALPHA:
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	default:
		color_type = PNG_COLOR_TYPE_RGB;
		break;
	}

	png_set_write_fn(png_ptr, da, io_write, io_flush);
	png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	u8* data = t->get_data();
	std::vector<RowPtr> rows = tex_codec_alloc_rows(data, h, pitch, t->m_Flags, TEX_TOP_DOWN);

	// PNG is native RGB.
	const int png_transforms = (t->m_Flags & TEX_BGR)? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;

	png_set_rows(png_ptr, info_ptr, (png_bytepp)&rows[0]);
	png_write_png(png_ptr, info_ptr, png_transforms, 0);

	return INFO::OK;
}



bool TexCodecPng::is_hdr(const u8* file) const
{
	// don't use png_sig_cmp, so we don't pull in libpng for
	// this check alone (it might not actually be used).
	return *(u32*)file == FOURCC('\x89','P','N','G');
}


bool TexCodecPng::is_ext(const OsPath& extension) const
{
	return extension == L".png";
}


size_t TexCodecPng::hdr_size(const u8* UNUSED(file)) const
{
	return 0;	// libpng returns decoded image data; no header
}

static void user_warning_fn(png_structp UNUSED(png_ptr), png_const_charp warning_msg)
{
	// Suppress this warning because it's useless and occurs on a large number of files
	// see http://trac.wildfiregames.com/ticket/2184
	if (strcmp(warning_msg, "iCCP: known incorrect sRGB profile") == 0)
		return;
	debug_printf("libpng warning: %s\n", warning_msg);
}

TIMER_ADD_CLIENT(tc_png_decode);

// limitation: palette images aren't supported
Status TexCodecPng::decode(u8* RESTRICT data, size_t size, Tex* RESTRICT t) const
{
TIMER_ACCRUE(tc_png_decode);

	png_infop info_ptr = 0;

	// allocate PNG structures; use default stderr and longjmp error handler, use custom
	// warning handler to filter out useless messages
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, user_warning_fn);
	if(!png_ptr)
		WARN_RETURN(ERR::FAIL);
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		WARN_RETURN(ERR::NO_MEM);
	}
	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		WARN_RETURN(ERR::FAIL);
	}

	MemoryStream stream(data, size);
	Status ret = png_decode_impl(&stream, png_ptr, info_ptr, t);

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return ret;
}


// limitation: palette images aren't supported
Status TexCodecPng::encode(Tex* RESTRICT t, DynArray* RESTRICT da) const
{
	png_infop info_ptr = 0;

	// allocate PNG structures; use default stderr and longjmp error handlers
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		WARN_RETURN(ERR::FAIL);
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		WARN_RETURN(ERR::NO_MEM);
	}
	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		// libpng longjmps here after an error
		png_destroy_write_struct(&png_ptr, &info_ptr);
		WARN_RETURN(ERR::FAIL);
	}

	Status ret = png_encode_impl(t, png_ptr, info_ptr, da);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	return ret;
}
