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




#define CODEC(name) { name##_fmt, name##_ext, name##_decode, name##_get_output_size, name##_encode, #name}

struct Codec
{
	// size is at least 4.
	bool(*is_fmt)(const u8* p, size_t size);
	bool(*is_ext)(const char* ext);
	int(*decode)(TexInfo* t, const char* fn, const u8* in, size_t in_size);
	int(*get_output_size)(TexInfo* t, size_t* out_size);
	int(*encode)(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size);

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


static int dds_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	const char* err = 0;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(in+4);
	const size_t hdr_size = 4+sizeof(DDSURFACEDESC2);

	// make sure we can access all header fields
	if(in_size < hdr_size)
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

	if(in_size < hdr_size + img_size)
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


static int dds_get_output_size(TexInfo* t, size_t* out_size)
{
	return -1;
}


static int dds_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
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
static int tga_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	const char* err = 0;

	const u8 img_id_len = in[0];
	const size_t hdr_size = 18+img_id_len;

	// make sure we can access all header fields
	if(in_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("tga_decode: %s: %s\n", fn, err);
		return -1;
	}

	const u8 type = in[2];
	const u16 w   = read_le16(in+12);
	const u16 h   = read_le16(in+14);
	const u8 bpp  = in[16];
	const u8 desc = in[17];

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
	if(in_size < hdr_size + img_size)
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


static int tga_get_output_size(TexInfo* t, size_t* out_size)
{
	return -1;
}


static int tga_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
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
static int bmp_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	const char* err = 0;

	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);

	// make sure we can access all header fields
	if(in_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("bmp_decode: %s: %s\n", fn, err);
		return -1;
	}

	const BITMAPFILEHEADER* bfh = (const BITMAPFILEHEADER*)in;
	const BITMAPCOREHEADER2* bch = (const BITMAPCOREHEADER2*)(in+sizeof(BITMAPFILEHEADER));

	const long w       = read_le32(&bch->biWidth);
	const long h       = read_le32(&bch->biHeight);
	const u16 bpp      = read_le16(&bch->biBitCount);
	const u32 compress = read_le32(&bch->biCompression);
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
	if(in_size < ofs+img_size)
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


static int bmp_get_output_size(TexInfo* t, size_t* out_size)
{
	int flags = t->flags;
	if((flags & TEX_DXT) || !(flags & TEX_BGR))
	{
		debug_warn("bmp: can't encode");
		return -1;
	}

	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);
	*out_size = hdr_size + (t->w * t->h * t->bpp / 8);
	return 0;
}


static int bmp_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
{
	const size_t hdr_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPCOREHEADER2);

	BITMAPFILEHEADER* bfh = (BITMAPFILEHEADER*)out;
	bfh->bfType    = 0x4d42;	// 'B','M'
	bfh->bfSize    = (u32)round_up(out_size, 65536);
	bfh->reserved  = 0;
	bfh->bfOffBits = hdr_size;

	BITMAPCOREHEADER2* bch = (BITMAPCOREHEADER2*)(out+sizeof(BITMAPFILEHEADER));
	bch->biSize        = sizeof(BITMAPCOREHEADER2);
	bch->biWidth       = t->w;
	bch->biHeight      = t->h;
	bch->biPlanes      = 1;
	bch->biBitCount    = t->bpp;
	bch->biCompression = 0;

	memcpy(out+hdr_size, img, img_size);
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


