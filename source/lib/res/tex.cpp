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

#include "lib.h"
#include "res.h"

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include <algorithm>

// supported formats:
//#define NO_DDS
//#define NO_TGA
//#define NO_BMP
//#define NO_PNG
#define NO_JP2
//#define NO_RAW


#ifndef NO_JP2
#include <jasper/jasper.h>
#endif


#ifndef NO_PNG
# ifdef _WIN32
#  define _WINDOWS_			// prevent libpng from including windows.h
#  define WINAPI __stdcall	// .. and define what it needs
#  define WINAPIV __cdecl
#  include <libpng10/png.h>
#  ifdef _MSC_VER
#   pragma comment(lib, "libpng10.lib")
#  endif
# else	// _WIN32
#  include <png.h>
# endif	// _WIN32
#endif	// NO_PNG




#define CODEC(name) { name##_fmt, name##_ext, name##_decode, name##_encode, #name}

struct Codec
{
	// size is at least 4.
	bool(*is_fmt)(const u8* p, size_t size);
	bool(*is_ext)(const char* ext);
	int(*decode)(TexInfo* t, const char* fn, const u8* file, size_t file_size);
	int(*encode)(TexInfo* t, const char* fn, const u8* img, size_t img_size);

	// no get_output_size() function and central mem alloc / file write,
	// because codecs that write via external lib or output compressed files
	// don't know the output size beforehand.

	const char* name;
};


//////////////////////////////////////////////////////////////////////////////
//
// DDS
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_DDS


// defs modified from ddraw header


#pragma pack(push, 1)

// DDPIXELFORMAT.dwFlags
#define DDPF_ALPHAPIXELS	0x00000001	

typedef struct
{
    u32 dwSize;                       // size of structure (32)
    u32 dwFlags;                      // indicates which fields are valid
    u32 dwFourCC;                     // (DDPF_FOURCC) FOURCC code, "DXTn"
	u32 dwReserved1[5];               // reserved
}
DDPIXELFORMAT;

typedef struct
{
    u32 dwCaps[4];
}
DDSCAPS2;

// DDSURFACEDESC2.dwFlags
#define DDSD_HEIGHT	        0x00000002	
#define DDSD_WIDTH	        0x00000004	
#define DDSD_PIXELFORMAT	0x00001000	
#define DDSD_MIPMAPCOUNT	0x00020000	

typedef struct
{
    u32 dwSize;                       // size of structure (124)
    u32 dwFlags;                      // indicates which fields are valid
    u32 dwHeight;                     // height of main image (pixels)
    u32 dwWidth;                      // width of main image (pixels)
    u32 dwLinearSize;                 // (DDSD_LINEARSIZE): total image size
    u32 dwDepth;                      // (DDSD_DEPTH) vol. textures: vol. depth
    u32 dwMipMapCount;                // (DDSD_MIPMAPCOUNT) total # levels
    u32 dwReserved1[11];              // reserved
    DDPIXELFORMAT ddpfPixelFormat;    // pixel format description of the surface
    DDSCAPS2 ddsCaps;                 // direct draw surface capabilities
    u32 dwReserved2;                  // reserved
}
DDSURFACEDESC2;

#pragma pack(pop)


static inline bool dds_fmt(const u8* ptr, size_t size)
{
	UNUSED(size)	// only need first 4 chars
	
	return *(u32*)ptr == FOURCC('D','D','S',' ');
}


static inline bool dds_ext(const char* const ext)
{
	return !strcmp(ext, ".dds");
}


