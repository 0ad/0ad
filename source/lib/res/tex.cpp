// OpenGL texturing
//
// Copyright (c) 2003 Jan Wassenberg
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

// supported formats: DDS, TGA, BMP, PNG, JP2, RAW

#include "precompiled.h"

#include "lib.h"
#include "res.h"

#include <math.h>
#include <stdlib.h>
#include <assert.h>


#define NO_JP2
//#define NO_PNG


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


/*
// filled by loader funcs => declare here
struct Tex
{
	u32 w   : 16;
	u32 h   : 16;
	u32 fmt : 16;
	u32 bpp : 16;
	size_t ofs;			// offset to image data in file
	Handle hm;			// H_MEM handle to loaded file
	uint id;

	int filter;
	int int_fmt;
};
*/



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


static inline bool dds_valid(const u8* ptr, size_t size)
{
	UNUSED(size)	// only need first 4 chars
	
	return *(u32*)ptr == FOURCC('D','D','S',' ');
}


// TODO: DXT1a?
static int dds_load(const char* fn, const u8* p, size_t size, TexInfo* t)
{
	const char* err = 0;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(p+4);
	const u32 hdr_size = 4+sizeof(DDSURFACEDESC2);

	// make sure we can access all header fields
	if(size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("dds_load: %s: %s\n", fn, err);
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
	// we need to pass to OpenGL; it's calculated from w, h, and bpp,
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

	if(size < hdr_size + img_size)
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

#endif


//////////////////////////////////////////////////////////////////////////////
//
// TGA
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_TGA

static inline bool tga_valid(const u8* ptr, size_t size)
{
	UNUSED(size)

	// no color map; uncompressed grayscale or true color
	return (ptr[1] == 0 && (ptr[2] == 2 || ptr[2] == 3));
}


// requirements: uncompressed, direct color, bottom up
static int tga_load(const char* fn, const u8* ptr, size_t size, TexInfo* t)
{
	const char* err = 0;

	const u8 img_id_len = ptr[0];
	const uint hdr_size = 18+img_id_len;

	// make sure we can access all header fields
	if(size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("tga_load: %s: %s\n", fn, err);
		return -1;
	}

	const u8 type = ptr[2];
	const u16 w   = read_le16(ptr+12);
	const u16 h   = read_le16(ptr+14);
	const u8 bpp  = ptr[16];
	const u8 desc = ptr[17];

	const u8 alpha_bits = desc & 0x0f;
	const ulong img_size = (ulong)w * h * bpp / 8;

	int flags = 0;

	if(alpha_bits != 0)
		flags |= TEX_ALPHA;

	// true color
	if(type == 2)
		flags |= TEX_BGR;

	if(desc & 0x30)
		err = "image is not bottom-up and left-to-right";
	if(size < hdr_size + img_size)
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

// BITMAPCOREHEADER + compression field
struct BITMAPCOREHEADER2
{
	u32 biSize;
	long biWidth;
	long biHeight;
	u16 biPlanes;		// = 1
	u16 biBitCount;		// bpp
	u32 biCompression;
};

#pragma pack(pop)

#define BI_RGB 0		// bch->biCompression


static inline bool bmp_valid(const u8* ptr, size_t size)
{
	UNUSED(size)

	// bfType == BM? (check single bytes => endian safe)
	return ptr[0] == 'B' && ptr[1] == 'M';
}


// requirements: uncompressed, direct color, bottom up
static int bmp_load(const char* fn, const u8* ptr, size_t size, TexInfo* t)
{
	const char* err = 0;

	const int hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);

	// make sure we can access all header fields
	if(size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("bmp_load: %s: %s\n", fn, err);
		return -1;
	}

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)ptr;
	BITMAPCOREHEADER2* bch = (BITMAPCOREHEADER2*)(ptr+sizeof(BITMAPFILEHEADER));

	const long w       = read_le32(&bch->biWidth);
	const long h       = read_le32(&bch->biHeight);
	const u16 bpp      = read_le16(&bch->biBitCount);
	const u32 compress = read_le32(&bch->biCompression);
	const u32 ofs      = read_le32(&bfh->bfOffBits);

	const u32 img_size = w * h * bpp/8;

	int flags = TEX_BGR;
	if(bpp == 32)
		flags |= TEX_ALPHA;

	if(h < 0)
		err = "top-down";
	if(compress != BI_RGB)
		err = "compressed";
	if(bpp < 24)
		err = "not direct color";
	if(size < ofs+img_size)
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


static int bmp_write(void*& out_buf, const u8* img, const size_t size, TexInfo* t)
{
	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);
	const size_t file_size = size+hdr_size;
	out_buf = mem_alloc(file_size);

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)out_buf;
	char* type = (char*)bfh;
	type[0] = 'B'; type[1] = 'M';
	bfh->bfSize = (u32)file_size;
	bfh->reserved = 0;
	bfh->bfOffBits = hdr_size;

	BITMAPCOREHEADER2* bch = (BITMAPCOREHEADER2*)((char*)out_buf+sizeof(BITMAPFILEHEADER));
	bch->biSize = sizeof(BITMAPCOREHEADER2);
	bch->biWidth = t->w;
	bch->biHeight = t->h;
	bch->biPlanes = 1;
	bch->biBitCount = t->bpp;
	bch->biCompression = 0;

	memcpy((char*)out_buf+hdr_size, img, size);
	return 0;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// RAW
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_RAW

static inline bool raw_valid(const u8* p, size_t size)
{
	UNUSED(p)
	UNUSED(size)

	return true;
}


static int raw_load(const char* fn, const u8* ptr, size_t size, TexInfo* t)
{
	UNUSED(ptr);

	// TODO: allow 8 bit format. problem: how to differentiate from 32? filename?

	for(uint i = 2; i <= 4; i++)
	{
		const u32 dim = (u32)sqrtf((float)size/i);
		if(dim*dim*i != size)
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

	debug_out("raw_load: %s: %s\n", fn, "no matching format found");
	return -1;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// PNG
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_PNG

struct MemRange
{
	const u8* p;
	size_t size;
};


static void png_read_fn(png_struct* const png_ptr, u8* const data, const png_size_t length)
{
	MemRange* const mr = (MemRange*)png_ptr->io_ptr;
	if(mr->size < length)
		png_error(png_ptr, "png_read_fn: not enough data to satisfy request!");

	memcpy(data, mr->p, length);
	mr->p += length;
	mr->size -= length;	// >= 0 due to test above
}


static inline bool png_valid(const u8* ptr, size_t size)
{
	return png_sig_cmp((u8*)ptr, 0, MIN(size, 8)) == 0;
}


// requirement: direct color
static int png_load(const char* fn, const u8* ptr, size_t size, TexInfo* t)
{
	const char* msg = 0;
	int err = -1;

	const u8** rows = 0; 
		// freed in cleanup code; need scoping on VC6 due to goto

	// allocate PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		return ERR_NO_MEM;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		err = ERR_NO_MEM;
		goto fail;
	}

	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
fail:
		debug_out("png_load: %s: %s\n", fn, msg? msg : "unknown");
		goto ret;
	}


	{

	MemRange mr = { ptr, size };
	png_set_read_fn(png_ptr, &mr, png_read_fn);

	png_read_info(png_ptr, info_ptr);
	unsigned long w, h;
	int prec, color_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &prec, &color_type, 0, 0, 0);

	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);

	const u32 bpp = (u32)(pitch / w * 8);
	const u32 ofs = 0;	// libpng returns decoded image data; no header

	int flags = (bpp == 24)? 0 : TEX_ALPHA;

	if(prec != 8)
		msg = "channel precision != 8 bits";
	if(color_type & 1)
		msg = "color type is invalid (must be direct color)";
	if(msg)
	{
		err = -1;
		goto fail;
	}

	// allocate mem for image - rows point into buffer (sequential)
	rows = (const u8**)malloc(h*sizeof(void*));
	if(!rows)
		goto fail;
	const size_t img_size = pitch * h;
	Handle img_hm;
	const u8* img = (const u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
	if(!img)
		goto fail;
	const u8* pos = img;
	for(size_t i = 0; i < h; i++)
	{
		rows[i] = pos;
		pos += pitch;
	}

	png_read_image(png_ptr, (png_bytepp)rows);

	png_read_end(png_ptr, info_ptr);

	mem_free_h(t->hm);
	t->hm  = img_hm;

	t->ofs = ofs;
	t->w   = w;
	t->h   = h;
	t->bpp = bpp;
	t->flags = flags;

	err = 0;

	}

	// shared cleanup
ret:
	free(rows);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return err;
}