static int raw_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	UNUSED(in);

	// TODO: allow 8 bit format. problem: how to differentiate from 32? filename?

	for(uint i = 2; i <= 4; i++)
	{
		const u32 dim = (u32)sqrtf((float)in_size/i);
		if(dim*dim*i != in_size)
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


static int raw_get_output_size(TexInfo* t, size_t* out_size)
{
	return -1;
}


static int raw_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
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


enum PngIO { PNG_WRITE, PNG_READ };

struct PngTransfer
{
	const u8* p;
	size_t size;

	PngIO op;
	size_t pos;		// 0-initialized if no initializer
};


static void png_io(png_struct* const png_ptr, u8* const data, const png_size_t length)
{
	PngTransfer* const t = (PngTransfer*)png_ptr->io_ptr;

	void* src = (void*)(t->p + t->pos);
	void* dst = data;
	// (assume read, i.e. copy from transfer buffer; switch if writing)
	if(t->op == PNG_WRITE)
		std::swap(src, dst);

	// make sure there's enough space/data in the buffer
	t->pos += length;
	if(t->pos > t->size)
		png_error(png_ptr, "png_io: not enough data to satisfy request!");

	memcpy(dst, src, length);
}


// requirement: direct color
static int png_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	const char* msg = 0;
	int err = -1;

	const u8** rows = 0; 
		// freed in cleanup code; need scoping on VC6 due to goto

	// allocate PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		// default stderr and longjmp error handling
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
		debug_out("png_decode: %s: %s\n", fn, msg? msg : "unknown");
		goto ret;
	}


	{

	PngTransfer trans = { in, in_size, PNG_READ };
	png_set_read_fn(png_ptr, &trans, png_io);

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


static int png_get_output_size(TexInfo* t, size_t* out_size)
{
	return -1;
}


static int png_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
{
	const char* msg = 0;
	int err = -1;

	const u8** rows = 0; 
	// freed in cleanup code; need scoping on VC6 due to goto


	// allocate PNG structures
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		// default stderr and longjmp error handling
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
		debug_out("png_encode: %s\n", msg? msg : "unknown");
		goto ret;
	}

	{



	PngTransfer trans = { out, out_size, PNG_WRITE };
	png_set_write_fn(png_ptr, (void*)&trans, png_io, 0);




	/* Set the image information here.  Width and height are up to 2^31,
	* bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
	* the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
	* PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
	* or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
	* PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
	* currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	*/
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
	
	png_set_IHDR(png_ptr, info_ptr, t->w, t->h, 8, color_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	/* Write the file header information.  REQUIRED */
	png_write_info(png_ptr, info_ptr);

	const size_t pitch = png_get_rowbytes(png_ptr, info_ptr);

	// allocate mem for image - rows point into buffer (sequential)
	rows = (const u8**)malloc(t->h*sizeof(void*));
	if(!rows)
		goto fail;
	const u8* pos = img;
	for(size_t i = 0; i < t->h; i++)
	{
		rows[i] = pos;
		pos += pitch;
	}

	png_write_image(png_ptr, (png_bytepp)rows);

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);


   /* Similarly, if you png_malloced any data that you passed in with
      png_set_something(), such as a hist or trans array, free it here,
      when you can be sure that libpng is through with it. */

	}

ret:
   free(rows);
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


static int jp2_decode(TexInfo* t, const char* fn, const u8* in, size_t in_size)
{
	const char* err = 0;

	jas_stream_t* stream = jas_stream_memopen((char*)in, in_size);
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


static int jp2_get_output_size(TexInfo* t, size_t* out_size)
{
	return -1;
}


static int jp2_encode(TexInfo* t, const u8* img, size_t img_size, u8* out, size_t out_size)
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
	int err;

	u8* img = (u8*)mem_get_ptr(t->hm);
	if(!img)
		return -1;

	char* ext = strrchr(fn, '.');
	if(!ext)
		return -1;

	Codec* c = codecs;
	for(int i = 0; i < num_codecs; i++, c++)
		if(c->is_ext(ext))
			goto have_codec;

	// no codec found
	return -1;

have_codec:

	size_t out_size;
	CHECK_ERR(c->get_output_size(t, &out_size));

	u8* out = (u8*)mem_alloc(out_size);
	if(!out)
		return ENOMEM;

	const size_t img_size = t->w * t->h * t->bpp / 8;
	err = c->encode(t, img, img_size, out, out_size);
	if(err < 0)
		goto fail;

	vfs_uncached_store(fn, out, out_size);
	if(err < 0)
		goto fail;

	return 0;

fail:
	mem_free(out);
	return -1;
}


int tex_write(const char* fn, int w, int h, int bpp, int flags, void* img)
{
	size_t img_size = w * h * bpp / 8;
	const Handle hm = mem_assign(img, img_size);
	TexInfo t = { hm, 0, w, h, bpp, flags };

	return tex_write(&t, fn);
}