static int dds_decode(TexInfo* t, const char* fn, const u8* file, size_t file_size)
{
	const char* err = 0;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(file+4);
	const size_t hdr_size = 4+sizeof(DDSURFACEDESC2);

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("dds_decode: %s: %s\n", fn, err);
		return -1;
	}

	const u32 sd_size   = read_le32(&surf->dwSize);
	const u32 sd_flags  = read_le32(&surf->dwFlags);
	const u32 h         = read_le32(&surf->dwHeight);
	const u32 w         = read_le32(&surf->dwWidth);
	const u32 img_size  = read_le32(&surf->dwLinearSize);
	      u32 mipmaps   = read_le32(&surf->dwMipMapCount);
	const u32 pf_size   = read_le32(&surf->ddpfPixelFormat.dwSize);
	const u32 pf_flags  = read_le32(&surf->ddpfPixelFormat.dwFlags);
	const u32 fourcc    = surf->ddpfPixelFormat.dwFourCC;
		// compared against FOURCC, which takes care of endian conversion.

	// we'll use these fields; make sure they're present below.
	// note: we can't guess image dimensions if not specified -
	//       the image isn't necessarily square.
	const u32 sd_req_flags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

	// make sure fields that aren't indicated as valid are zeroed.
	if(!(sd_flags & DDSD_MIPMAPCOUNT))
		mipmaps = 0;

	// MS DXTex tool doesn't set the required dwPitchOrLinearSize field -
	// they can't even write out their own file format correctly. *sigh*
	// we need to pass it to OpenGL; it's calculated from w, h, and bpp,
	// which we determine from the pixel format.
	int bpp = 0;
	int flags = 0;

	if(pf_flags & DDPF_ALPHAPIXELS)
		flags |= TEX_ALPHA;

	switch(fourcc)
	{
	case FOURCC('D','X','T','1'):
		bpp = 4;
		flags |= 1;
		break;
	case FOURCC('D','X','T','3'):
		bpp = 8;
		flags |= 3;
		break;
	case FOURCC('D','X','T','5'):
		bpp = 8;
		flags |= 5;
		break;
	}

	if(file_size < hdr_size + img_size)
		err = "image not completely loaded";
	if(w % 4 || h % 4)
		err = "image dimensions not padded to S3TC block size";
	if(!w || !h)
		err = "width or height = 0";
	if(mipmaps > 0)
		err = "contains mipmaps";
	if(bpp == 0)
		err = "invalid pixel format (not DXT{1,3,5})";
	if((sd_flags & sd_req_flags) != sd_req_flags)
		err = "missing one or more required fields (w, h, pixel format)";
	if(sizeof(DDPIXELFORMAT) != pf_size)
		err = "DDPIXELFORMAT size mismatch";
	if(sizeof(DDSURFACEDESC2) != sd_size)
		err = "DDSURFACEDESC2 size mismatch";

	if(err)
		goto fail;

	t->ofs   = hdr_size;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;
	return 0;
}


static int dds_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	return -1;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// TGA
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_TGA

static inline bool tga_fmt(const u8* ptr, size_t size)
{
	UNUSED(size)

	// no color map; uncompressed grayscale or true color
	return (ptr[1] == 0 && (ptr[2] == 2 || ptr[2] == 3));
}


static inline bool tga_ext(const char* const ext)
{
	return !strcmp(ext, ".tga");
}


// requirements: uncompressed, direct color, bottom up
static int tga_decode(TexInfo* t, const char* fn, const u8* file, size_t file_size)
{
	const char* err = 0;

	const u8 img_id_len = file[0];
	const size_t hdr_size = 18+img_id_len;

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("tga_decode: %s: %s\n", fn, err);
		return -1;
	}

	const u8 type = file[2];
	const u16 w   = read_le16(file+12);
	const u16 h   = read_le16(file+14);
	const u8 bpp  = file[16];
	const u8 desc = file[17];

	const u8 alpha_bits = desc & 0x0f;
	const size_t img_size = (ulong)w * h * bpp / 8;

	int flags = 0;

	if(alpha_bits != 0)
		flags |= TEX_ALPHA;

	// true color
	if(type == 2)
		flags |= TEX_BGR;

	if(desc & 0x30)
		err = "image is not bottom-up and left-to-right";
	if(file_size < hdr_size + img_size)
		err = "size < image size";

	if(err)
		goto fail;

	t->ofs   = hdr_size;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	return 0;
}


static int tga_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	return -1;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// BMP
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_BMP

#pragma pack(push, 1)

struct BITMAPFILEHEADER
{ 
	u16 bfType;			// "BM"
	u32 bfSize;			// of file
	u32 reserved;
	u32 bfOffBits;		// offset to image data
};

struct BITMAPINFOHEADER
{
	u32 biSize;
	long biWidth;
	long biHeight;
	u16 biPlanes;
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;

	// unused; zeroed when writing
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
};

#pragma pack(pop)

#define BI_RGB 0		// bih->biCompression


static inline bool bmp_fmt(const u8* p, size_t size)
{
	UNUSED(size)

	// bfType == BM? (check single bytes => endian safe)
	return p[0] == 'B' && p[1] == 'M';
}


static inline bool bmp_ext(const char* const ext)
{
	return !strcmp(ext, ".bmp");
}