#endif


//////////////////////////////////////////////////////////////////////////////
//
// JP2
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_JP2

static inline bool jp2_valid(const u8* p, size_t size)
{
	static bool initialized;
	if(!initialized)
	{
		jas_init();
		initialized = true;
	}

	jas_stream_t* stream = jas_stream_memopen((char*)ptr, size);
	return jp2_validate(stream) >= 0;
}


static int jp2_load(const char* fn, const u8* ptr, size_t size, TexInfo* t)
{
	const char* err = 0;

	jas_stream_t* stream = jas_stream_memopen((char*)ptr, size);
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
		debug_out("jp2_load: %s: %s\n", fn, err);
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

#endif


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

	// more convenient to pass loaders u8 - less casting
	const u8* p = (const u8*)_p;

#ifndef NO_DDS
	if(dds_valid(p, size))
		err = dds_load(fn, p, size, t); else
#endif
#ifndef NO_PNG
	if(png_valid(p, size))
		err = png_load(fn, p, size, t); else
#endif
#ifndef NO_JP2
	if(jp2_valid(p, size))
		err = jp2_load(fn, p, size, t); else
#endif
#ifndef NO_BMP
	if(bmp_valid(p, size))
		err = bmp_load(fn, p, size, t); else
#endif
#ifndef NO_TGA
	if(tga_valid(p, size))
		err = tga_load(fn, p, size, t); else
#endif
#ifndef NO_RAW
	if(raw_valid(p, size))
		err = raw_load(fn, p, size, t); else
#endif
	{};	// make sure else-chain is ended

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
