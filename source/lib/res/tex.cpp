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

// supported formats: DDS, TGA, PNG, JP2, BMP, RAW


#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "lib.h"
#include "vfs.h"
#include "tex.h"
#include "mem.h"
#include "ogl.h"
#include "h_mgr.h"
#include "misc.h"


#define NO_JP2
//#define NO_PNG


#ifndef NO_JP2
#include <jasper/jasper.h>
#endif


#define _WINDOWS_
#define WINAPI __stdcall
#define WINAPIV __cdecl

#ifndef NO_PNG
#include <png.h>
#pragma comment(lib, "libpng.lib")
#endif


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
};

H_TYPE_DEFINE(Tex)


const u32 FMT_UNKNOWN = 0;


//////////////////////////////////////////////////////////////////////////////
//
// DDS
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_DDS


// modified from ddraw header

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
static int dds_load(const char* fn, const u8* p, size_t size, Tex* t)
{
	const char* err = 0;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(p+4);
	const u32 hdr_size = 4+sizeof(DDSURFACEDESC2);
	// make sure we can access all header fields
	if(size < hdr_size)
		err = "header not completely read";
	else
	{
		const u32 sd_size   = read_le32(&surf->dwSize);
		const u32 sd_flags  = read_le32(&surf->dwSize);
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
		u32 bpp = 0;
		u32 fmt = FMT_UNKNOWN;

		switch(fourcc)
		{
		case FOURCC('D','X','T','1'):
			if(pf_flags & DDPF_ALPHAPIXELS)
				fmt = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			else
				fmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			bpp = 4;
			break;
		case FOURCC('D','X','T','3'):
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			bpp = 8;
			break;
		case FOURCC('D','X','T','5'):
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			bpp = 8;
			break;
		}

		if(size < hdr_size + img_size)
			err = "image not completely loaded";
		if(w % 4 || h % 4)
			err = "image dimensions not padded to S3TC block size";
		if(!w || !h)
			err = "width or height = 0 -- that's silly";
		if(mipmaps > 0)
			err = "contains mipmaps";
		if(fmt == 0)
			err = "invalid pixel format (not DXT{1,3,5})";
		if((sd_flags & sd_req_flags) != sd_req_flags)
			err = "missing one or more required fields (w, h, pixel format)";
		if(sizeof(DDPIXELFORMAT) != pf_size)
			err = "DDPIXELFORMAT size mismatch";
		if(sizeof(DDSURFACEDESC2) != sd_size)
			err = "DDSURFACEDESC2 size mismatch";

		t->w   = w;
		t->h   = h;
		t->fmt = fmt;
		t->bpp = bpp;
		t->ofs = hdr_size;
	}

	if(err)
	{
		printf("dds_load: %s: %s\n", fn, err);
		return -1;
	}

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
static int tga_load(const char* fn, const u8* ptr, size_t size, Tex* t)
{
	const char* err = 0;

	const u8 img_id_len = ptr[0];
	const uint hdr_size = 18+img_id_len;
	if(size < hdr_size)
		err = "header not completely read";
	else
	{
		const u8 type = ptr[2];
		const u16 w   = read_le16(ptr+12);
		const u16 h   = read_le16(ptr+14);
		const u8 bpp  = ptr[16];
		const u8 desc = ptr[17];

		const u8 alpha_bits = desc & 0x0f;

		const ulong img_size = (ulong)w * h * bpp / 8;
		const u32 ofs = hdr_size;

		// determine format
		u32 fmt = ~0;
		// .. grayscale
		if(type == 3)
		{
			// 8 bit format: several are possible, we can't decide
			if(bpp == 8)
				fmt = FMT_UNKNOWN;
			else if(bpp == 16 && alpha_bits == 8)
				fmt = GL_LUMINANCE_ALPHA;
		}
		// .. true color
		else if(type == 2)
		{
			if(bpp == 24 && alpha_bits == 0)
				fmt = GL_BGR;
			else if(bpp == 32 && alpha_bits == 8)
				fmt = GL_BGRA;
		}

		if(fmt == ~0)
			err = "invalid format or bpp";
		if(desc & 0x18)
			err = "image is not bottom-up and left-to-right";
		if(size < hdr_size + img_size)
			err = "size < image size";

		t->w   = w;
		t->h   = h;
		t->fmt = fmt;
		t->bpp = bpp;
		t->ofs = ofs;
	}

	if(err)
	{
		printf("tga_load: %s: %s\n", fn, err);
		return -1;
	}

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
static int bmp_load(const char* fn, const u8* ptr, size_t size, Tex* t)
{
	const char* err = 0;

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)ptr;
	BITMAPCOREHEADER2* bch = (BITMAPCOREHEADER2*)(ptr+sizeof(BITMAPFILEHEADER));
	const int hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);
	if(size < hdr_size)
		err = "header not completely read";
	else
	{
		const long w       = read_le32(&bch->biWidth);
		const long h       = read_le32(&bch->biHeight);
		const u16 bpp      = read_le16(&bch->biBitCount);
		const u32 compress = read_le32(&bch->biCompression);
		const u32 ofs      = read_le32(&bfh->bfOffBits);

		const u32 img_size = w * h * bpp/8;
		const u32 fmt = (bpp == 24)? GL_BGR : GL_BGRA;

		if(h < 0)
			err = "top-down";
		if(compress != BI_RGB)
			err = "compressed";
		if(bpp < 24)
			err = "not direct color";
		if(size < ofs+img_size)
			err = "image not completely read";

		t->w   = w;
		t->h   = h;
		t->fmt = fmt;
		t->bpp = bpp;
		t->ofs = ofs;
	}

	if(err)
	{
		printf("bmp_load: %s: %s\n", fn, err);
		return -1;
	}

	return 0;
}
// TODO: no extra buffer needed here; dealloc?

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