// requirements: uncompressed, direct color, bottom up
static int bmp_decode(TexInfo* t, const char* fn, const u8* in, size_t file_size)
{
	const char* err = 0;

	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("bmp_decode: %s: %s\n", fn, err);
		return -1;
	}

	const BITMAPFILEHEADER* bfh = (const BITMAPFILEHEADER*)in;
	const BITMAPINFOHEADER* bih = (const BITMAPINFOHEADER*)(in+sizeof(BITMAPFILEHEADER));

	const long w       = read_le32(&bih->biWidth);
	const long h       = read_le32(&bih->biHeight);
	const u16 bpp      = read_le16(&bih->biBitCount);
	const u32 compress = read_le32(&bih->biCompression);
	const u32 ofs      = read_le32(&bfh->bfOffBits);

	const size_t img_size = w * h * bpp/8;

	int flags = TEX_BGR;
	if(bpp == 32)
		flags |= TEX_ALPHA;

	if(h < 0)
		err = "top-down";
	if(compress != BI_RGB)
		err = "compressed";
	if(bpp < 24)
		err = "not direct color";
	if(file_size < ofs+img_size)
		err = "image not completely read";

	if(err)
		goto fail;

	t->ofs = ofs;
	t->w   = w;
	t->h   = h;
	t->bpp = bpp;
	t->flags = flags;

	return 0;
}


static int bmp_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	int flags = t->flags;
	if((flags & TEX_DXT) || !(flags & TEX_BGR))
	{
		debug_warn("bmp: can't encode");
		return -1;
	}

	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	const size_t file_size = hdr_size + img_size;
	u8* file = (u8*)mem_alloc(hdr_size + img_size);
	if(!file)
		return -1;

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)file;
	bfh->bfType    = 0x4d42;	// 'B','M'
	bfh->bfSize    = (u32)file_size;
	bfh->reserved  = 0;
	bfh->bfOffBits = hdr_size;

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(file+sizeof(BITMAPFILEHEADER));
	bih->biSize          = sizeof(BITMAPINFOHEADER);
	bih->biWidth         = t->w;
	bih->biHeight        = t->h;
	bih->biPlanes        = 1;
	bih->biBitCount      = t->bpp;
	bih->biCompression   = BI_RGB;
	bih->biSizeImage     = (u32)img_size;
	bih->biXPelsPerMeter = 0;
	bih->biYPelsPerMeter = 0;
	bih->biClrUsed       = 0;
	bih->biClrImportant  = 0;

	memcpy(file+hdr_size, img, img_size);
	vfs_store(fn, file, file_size, FILE_NO_AIO);
	mem_free(file);
	return 0;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// RAW
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_RAW

static inline bool raw_fmt(const u8* p, size_t size)
{
	UNUSED(p)
	UNUSED(size)

	return true;
}


static inline bool raw_ext(const char* const ext)
{
	return !strcmp(ext, ".raw");
}


static int raw_decode(TexInfo* t, const char* fn, const u8* in, size_t file_size)
{
	UNUSED(in);

	// TODO: allow 8 bit format. problem: how to differentiate from 32? filename?

	for(uint i = 2; i <= 4; i++)
	{
		const u32 dim = (u32)sqrtf((float)file_size/i);
		if(dim*dim*i != file_size)
			continue;

		// formats are: GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
		int flags = (i == 3)? 0 : TEX_ALPHA;

		t->ofs = 0;
		t->w   = dim;
		t->h   = dim;
		t->bpp = i*8;
		t->flags = flags;

		return 0;
	}

	debug_out("raw_decode: %s: %s\n", fn, "no matching format found");
	return -1;
}


static int raw_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	return -1;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// PNG
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_PNG


static inline bool png_fmt(const u8* ptr, size_t size)
{
	return png_sig_cmp((u8*)ptr, 0, MIN(size, 8)) == 0;
}


static inline bool png_ext(const char* const ext)
{
	return !strcmp(ext, ".png");
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr). we at least split
// out alloc_rows, though.

// allocate and set up rows to point into image buffer
static int alloc_rows(const u8* img, size_t h, size_t pitch, bool inverted, const u8*** rows)
{
	*rows = (const u8**)malloc(h * sizeof(void*));
	if(!*rows)
		return -ENOMEM;
	const u8* pos = img;
	size_t i;
	if(inverted)
		for(i = 0; i < h; i++)
		{
			(*rows)[h-1-i] = pos;
			pos += pitch;
		}
	else
		for(i = 0; i < h; i++)
		{
			(*rows)[i] = pos;
			pos += pitch;
		}

	return 0;
}


