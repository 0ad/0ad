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

#include "vfs.h"
#include "tex.h"
#include "mem.h"
#include "ogl.h"
#include "res.h"
#include "misc.h"

#define NO_JP2
#define NO_PNG

#ifndef NO_JP2
#include <jasper/jasper.h>
#endif

#ifndef NO_PNG
#include <png.h>
#pragma comment(lib, "libpng.lib")
#endif


//////////////////////////////////////////////////////////////////////////////
//
// DDS
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_DDS

// converts 4 character string to u32 for easy comparison
// can't pass code as string, and use s[0]..s[3], because
// VC6/7 don't realize the macro is constant (and it's used in a switch{})
#ifdef BIG_ENDIAN
#define FOURCC(a,b,c,d) ( ((u32)a << 24) | ((u32)b << 16) | \
                          ((u32)c << 8 ) | ((u32)d << 0 ) )
#else
#define FOURCC(a,b,c,d) ( ((u32)a << 0 ) | ((u32)b << 8 ) | \
                          ((u32)c << 16) | ((u32)d << 24) )
#endif

// modified from ddraw header

#pragma pack(push, 1)

typedef struct { u32 lo, hi; } DDCOLORKEY;

typedef struct
{
    u32 dwSize;                       // size of structure
    u32 dwFlags;                      // pixel format flags
    u32 dwFourCC;                     // (FOURCC code)
    u32 dwRGBBitCount;                // how many bits per pixel
    u32 dwRBitMask;                   // mask for red bit
    u32 dwGBitMask;                   // mask for green bits
    u32 dwBBitMask;                   // mask for blue bits
    u32 dwRGBAlphaBitMask;            // mask for alpha channel
}
DDPIXELFORMAT;

typedef struct
{
    u32 dwCaps;                       // capabilities of surface wanted
    u32 dwCaps2;
    u32 dwCaps3;
    u32 dwCaps4;
}
DDSCAPS2;

typedef struct
{
    u32 dwSize;                       // size of the DDSURFACEDESC structure
    u32 dwFlags;                      // determines what fields are valid
    u32 dwHeight;                     // height of surface to be created
    u32 dwWidth;                      // width of input surface
    u32 dwLinearSize;                 // surface size
    u32 dwBackBufferCount;            // number of back buffers requested
    u32 dwMipMapCount;                // number of mip-map levels requestde
    u32 dwAlphaBitDepth;              // depth of alpha buffer requested
    u32 dwReserved;                   // reserved
    void* lpSurface;                  // pointer to the associated surface memory
    DDCOLORKEY unused[4];             // src/dst overlay, blt
    DDPIXELFORMAT ddpfPixelFormat;    // pixel format description of the surface
    DDSCAPS2 ddsCaps;                 // direct draw surface capabilities
    u32 dwTextureStage;               // stage in multitexture cascade
}
DDSURFACEDESC2;

#pragma pack(pop)


static inline bool dds_valid(const u8* ptr, size_t size)
{
	UNUSED(size)	// only need first 4 chars

	return *(u32*)ptr == FOURCC('D','D','S',' ');
}