static int raw_load(const char* fn, const u8* ptr, size_t size, Tex* t)
{
	static u32 fmts[5] = { 0, 0, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	for(uint i = 1; i <= 4; i++)
	{
		u32 dim = (u32)sqrtf((float)size/i);
		// TODO: differentiate 8/32 bpp
		if(dim*dim*i != size)
			continue;

		const u32 w = dim;
		const u32 h = dim;
		const u32 fmt = fmts[i];
		const u32 bpp = i*8;
		const u32 ofs = 0;

		t->w   = w;
		t->h   = h;
		t->fmt = fmt;
		t->bpp = bpp;
		t->ofs = ofs;

		return 0;
	}

	printf("raw_load: %s: %s\n", fn, "no matching format found");
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


static void png_read_fn(png_struct* png_ptr, u8* data, png_size_t length)
{
	MemRange* const mr = (MemRange*)png_ptr->io_ptr;
	if(mr->size < length)
		png_error(png_ptr, "png_read_fn: not enough data to satisfy request!");

	memcpy(data, mr->p, length);
	mr->p += length;
	mr->size -= length;	// > 0 due to test above
}


static inline bool png_valid(const u8* ptr, size_t size)
{
	return png_sig_cmp((u8*)ptr, 0, MIN(size, 8)) == 0;
}


// requirement: direct color
static int png_load(const char* fn, const u8* ptr, size_t size, Tex* t)
{
	const char* err = 0;

	// allocate PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		return ERR_NO_MEM;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, 0, 0);
		return ERR_NO_MEM;
	}

	// setup error handling
	if(setjmp(png_jmpbuf(png_ptr)))
	{
fail:
		printf("png_load: %s: %s\n", fn, err? err : "");
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return -1;
	}

	MemRange mr = { ptr, size };
	png_set_read_fn(png_ptr, &mr, png_read_fn);

	png_read_info(png_ptr, info_ptr);
	unsigned long w, h;
	int prec, color_type;
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &prec, &color_type, 0, 0, 0);

	size_t pitch = png_get_rowbytes(png_ptr, info_ptr);

	const u32 fmts[8] = { 0, ~0, GL_RGB, ~0, GL_LUMINANCE_ALPHA, ~0, GL_RGBA, ~0 };
	const u32 fmt = color_type < 8? fmts[color_type] : ~0;
	const u32 bpp = (u32)(pitch / w * 8);
	const u32 ofs = 0;	// libpng returns decoded image data; no header

	if(prec != 8)
		err = "channel precision != 8 bits";
	if(fmt == ~0)
		err = "color type is invalid (must be direct color)";
	if(err)
		goto fail;

	// allocate mem for image - rows point into buffer (sequential)
	// .. (rows is freed in png_destroy_read_struct)
	u8** rows = (u8**)png_malloc(png_ptr, (h+1)*sizeof(void*));
	if(!rows)
		goto fail;
	size_t img_size = pitch * (h+1);
	Handle img_hm;
	u8* img = (u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
	if(!img)
		goto fail;
	u8* pos = img;
	for(u32 i = 0; i < h+1; i++)
	{
		rows[i] = pos;
		pos += pitch;
	}

	png_read_image(png_ptr, rows);

	png_read_end(png_ptr, 0);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	mem_free_h(t->hm);

	t->w   = w;
	t->h   = h;
	t->fmt = fmt;
	t->bpp = bpp;
	t->ofs = ofs;
	t->hm = img_hm;

	return 0;
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


static int jp2_load(const char* fn, const u8* ptr, size_t size, Tex* t)
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
	const u32 fmt = GL_RGB;
	const u32 bpp = num_cmpts * 8;
	const u32 ofs = 0;	// jasper returns decoded image data; no header

	if(depth != 8)
	{
		err = "channel precision != 8";

fail:
		printf("jp2_load: %s: %s\n", fn, err);
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

	t->w   = w;
	t->h   = h;
	t->fmt = fmt;
	t->bpp = bpp;
	t->ofs = ofs;
	t->hm  = img_hm;

	return 0;
}

#endif

static void Tex_init(Tex* t, va_list args)
{
}

static void Tex_dtor(Tex* t)
{
	mem_free_h(t->hm);

	glDeleteTextures(1, &t->id);
}


// TEX output param is invalid if function fails
static int Tex_reload(Tex* t, const char* fn)
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
	;	// make sure else chain is ended

	if(err < 0)
	{
		mem_free_h(hm);
		return err;
	}

	// loaders weren't able to determine type
	if(t->fmt == FMT_UNKNOWN)
	{
		assert(t->bpp == 8);
		t->fmt = GL_ALPHA;
		// TODO: check file name, go to 32 bit if wrong
	}

	uint id;
	glGenTextures(1, &id);
	t->id = id;
	// this can't realistically fail, just note that the already_loaded
	// check above assumes (id > 0) <==> texture is loaded and valid

	return 0;
}


inline Handle tex_load(const char* const fn, int scope)
{
	return h_alloc(H_Tex, fn, scope);
}


int tex_bind(const Handle h)
{
	Tex* t = H_USER_DATA(h, Tex);
	if(!t)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return ERR_INVALID_HANDLE;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, t->id);
		return 0;
	}
}


