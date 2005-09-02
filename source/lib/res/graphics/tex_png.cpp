#include "precompiled.h"

// include libpng header. we also prevent it from including windows.h, which
// would conflict with other headers. instead, WINAPI is defined here.
#if OS_WIN
# define _WINDOWS_
# define WINAPI __stdcall
# define WINAPIV __cdecl
#endif	// OS_WIN
#include "libpng13/png.h"

#include "lib/byte_order.h"
#include "lib/res/res.h"
#include "tex_codec.h"



#if MSC_VERSION

// squelch "dtor / setjmp interaction" warnings.
// all attempts to resolve the underlying problem failed; apparently
// the warning is generated if setjmp is used at all in C++ mode.
// (png_decode has no code that would trigger ctors/dtors, nor are any
// called in its prolog/epilog code).
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
	MemSource* ms = (MemSource*)png_ptr->io_ptr;

	void* src = (u8*)(ms->p + ms->pos);

	// make sure there's enough new data remaining in the buffer
	ms->pos += length;
	if(ms->pos > ms->size)
		png_error(png_ptr, "png_read: not enough data to satisfy request!");

	memcpy(data, src, length);
}


// write libpng output to PNG file
static void io_write(png_struct* png_ptr, u8* data, png_size_t length)
{
	void* p = (void*)data;
	Handle hf = *(Handle*)png_ptr->io_ptr;
	if(vfs_io(hf, length, &p) != (ssize_t)length)
		png_error(png_ptr, "png_write: !");
}


static void io_flush(png_structp)
{
}



//-----------------------------------------------------------------------------

static int png_transform(Tex* t, int new_flags)
{
	return TEX_CODEC_CANNOT_HANDLE;
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr).

// split out of png_decode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static int png_decode_impl(Tex* t, u8* file, size_t file_size,
	png_structp png_ptr, png_infop info_ptr,
	Handle& img_hm, RowArray& rows, const char** perr_msg)
{
	MemSource ms = 	{ file, file_size, 0 };
	png_set_read_fn(png_ptr, &ms, io_read);

	// read header and determine format
	png_read_info(png_ptr, info_ptr);
	png_uint_32 w, h;
	int bit_depth, colour_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &colour_type, 0, 0, 0);
	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);
	const u32 bpp = (u32)(pitch / w * 8);

	int flags = 0;
	if(bpp == 32)
		flags |= TEX_ALPHA;
	if(colour_type == PNG_COLOR_TYPE_GRAY)
		flags |= TEX_GREY;

	// make sure format is acceptable
	const char* err = 0;
	if(bit_depth != 8)
		err = "channel precision != 8 bits";
	if(colour_type & PNG_COLOR_MASK_PALETTE)
		err = "colour type is invalid (must be direct colour)";
	if(err)
	{
		*perr_msg = err;
		return -1;
	}

	const size_t img_size = pitch * h;
	u8* img = (u8*)mem_alloc(img_size, 64*KiB, 0, &img_hm);
	if(!img)
		return ERR_NO_MEM;

	CHECK_ERR(tex_codec_alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows));

	png_read_image(png_ptr, (png_bytepp)rows);
	png_read_end(png_ptr, info_ptr);

	// success; make sure all data was consumed.
	debug_assert(ms.p == file && ms.size == file_size && ms.pos == ms.size);

	// store image info
	// .. transparently switch handles - free the old (compressed)
	//    buffer and replace it with the decoded-image memory handle.
	mem_free_h(t->hm);	// must come after png_read_end
	t->hm    = img_hm;
	t->ofs   = 0;	// libpng returns decoded image data; no header
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	return 0;
}


// split out of png_encode to simplify resource cleanup and avoid
// "dtor / setjmp interaction" warning.
static int png_encode_impl(Tex* t,
	png_structp png_ptr, png_infop info_ptr,
	RowArray& rows, const char** perr_msg)
{
	UNUSED2(perr_msg);	// we don't produce any error messages ATM.

	const int png_transforms = (t->flags & TEX_BGR)? PNG_TRANSFORM_BGR : PNG_TRANSFORM_IDENTITY;
	// PNG is native RGB.

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
/*
	hf = vfs_open(fn, FILE_WRITE|FILE_NO_AIO);
	CHECK_ERR(hf);
	png_set_write_fn(png_ptr, &hf, io_write, io_flush);
*/
	png_set_IHDR(png_ptr, info_ptr, w, h, 8, colour_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	u8* data = tex_get_data(t);
	CHECK_ERR(tex_codec_alloc_rows(data, h, pitch, TEX_TOP_DOWN, rows));

	png_set_rows(png_ptr, info_ptr, (png_bytepp)rows);
	png_write_png(png_ptr, info_ptr, png_transforms, 0);

	return 0;
}



// limitation: palette images aren't supported
static int png_decode(u8* file, size_t file_size, Tex* t, const char** perr_msg)
{
	// don't use png_sig_cmp, so we don't pull in libpng for
	// this check alone (it might not actually be used).
	if(*(u32*)file != FOURCC('\x89','P','N','G'))
		return TEX_CODEC_CANNOT_HANDLE;

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
		// libpng longjmp-ed here after an error, or code below failed.
fail:
		mem_free_h(img_hm);
		goto ret;
	}

	int err = png_decode_impl(t, file, file_size, png_ptr, info_ptr, img_hm, rows, perr_msg);
	if(err < 0)
		goto fail;

ret:
	if(png_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(rows);
	return err;
}


// limitation: palette images aren't supported
static int png_encode(const char* ext, Tex* t, u8** out, size_t* UNUSED(out_size), const char** perr_msg)
{
	if(stricmp(ext, "png"))
		return TEX_CODEC_CANNOT_HANDLE;

	// freed when ret is reached:
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0;

	// free when fail is reached:
	// (mem buffer)

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
fail:
		// libjpg longjmp-ed here after an error, or code below failed.
		goto ret;
	}

	int err = png_encode_impl(t, png_ptr, info_ptr, rows, perr_msg);
	if(err < 0)
		goto fail;

	// shared cleanup
ret:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(rows);

	return err;
}

TEX_CODEC_REGISTER(png);