struct PngMemFile
{
	const u8* p;
	size_t size;

	size_t pos;		// 0-initialized if no initializer
};

// pass data from PNG file in memory to libpng
static void png_read(png_struct* const png_ptr, u8* const data, const png_size_t length)
{
	PngMemFile* const f = (PngMemFile*)png_ptr->io_ptr;

	void* src = (u8*)(f->p + f->pos);

	// make sure there's enough new data remaining in the buffer
	f->pos += length;
	if(f->pos > f->size)
		png_error(png_ptr, "png_read: not enough data to satisfy request!");

	memcpy(data, src, length);
}


// limitation: palette images aren't supported
static int png_decode(TexInfo* t, const char* fn, const u8* file, size_t file_size)
{
	const char* msg = 0;
	int err = -1;

	// freed when ret is reached.
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	const u8** rows = 0;
	// freed when fail is reached
	const u8* img = 0;

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
fail:
		mem_free(img);

		debug_out("png_decode: %s: %s\n", fn, msg? msg : "unknown");
		goto ret;
	}

	{
		PngMemFile f = { file, file_size };
		png_set_read_fn(png_ptr, &f, png_read);

		// read header and determine format
		png_read_info(png_ptr, info_ptr);
		png_uint_32 w, h;
		int bit_depth, color_type;
		png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type, 0, 0, 0);
		const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);
		const u32 bpp = (u32)(pitch / w * 8);

		// make sure format is acceptable
		if(bit_depth != 8)
			msg = "channel precision != 8 bits";
		if(color_type & 1)
			msg = "color type is invalid (must be direct color)";
		if(msg)
			goto fail;

		const size_t img_size = pitch * h;
		Handle img_hm;
			// cannot free old t->hm until after png_read_end,
			// but need to set to this handle afterwards => need tmp var.
		img = (const u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
		if(!img)
			goto fail;

		CHECK_ERR(alloc_rows(img, h, pitch, false, &rows));

		png_read_image(png_ptr, (png_bytepp)rows);
		png_read_end(png_ptr, info_ptr);

		// store image info
		mem_free_h(t->hm);
		t->hm    = img_hm;
		t->ofs   = 0;	// libpng returns decoded image data; no header
		t->w     = w;
		t->h     = h;
		t->bpp   = bpp;
		t->flags = (bpp == 24)? 0 : TEX_ALPHA;

		err = 0;
	}

	// shared cleanup
ret:
	free(rows);

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return err;
}


// write libpng output to PNG file
static void png_write(png_struct* const png_ptr, u8* const data, const png_size_t length)
{
	void* p = (void*)data;
	Handle hf = *(Handle*)png_ptr->io_ptr;
	if(vfs_io(hf, length, &p) != length)
		png_error(png_ptr, "png_write: !");
}


// limitation: palette images aren't supported
static int png_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	const char* msg = 0;
	int err = -1;

	// freed when ret is reached.
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	const u8** rows = 0; 
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
fail:
		debug_out("png_encode: %s: %s\n", fn, msg? msg : "unknown");
		goto ret;
	}

	{
		const png_uint_32 w = t->w, h = t->h;
		const size_t pitch = w * t->bpp / 8;

		int color_type;
		switch(t->flags & (TEX_GRAY|TEX_ALPHA))
		{
		case TEX_GRAY|TEX_ALPHA:
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;
		case TEX_GRAY:
			color_type = PNG_COLOR_TYPE_GRAY;
			break;
		case TEX_ALPHA:
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		default:
			color_type = PNG_COLOR_TYPE_RGB;
			break;
		}

		hf = vfs_open(fn, FILE_WRITE|FILE_NO_AIO);
		if(hf < 0)
		{
			err = (int)hf;
			goto fail;
		}
		png_set_write_fn(png_ptr, &hf, png_write, 0);

		png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info(png_ptr, info_ptr);
	
		CHECK_ERR(alloc_rows(img, h, pitch, true, &rows));

		png_write_image(png_ptr, (png_bytepp)rows);
		png_write_end(png_ptr, info_ptr);
	
		err = 0;

	}

	// shared cleanup
ret:
	free(rows);
	vfs_close(hf);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	return err;
}



#endif


//////////////////////////////////////////////////////////////////////////////
//
// JP2
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_JP2

static inline bool jp2_fmt(const u8* p, size_t size)
{
	ONCE(jas_init());

	jas_stream_t* stream = jas_stream_memopen((char*)p, size);
	return jp2_validate(stream) >= 0;
}