int tex_filter = GL_LINEAR;
uint tex_bpp = 32;				// 16 or 32

int tex_upload(const Handle ht, int filter, int int_fmt)
{
	H_DEREF(ht, Tex, t);

	// greater than max supported tex dimension?
	// no-op if oglInit not yet called
	if(t->w > (uint)max_tex_size || t->h > (uint)max_tex_size)
	{
		assert(!"tex_upload: image dimensions exceed OpenGL implementation limit");
		return 0;
	}

	// both NV_texture_rectangle and subtexture require work for the client
	// (changing tex coords) => we'll just disallow non-power of 2 textures.
	// TODO: ARB_texture_non_power_of_two
	if(!is_pow2(t->w) || !is_pow2(t->h))
	{
		assert(!"tex_upload: image is not power-of-2");
		return 0;
	}

	tex_bind(ht);

	// get pointer to image data
	size_t size;
	void* p = mem_get_ptr(t->hm, &size);
	if(!p)
	{
		assert(0 && "tex_upload: mem object is a NULL pointer");
		return 0;
	}
	const u8* img = (const u8*)p + t->ofs;

	// set filter
	if(!filter)
		filter = tex_filter;
	const int mag = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
	const bool mipmap = (filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_NEAREST ||
	                     filter == GL_NEAREST_MIPMAP_LINEAR  || filter == GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);


	const bool has_alpha = t->fmt == GL_RGBA || t->fmt == GL_BGRA || t->fmt == GL_LUMINANCE_ALPHA || t->fmt == GL_ALPHA;

	// S3TC compressed
	if(t->fmt >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
	   t->fmt <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		const int img_size = t->w * t->h * t->bpp / 8;
		assert(4+sizeof(DDSURFACEDESC2)+img_size == size && "tex_upload: dds file size mismatch");
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, t->fmt, t->w, t->h, 0, img_size, img);
	}
	// normal
	else
	{
		// calc internal fmt from format and global bpp, if not passed as a param
		if(!int_fmt)
		{
			if(t->bpp == 32)
				int_fmt = (tex_bpp == 32)? GL_RGBA8 : GL_RGBA4;
			else if(t->bpp == 24)
				int_fmt = (tex_bpp == 32)? GL_RGB8 : GL_RGB5;
			else if(t->bpp == 16)
				int_fmt = (tex_bpp == 32)? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE4_ALPHA4;
			else if(t->fmt == GL_ALPHA)
				int_fmt = (tex_bpp == 32)? GL_ALPHA8 : GL_ALPHA4;
			else if(t->fmt == GL_LUMINANCE)
				int_fmt = (tex_bpp == 32)? GL_LUMINANCE8 : GL_LUMINANCE4;
			else
				return -1;
		}

		// check if SGIS_generate_mipmap is available (once)
		static int sgm_avl = -1;
		if(sgm_avl == -1)
			sgm_avl = oglExtAvail("GL_SGIS_generate_mipmap");

		// manual mipmap gen via GLU (box filter)
		if(mipmap && !sgm_avl)
			gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, t->w, t->h, t->fmt, GL_UNSIGNED_BYTE, img);
		// auto mipmap gen, or no mipmap
		else
		{
			if(mipmap)
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

			glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, t->w, t->h, 0, t->fmt, GL_UNSIGNED_BYTE, img);
		}
	}

	mem_free_h(t->hm);

	return 0;
}


int tex_free(Handle& ht)
{
	return h_free(ht, H_Tex);
}


int tex_info(Handle ht, int* w, int* h, void** p)
{
	H_DEREF(ht, Tex, t);

	if(w)
		*w = t->w;
	if(h)
		*h = t->h;
	if(p)
		*p = mem_get_ptr(t->hm);
	return 0;
}