// TODO: DXT1a?
static int dds_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
{
	const char* err = 0;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(ptr+4);
	const u32 hdr_size = 4+sizeof(DDSURFACEDESC2);
	if(size < hdr_size)
		err = "header not completely read";
	else
	{
		const u32 w         = read_le32(&surf->dwWidth);
		const u32 h         = read_le32(&surf->dwHeight);
		const u32 ddsd_size = read_le32(&surf->dwSize);
		const u32 img_size  = read_le32(&surf->dwLinearSize);
		const u32 mipmaps   = read_le32(&surf->dwMipMapCount);

		uint fmt = 0;
		switch(surf->ddpfPixelFormat.dwFourCC)	// endian-independent
		{
		case FOURCC('D','X','T','1'):
			fmt = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case FOURCC('D','X','T','3'):
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case FOURCC('D','X','T','5'):
			fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		}

		tex->width  = w;
		tex->height = h;
		tex->fmt    = fmt;
		tex->bpp    = img_size / (w * h);
		tex->ofs    = hdr_size;

		if(sizeof(DDSURFACEDESC2) != ddsd_size)
			err = "DDSURFACEDESC2 size mismatch";
		if(size < hdr_size + img_size)
			err = "not completely loaded";
		if(mipmaps > 0)
			err = "contains mipmaps";
		if(fmt == 0)
			err = "invalid pixel format (not DXT{1,3,5})";
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
static int tga_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
{
	const char* err = "";

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

		// determine format
		int fmt = -1;
		// .. grayscale
		if(type == 3)
		{
			if(bpp == 8)
				fmt = 0;
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

		tex->width  = w;
		tex->height = h;
		tex->fmt    = fmt;
		tex->bpp    = bpp;
		tex->ofs    = hdr_size;

		if(fmt == -1)
			err = "invalid format or bpp";
		if(desc & 0x18)
			err = "image is not bottom-up and left-to-right";
		if(size < hdr_size + img_size)
			err = "size < image size";
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
static int bmp_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
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

		tex->width  = w;
		tex->height = h;
		tex->fmt    = (bpp == 24)? GL_BGR : GL_BGRA;
		tex->bpp    = bpp;
		tex->ofs    = ofs;

		if(h < 0)
			err = "top-down";
		if(compress != BI_RGB)
			err = "compressed";
		if(bpp < 24)
			err = "not direct color";
		if(size < ofs+img_size)
			err = "image not completely read";
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


static int raw_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
{
	static GLenum fmts[5] = { 0, 0, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };
	for(int i = 1; i <= 4; i++)
	{
		u32 dim = (u32)sqrtf((float)size/i);
		// TODO: differentiate 8/32 bpp
		if(dim*dim*i != size)
			continue;

		tex->width  = dim;
		tex->height = dim;
		tex->fmt    = fmts[i];
		tex->bpp    = i * 8;
		tex->ofs    = 0;

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
	mr->size -= length;
	if(mr->size < 0)
		png_error(png_ptr, "png_read_fn: no data remaining");

	memcpy(data, mr->ptr, length);
	mr->ptr += length;
}


static inline bool png_valid(const u8* ptr, size_t size)
{
	return png_sig_cmp((u8*)ptr, 0, min(size, 8)) == 0;
}


// requirement: direct color
static int png_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
{
	const char* err = 0;

	// allocate PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(!png_ptr)
		return -1;
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, 0, 0);
		return -1;
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
	unsigned long width, height;
	int prec, color_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, 0, 0, 0);

	// can only handle 8 bits per channel
	if(prec != 8)
	{
		err = "channel precision != 8";
		goto fail;
	}

	// allocate mem for image - rows point into buffer (sequential)
	int pitch = png_get_rowbytes(png_ptr, info_ptr);
	u8* img = (u8*)malloc(pitch * (height+1));
	u8** rows = (u8**)png_malloc(png_ptr, (height+1)*sizeof(void*));
	for(u32 i = 0; i < height+1; i++)
		rows[i] = img + i*pitch;

	png_read_image(png_ptr, rows);

	png_read_end(png_ptr, 0);
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	// store info in ti
	tex->width = width; tex->height = height;

	int fmts[8] = { 0, -1, GL_RGB, -1, GL_LUMINANCE_ALPHA, -1, GL_RGBA, -1 };
	if(color_type >= 8)
		return -1;
	tex->fmt = fmts[color_type];
	if(tex->fmt == -1)			// <==> palette image
	{
		printf("png_load: %s: %s\n", fn, "not direct color");
		return -1;
	}

	tex->ptr = img;
	tex->bpp = pitch / width * 8;

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


static int jp2_load(const char* fn, const u8* ptr, size_t size, TEX* tex)
{
	const char* err = 0;

	jas_stream_t* stream = jas_stream_memopen((char*)ptr, size);
	jas_image_t* image = jas_image_decode(stream, -1, 0);
	if(!image)
		return -1;

	int num_cmpts = jas_image_numcmpts(image);
	jas_matrix_t* matr[4] = {0};
	jas_seqent_t* rows[4] = {0};
	int width  = jas_image_cmptwidth (image, 0);
	int height = jas_image_cmptheight(image, 0);
	int prec   = jas_image_cmptprec  (image, 0);

	if(depth != 8)
	{
		err = "channel precision != 8";
		printf("jp2_load: %s: %s\n", fn, err);
// TODO: destroy image
		return -1;
	}

	u8* img = (u8*)malloc(width*height*num_cmpts);
	u8* out = img;

	int cmpt;
	for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		matr[cmpt] = jas_matrix_create(1, width);

	for(int y = 0; y < height; y++)
	{
		for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		{
			jas_image_readcmpt(image, cmpt, 0, y, width, 1, matr[cmpt]);
			rows[cmpt] = jas_matrix_getref(matr[cmpt], 0, 0);
		}

		for(int x = 0; x < width; x++)
			for(cmpt = 0; cmpt < num_cmpts; cmpt++)
				*out++ = *rows[cmpt]++;
	}

	for(cmpt = 0; cmpt < num_cmpts; cmpt++)
		jas_matrix_destroy(matr[cmpt]);

	tex->width = width;
	tex->height = height;
	tex->fmt = GL_RGB;
	tex->bpp = num_cmpts * 8;
	tex->ptr = img;

	return 0;
}

#endif




static void tex_dtor(void* p)
{
	TEX* tex = (TEX*)p;

	mem_free(tex->hm);

	glDeleteTextures(1, &tex->id);
}


// TEX output param is invalid if function fails
Handle tex_load(const char* fn, TEX* ptex)
{
	const u32 fn_hash = fnv_hash(fn, strlen(fn));

	TEX* tex;
	Handle ht = h_alloc(fn_hash, H_TEX, tex_dtor, (void**)&tex);
	if(!ht)
		return 0;
	if(tex->id != 0)
		goto already_loaded;

{
	// load file
	const u8* p;
	size_t size;
	Handle hm = vfs_load(fn, (void*&)p, size);
	// .. note: xxx_valid routines assume 4 header bytes are available
	if(!hm || !p || size < 4)
	{
		h_free(ht, H_TEX);
		return 0;
	}
	tex->hm = hm;

	int err = -1;

#ifndef NO_DDS
	if(dds_valid(p, size))
		err = dds_load(fn, p, size, tex); else
#endif
#ifndef NO_PNG
	if(png_valid(p, size))
		err = png_load(fn, p, size, tex); else
#endif
#ifndef NO_JP2
	if(jp2_valid(p, size))
		err = jp2_load(fn, p, size, tex); else
#endif
#ifndef NO_BMP
	if(bmp_valid(p, size))
		err = bmp_load(fn, p, size, tex); else
#endif
#ifndef NO_TGA
	if(tga_valid(p, size))
		err = tga_load(fn, p, size, tex); else
#endif
#ifndef NO_RAW
	if(raw_valid(p, size))
		err = raw_load(fn, p, size, tex); else
#endif
	;	// make sure else chain is ended

	if(err < 0)
		return err;

	// loaders weren't able to determine type
	if(tex->fmt == 0)
	{
		assert(tex->bpp == 8);
		tex->fmt = GL_ALPHA;
		// TODO: check file name, go to 32 bit if wrong
	}

	uint id;
	glGenTextures(1, &id);
	tex->id = id;
	// this can't realistically fail, just note that the already_loaded
	// check above assumes (id > 0) <==> texture is loaded and valid
}

already_loaded:
	if(ptex)
		*ptex = *tex;

	return ht;
}


int tex_bind(const Handle h)
{
	TEX* tex = (TEX*)h_user_data(h, H_TEX);
	if(!tex)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return -1;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, tex->id);
		return 0;
	}
}


int tex_filter = GL_LINEAR;
uint tex_bpp = 32;				// 16 or 32

int tex_upload(const Handle ht, int filter, int int_fmt)
{
	TEX* tex = (TEX*)h_user_data(ht, H_TEX);
	if(!tex)
		return -1;

	// greater than max supported tex dimension?
	// no-op if oglInit not yet called
	if(tex->width > (uint)max_tex_size || tex->height > (uint)max_tex_size)
	{
		assert(!"tex_upload: image dimensions exceed OpenGL implementation limit");
		return 0;
	}

	// both NV_texture_rectangle and subtexture require work for the client
	// (changing tex coords) => we'll just disallow non-power of 2 textures.
	// TODO: ARB_texture_non_power_of_two
	if(!is_pow2(tex->width) || !is_pow2(tex->height))
	{
		assert(!"tex_upload: image is not power-of-2");
		return 0;
	}

	tex_bind(ht);

	// get pointer to image data
	MEM* mem = (MEM*)h_user_data(tex->hm, H_MEM);
	if(!mem)
		return 0;
	void* p = mem->p;
	if(!p)
	{
		assert(0 && "tex_upload: mem object is a NULL pointer");
		return 0;
	}

	// set filter
	if(!filter)
		filter = tex_filter;
	const int mag = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
	const bool mipmap = (filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_NEAREST ||
	                     filter == GL_NEAREST_MIPMAP_LINEAR  || filter == GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);


	const bool has_alpha = tex->fmt == GL_RGBA || tex->fmt == GL_BGRA || tex->fmt == GL_LUMINANCE_ALPHA || tex->fmt == GL_ALPHA;

	// S3TC compressed
	if(tex->fmt >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
	   tex->fmt <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		const int img_size = tex->width * tex->height * tex->bpp;
		assert(4+sizeof(DDSURFACEDESC2)+img_size == mem->size && "tex_upload: dds file size mismatch");
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, tex->fmt, tex->width, tex->height, 0, img_size, p);
	}
	// normal
	else
	{
		// calc internal fmt from format and global bpp, if not passed as a param
		if(!int_fmt)
		{
			if(tex->bpp == 32)
				int_fmt = (tex_bpp == 32)? GL_RGBA8 : GL_RGBA4;
			else if(tex->bpp == 24)
				int_fmt = (tex_bpp == 32)? GL_RGB8 : GL_RGB5;
			else if(tex->bpp == 16)
				int_fmt = (tex_bpp == 32)? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE4_ALPHA4;
			else if(tex->fmt == GL_ALPHA)
				int_fmt = (tex_bpp == 32)? GL_ALPHA8 : GL_ALPHA4;
			else if(tex->fmt == GL_LUMINANCE)
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
			gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, tex->width, tex->height, tex->fmt, GL_UNSIGNED_BYTE, p);
		// auto mipmap gen, or no mipmap
		else
		{
			if(mipmap)
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

			glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, tex->width, tex->height, 0, tex->fmt, GL_UNSIGNED_BYTE, p);
		}
	}

	mem_free(tex->hm);

	return 0;
}
