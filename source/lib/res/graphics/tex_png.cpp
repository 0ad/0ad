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


struct PngMemFile
{
	const u8* p;
	size_t size;

	size_t pos;		// 0-initialized if no initializer
};

// pass data from PNG file in memory to libpng
static void png_io_read(png_struct* png_ptr, u8* data, png_size_t length)
{
	PngMemFile* f = (PngMemFile*)png_ptr->io_ptr;

	void* src = (u8*)(f->p + f->pos);

	// make sure there's enough new data remaining in the buffer
	f->pos += length;
	if(f->pos > f->size)
		png_error(png_ptr, "png_read: not enough data to satisfy request!");

	memcpy(data, src, length);
}


// write libpng output to PNG file
static void png_io_write(png_struct* png_ptr, u8* data, png_size_t length)
{
	void* p = (void*)data;
	Handle hf = *(Handle*)png_ptr->io_ptr;
	if(vfs_io(hf, length, &p) != (ssize_t)length)
		png_error(png_ptr, "png_write: !");
}


static void png_io_flush(png_structp)
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
	u8*& img, RowArray& rows, const char** perr_msg)
{
	PngMemFile f = 	{ file, file_size };
	png_set_read_fn(png_ptr, &f, png_io_read);

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
	Handle img_hm;
	// cannot free old t->hm until after png_read_end,
	// but need to set to this handle afterwards => need tmp var.
	img = (u8*)mem_alloc(img_size, 64*KiB, 0, &img_hm);
	if(!img)
		return ERR_NO_MEM;

	CHECK_ERR(tex_codec_alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows));

	png_read_image(png_ptr, (png_bytepp)rows);
	png_read_end(png_ptr, info_ptr);

	// success; make sure all data was consumed.
	debug_assert(f.p == file && f.size == file_size && f.pos == f.size);

	// store image info
	// .. transparently switch handles - free the old (compressed)
	//    buffer and replace it with the decoded-image memory handle.
	mem_free_h(t->hm);
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
	RowArray& rows, Handle& hf, const char** perr_msg)
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
	png_set_write_fn(png_ptr, &hf, png_io_write, png_io_flush);
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

	int err = -1;

	// freed when ret is reached:
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0;

	// freed if fail is reached:
	u8* img = 0;	// decompressed image memory

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
		// reached if PNG triggered a longjmp
fail:
		mem_free(img);
		goto ret;
	}

	err = png_decode_impl(t, file, file_size, png_ptr, info_ptr, img, rows, perr_msg);
	if(err < 0)
		goto fail;

	// shared cleanup
ret:
	free(rows);

	if(png_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return err;
}


// limitation: palette images aren't supported
static int png_encode(const char* ext, Tex* t, u8** out, size_t* UNUSED(out_size), const char** perr_msg)
{
	if(stricmp(ext, "png"))
		return TEX_CODEC_CANNOT_HANDLE;

	int err = -1;

	// freed when ret is reached.
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0; 
	Handle hf = 0;

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
		// reached if libpng triggered a longjmp
fail:
		// currently no extra resources that need to be freed
		goto ret;
	}

	err = png_encode_impl(t, png_ptr, info_ptr, rows, hf, perr_msg);
	if(err < 0)
		goto fail;

	// shared cleanup
ret:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(rows);
	vfs_close(hf);

	return err;
}

TEX_CODEC_REGISTER(png);