static inline bool jp2_ext(const char* const ext)
{
	return !strcmp(ext, ".jp2");
}


static int jp2_decode(TexInfo* t, const char* fn, const u8* file, size_t file_size)
{
	const char* err = 0;

	jas_stream_t* stream = jas_stream_memopen((char*)file, file_size);
	jas_image_t* image = jas_image_decode(stream, -1, 0);
	if(!image)
		return -1;

	const int num_cmpts = jas_image_numcmpts(image);
	jas_matrix_t* matr[4] = {0};
	jas_seqent_t* rows[4] = {0};
	const u32 w    = jas_image_cmptwidth (image, 0);
	const u32 h    = jas_image_cmptheight(image, 0);
	const int prec = jas_image_cmptprec  (image, 0);
	const u32 bpp = num_cmpts * 8;
	const u32 ofs = 0;	// jasper returns decoded image data; no header
	int flags = 0;

	if(depth != 8)
	{
		err = "channel precision != 8";

fail:
		debug_out("jp2_decode: %s: %s\n", fn, err);
// TODO: destroy image
		return -1;
	}

	size_t img_size = w * h * num_cmpts;
	Handle img_hm;
	u8* img = (u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
	u8* out = img;

	int cmpt;
	for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		matr[cmpt] = jas_matrix_create(1, w);

	for(int y = 0; y < h; y++)
	{
		for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		{
			jas_image_readcmpt(image, cmpt, 0, y, w, 1, matr[cmpt]);
			rows[cmpt] = jas_matrix_getref(matr[cmpt], 0, 0);
		}

		for(int x = 0; x < w; x++)
			for(cmpt = 0; cmpt < num_cmpts; cmpt++)
				*out++ = *rows[cmpt]++;
	}

	for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		jas_matrix_destroy(matr[cmpt]);

	mem_free_h(t->hm);
	t->hm  = img_hm;

	t->ofs = ofs;
	t->w   = w;
	t->h   = h;
	t->bpp = bpp;
	t->flags = flags;

	return 0;
}


static int jp2_encode(TexInfo* t, const char* fn, const u8* img, size_t img_size)
{
	return -1;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////



static Codec codecs[] =
{
#ifndef NO_DDS
	CODEC(dds),
#endif
#ifndef NO_PNG
	CODEC(png),
#endif
#ifndef NO_JP2
	CODEC(jp2),
#endif
#ifndef NO_BMP
	CODEC(bmp),
#endif
#ifndef NO_TGA
	CODEC(tga),
#endif
#ifndef NO_RAW
	CODEC(raw),
#endif
};

static const int num_codecs = sizeof(codecs) / sizeof(codecs[0]);





int tex_load(const char* const fn, TexInfo* t)
{
	// load file
	void* _p = 0;
	size_t size;
	Handle hm = vfs_load(fn, _p, size);
	if(hm <= 0)
		return (int)hm;
	// guarantee *_valid routines 4 header bytes
	if(size < 4)
	{
		mem_free_h(hm);
		return -1;
	}
	t->hm = hm;

	int err = -1;
		// initial value, in case no codec is found

	const u8* p = (const u8*)_p;
		// more convenient to pass loaders u8 - less casting

	// find codec that understands the data, and decode
	Codec* c = codecs;
	for(int i = 0; i < num_codecs; i++, c++)
		if(c->is_fmt(p, size))
		{
			err = c->decode(t, fn, p, size);
			break;
		}

	if(err < 0)
	{
		mem_free_h(hm);
		return err;
	}

	return 0;
}


int tex_free(TexInfo* t)
{
	mem_free_h(t->hm);
	return 0;
}


int tex_write(TexInfo* t, const char* fn)
{
	u8* img = (u8*)mem_get_ptr(t->hm);
	if(!img)
		return -1;

	const size_t img_size = t->w * t->h * t->bpp / 8;

	char* ext = strrchr(fn, '.');
	if(!ext)
		return -1;

	Codec* c = codecs;
	for(int i = 0; i < num_codecs; i++, c++)
		if(c->is_ext(ext))
		{
			c->encode(t, fn, img, img_size);
			return 0;
		}

	// no codec found
	return -1;
}


int tex_write(const char* fn, int w, int h, int bpp, int flags, void* img)
{
	size_t img_size = w * h * bpp / 8;
	const Handle hm = mem_assign(img, img_size);
	TexInfo t = { hm, 0, w, h, bpp, flags };

	return tex_write(&t, fn);
}
