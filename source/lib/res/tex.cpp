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
#include "tex.h"

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
//#define NO_JPG


#ifndef NO_JP2
#include <jasper/jasper.h>
#endif

#ifndef NO_JPG
extern "C" {
# include "jpeglib.h"
// extensions
EXTERN(void) jpeg_mem_src(j_decompress_ptr cinfo, void* p, size_t size);
}
# ifdef _MSC_VER
#  ifdef NDEBUG
#   pragma comment(lib, "jpeg-6b.lib")
#  else
#   pragma comment(lib, "jpeg-6bd.lib")
#  endif	// #ifdef NDEBUG
# endif	// #ifdef _MSV_VER
#endif	// #ifndef NO_JPG


#ifndef NO_PNG
# ifdef _WIN32
   // to avoid conflicts, windows.h must not be included.
   // libpng pulls it in for WINAPI; we prevent the include
   // and define that here.
#  define _WINDOWS_
#  define WINAPI __stdcall
#  define WINAPIV __cdecl
   // different header name, too.
#  include <libpng13/png.h>
#  ifdef _MSC_VER
#   ifdef NDEBUG
#    pragma comment(lib, "libpng13.lib")
#   else
#    pragma comment(lib, "libpng13d.lib")
#   endif	// NDEBUG
#  endif	// _MSC_VER
# else	// _WIN32
#  include <png.h>
# endif	// _WIN32
#endif	// NO_PNG




#define CODEC(name) { name##_fmt, name##_ext, name##_decode, name##_encode, #name}

struct Codec
{
	// pointers aren't const, because the textures
	// may have to be flipped in-place - see "texture orientation".

	// size is guaranteed to be >= 4.
	// (usually enough to compare the header's "magic" field;
	// anyway, no legitimate file will be smaller)
	bool(*is_fmt)(const u8* p, size_t size);
	bool(*is_ext)(const char* ext);
	int(*decode)(TexInfo* t, const char* fn, u8* file, size_t file_size);
	int(*encode)(TexInfo* t, const char* fn, u8* img, size_t img_size);

	// no get_output_size() function and central mem alloc / file write,
	// because codecs that write via external lib or output compressed files
	// don't know the output size beforehand.

	const char* name;
};


//////////////////////////////////////////////////////////////////////////////
//
// texture orientation
//
//////////////////////////////////////////////////////////////////////////////

// DDS and BMP are normally bottom-up, while PNG and JPG are always top-down.
// we don't want to dump the burden of flipping them to a common orientation
// on the application - that would affect all drawing code.
// instead, we flip the texture data to <global_orientation> when loading.
//
// codecs using libraries with a row-array mechanism (jpg, png) can
// just invert the row array pointers; otherwise (e.g. tga, bmp),
// we flip the texel rows in-place.
//
// this is slow; in release builds, we should be using formats optimized
// for their intended use that don't require preprocessing.

static TexOrientation global_orientation = TEX_BOTTOM_UP;

void tex_set_global_orientation(TexOrientation o)
{
	global_orientation = o;
}


// if fmt_orientation doesn't match the desired global orientation,
// tex will be flipped in-place. pitch is the total row width [bytes].
void flip_tex_rows(void* tex, size_t pitch, size_t h, TexOrientation fmt_orientation)
{
	// already correct, we're done
	if(fmt_orientation == global_orientation)
		return;

	size_t line = 0;	// counter
	const u8* src;
	u8* dst;

	// L1 cache is typically A2 => swapping in-place with a line buffer
	// leads to thrashing. we'll assume the whole texture*2 fits in cache,
	// allocate a copy, and transfer directly from there.
	//
	// note: we don't want to return a new buffer: the user assumes
	// buffer address will remain unchanged.

	const size_t size = pitch * h;
	void* clone_buf = mem_alloc(size, 32*KB);
	memcpy(clone_buf, tex, size);
	src = (const u8*)clone_buf + size-pitch;
	dst = (u8*)tex;
	for(; line < h; line++)
	{
		memcpy(dst, src, pitch);
		src -= pitch;
		dst += pitch;
	}
	mem_free(clone_buf);
}


typedef const u8* RowPtr;
typedef RowPtr* RowArray;

// allocate and set up rows to point into the given texture data.
// invert if the format's orientation doesn't match the current global setting.
static int alloc_rows(const u8* tex, size_t h, size_t pitch,
	TexOrientation fmt_orientation, RowArray& rows)
{
	rows = (RowArray)malloc(h * sizeof(RowPtr));
	if(!rows)
		return ERR_NO_MEM;

	// rows are inverted; current position pointer counts backwards.
	if(fmt_orientation != global_orientation)
	{
		RowPtr pos = tex + pitch*h;
		for(size_t i = 0; i < h; i++)
		{
			pos -= pitch;
			rows[i] = pos;
		}
	}
	// normal; count ahead.
	else
	{
		RowPtr pos = tex;
		for(size_t i = 0; i < h; i++)
		{
			rows[i] = pos;
			pos += pitch;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////


// write header and image out to file <fn>.
// pixels are BGR-flipped if desired. bpp can be 8, 24, or 32.
// note: if bpp=32, alpha is assumed to be the last component.
static int write_img(const char* fn, const void* hdr, size_t hdr_size, const void* img, size_t img_size,
					 bool flip_bgr, int bpp)
{
	const size_t file_size = hdr_size + img_size;
	u8* file = (u8*)mem_alloc(file_size);
	if(!file)
		return ERR_NO_MEM;

	memcpy(file, hdr, hdr_size);

	// fast: no conversion necessary
	if(!flip_bgr || bpp == 8)
		memcpy((char*)file+hdr_size, img, img_size);
	// slow: convert each pixel
	else
	{
		size_t pixels_left = img_size / (bpp / 8);
		u8* dst = (u8*)file+hdr_size;
		const u8* src = (const u8*)img;

		if(bpp == 24)
			while(pixels_left--)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst += 3;
				src += 3;
			}
		else
			while(pixels_left--)
			{
				dst[0] = src[2];
				dst[1] = src[1];
				dst[2] = src[0];
				dst[3] = src[3];
				dst += 4;
				src += 4;
			}
	}

	CHECK_ERR(vfs_store(fn, file, file_size, FILE_NO_AIO));
	mem_free(file);
	return 0;
}


static int fmt_8_or_24_or_32(int bpp, int flags)
{
	const bool alpha = (flags & TEX_ALPHA) != 0;
	const bool gray  = (flags & TEX_GRAY ) != 0;
	const bool dxt   = (flags & TEX_DXT  ) != 0;

	if(dxt)
		return ERR_TEX_FMT_INVALID;

	// if gray,
	if(gray)
	{
		// and 8bpp / no alpha, it's ok.
		if(bpp == 8 && !alpha)
			return 0;

		// .. otherwise, it's invalid
		return ERR_TEX_FMT_INVALID;
	}
	// it's not gray.

	if(bpp == 24 && !alpha)
		return 0;
	if(bpp == 32 && !alpha)
		return 0;

	return ERR_TEX_FMT_INVALID;
}


//////////////////////////////////////////////////////////////////////////////
//
// DDS
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_DDS


// NOTE: the convention is bottom-up for DDS, but there's no way to tell.

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
	UNUSED(size);	// size >= 4, we only need 4 bytes
	
	return *(u32*)ptr == FOURCC('D','D','S',' ');
}


static inline bool dds_ext(const char* ext)
{
	return !stricmp(ext, ".dds");
}


static int dds_decode(TexInfo* t, const char* fn, u8* file, size_t file_size)
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
		return ERR_CORRUPTED;
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
		flags |= 1;	// bits in TEX_DXT mask indicate format
		break;
	case FOURCC('D','X','T','3'):
		bpp = 8;
		flags |= 3;	// "
		break;
	case FOURCC('D','X','T','5'):
		bpp = 8;
		flags |= 5;	// "
		break;
	}

	if(file_size != hdr_size + img_size)
		err = "file size mismatch";
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


static int dds_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	return ERR_NOT_IMPLEMENTED;
}

#endif	// #ifndef NO_DDS


//////////////////////////////////////////////////////////////////////////////
//
// TGA
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_TGA


#pragma pack(push, 1)

enum TgaImgType
{
	TGA_TRUE_COLOR = 2,		// uncompressed 24 or 32 bit direct RGB
	TGA_GRAY       = 3		// uncompressed 8 bit direct grayscale
};

enum TgaImgDesc
{
	TGA_RIGHT_TO_LEFT = BIT(4),
	TGA_TOP_DOWN      = BIT(5)
};

typedef struct
{
	u8 img_id_len;			// 0 - no image identifier present
	u8 color_map_type;		// 0 - no color map present
	u8 img_type;			// see TgaImgType
	u8 color_map[5];		// unused

	u16 x_origin;			// unused
	u16 y_origin;			// unused

	u16 w;
	u16 h;
	u8 bpp;					// bits per pixel

	u8 img_desc;
}
TgaHeader;

// TGA file: header [img id] [color map] image data


#pragma pack(pop)


// the first TGA header doesn't have a magic field;
// we can only check if the first 4 bytes are valid
static inline bool tga_fmt(const u8* ptr, size_t size)
{
	UNUSED(size);

	TgaHeader* hdr = (TgaHeader*)ptr;

	// not direct color
	if(hdr->color_map_type != 0)
		return false;

	// wrong color type (not uncompressed grayscale or RGB)
	if(hdr->img_type != TGA_TRUE_COLOR && hdr->img_type != TGA_GRAY)
		return false;

	return true;
}


static inline bool tga_ext(const char* ext)
{
	return !stricmp(ext, ".tga");
}


// requirements: uncompressed, direct color, bottom up
static int tga_decode(TexInfo* t, const char* fn, u8* file, size_t file_size)
{
	const char* err = 0;

	TgaHeader* hdr = (TgaHeader*)file;
	const size_t hdr_size = 18 + hdr->img_id_len;

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("tga_decode: %s: %s\n", fn, err);
		return ERR_CORRUPTED;
	}

	const u8 type = hdr->img_type;
	const u16 w   = read_le16(&hdr->w);
	const u16 h   = read_le16(&hdr->h);
	const u8 bpp  = hdr->bpp;
	const u8 desc = hdr->img_desc;

	const u8 alpha_bits = desc & 0x0f;

	const size_t pitch = w *  bpp/8;
	const size_t img_size = h * pitch;
	void* img = file + hdr_size;

	TexOrientation fo = (desc & TGA_TOP_DOWN)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	flip_tex_rows(img, pitch, h, fo);

	int flags = 0;
	if(alpha_bits != 0)
		flags |= TEX_ALPHA;
	if(type == TGA_TRUE_COLOR)
		flags |= TEX_BGR;

	// storing right-to-left is just stupid;
	// we're not going to bother converting it.
	if(desc & TGA_RIGHT_TO_LEFT)
		err = "image is stored right-to-left";
	if(bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
		err = "invalid bpp";
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


static int tga_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	CHECK_ERR(fmt_8_or_24_or_32(t->bpp, t->flags));

	TgaHeader hdr =
	{
		0,				// no image identifier present
		0,				// no color map present
		(t->flags & TEX_GRAY)? TGA_GRAY : TGA_TRUE_COLOR,
		{0,0,0,0,0},	// unused (color map)
		0, 0,			// unused (origin)
		t->w,
		t->h,
		t->bpp,
		0				// image descriptor (TgaImgDesc)
	};
	const bool flip_bgr = !(t->flags & TEX_BGR);	// TGA is native BGR
	return write_img(fn, &hdr, sizeof(hdr), img, img_size, flip_bgr, t->bpp);
}

#endif	// #ifndef NO_TGA


//////////////////////////////////////////////////////////////////////////////
//
// BMP
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_BMP

#pragma pack(push, 1)

struct BmpHeader
{ 
	//
	// BITMAPFILEHEADER
	//

	u16 bfType;			// "BM"
	u32 bfSize;			// of file
	u16 bfReserved1;
	u16 bfReserved2;
	u32 bfOffBits;		// offset to image data


	//
	// BITMAPINFOHEADER
	//

	u32 biSize;
	long biWidth;
	long biHeight;
	u16 biPlanes;
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;

	// the following are unused and zeroed when writing:
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
};

#pragma pack(pop)

#define BI_RGB 0		// biCompression


static inline bool bmp_fmt(const u8* p, size_t size)
{
	UNUSED(size);	// size >= 4, we only need 2 bytes

	// check header signature (bfType == "BM"?).
	// we compare single bytes to be endian-safe.
	return p[0] == 'B' && p[1] == 'M';
}


static inline bool bmp_ext(const char* ext)
{
	return !stricmp(ext, ".bmp");
}


// requirements: uncompressed, direct color, bottom up
static int bmp_decode(TexInfo* t, const char* fn, u8* in, size_t file_size)
{
	const char* err = 0;

	const size_t hdr_size = sizeof(BmpHeader);

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		err = "header not completely read";
fail:
		debug_out("bmp_decode: %s: %s\n", fn, err);
		return ERR_CORRUPTED;
	}

	const BmpHeader* hdr = (const BmpHeader*)in;
	const long w       = (long)read_le32(&hdr->biWidth);
	const long h_      = (long)read_le32(&hdr->biHeight);
	const u16 bpp      = read_le16(&hdr->biBitCount);
	const u32 compress = read_le32(&hdr->biCompression);
	const u32 ofs      = read_le32(&hdr->bfOffBits);

	const TexOrientation fo = (h_ < 0)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	const long h = abs(h_);

	const size_t pitch = w * bpp/8;
	const size_t img_size = h * pitch;
	void* img = in + ofs;

	flip_tex_rows(img, pitch, h, fo);

	int flags = TEX_BGR;
	if(bpp == 32)
		flags |= TEX_ALPHA;

	if(compress != BI_RGB)
		err = "compressed";
	if(bpp != 24 && bpp != 32)
		err = "invalid bpp (not direct color)";
	if(file_size < ofs+img_size)
		err = "image not completely read";
	if(err)
		goto fail;

	t->ofs   = ofs;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	return 0;
}


static int bmp_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	CHECK_ERR(fmt_8_or_24_or_32(t->bpp, t->flags));

	const size_t hdr_size = sizeof(BmpHeader);
	const size_t file_size = hdr_size + img_size;

	const BmpHeader hdr =
	{
		// BITMAPFILEHEADER
		0x4d42,				// bfType = 'B','M'
		(u32)file_size,		// bfSize
		0, 0,				// bfReserved1,2
		hdr_size,			// bfOffBits

		// BITMAPINFOHEADER
		40,					// biSize = sizeof(BITMAPINFOHEADER)
		t->w,
		t->h,
		1,					// biPlanes
		t->bpp,
		BI_RGB,				// biCompression
		(u32)img_size,		// biSizeImage
		0, 0, 0, 0			// unused (bi?PelsPerMeter, biClr*)
	};

	const bool flip_bgr = !(t->flags & TEX_BGR);	// BMP is native BGR
	return write_img(fn, &hdr, sizeof(hdr), img, img_size, flip_bgr, t->bpp);
}

#endif	// #ifndef NO_BMP


//////////////////////////////////////////////////////////////////////////////
//
// RAW
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_RAW

// assume bottom-up

static inline bool raw_fmt(const u8* p, size_t size)
{
	UNUSED(p);
	UNUSED(size)

	return true;
}


static inline bool raw_ext(const char* ext)
{
	return !stricmp(ext, ".raw");
}


static int raw_decode(TexInfo* t, const char* fn, u8* in, size_t file_size)
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


static int raw_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	CHECK_ERR(fmt_8_or_24_or_32(t->bpp, t->flags));

	const bool flip_bgr = !(t->flags & TEX_BGR);	// RAW is native BGR
	return write_img(fn, 0, 0, img, img_size, flip_bgr, t->bpp);
}

#endif	// #ifndef NO_RAW


//////////////////////////////////////////////////////////////////////////////
//
// PNG
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_PNG

static inline bool png_fmt(const u8* ptr, size_t size)
{
	UNUSED(size);	// size >= 4, we only need 4 bytes

	// don't use png_sig_cmp, so we don't pull in libpng for
	// this check alone (it might not be used later).
	return *(u32*)ptr == FOURCC('\x89','P','N','G');
}


static inline bool png_ext(const char* ext)
{
	return !stricmp(ext, ".png");
}


// note: it's not worth combining png_encode and png_decode, due to
// libpng read/write interface differences (grr). we at least split
// out alloc_rows, though.


struct PngMemFile
{
	const u8* p;
	size_t size;

	size_t pos;		// 0-initialized if no initializer
};

// pass data from PNG file in memory to libpng
static void png_read(png_struct* png_ptr, u8* data, png_size_t length)
{
	PngMemFile* f = (PngMemFile*)png_ptr->io_ptr;

	void* src = (u8*)(f->p + f->pos);

	// make sure there's enough new data remaining in the buffer
	f->pos += length;
	if(f->pos > f->size)
		png_error(png_ptr, "png_read: not enough data to satisfy request!");

	memcpy(data, src, length);
}


// limitation: palette images aren't supported
static int png_decode(TexInfo* t, const char* fn, u8* file, size_t file_size)
{
	const char* msg = 0;
	int err = -1;

	// freed when ret is reached:
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	RowArray rows = 0;

	// freed when fail is reached:
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

		int flags = 0;
		if(bpp == 32)
			flags |= TEX_ALPHA;

		// make sure format is acceptable
		if(bit_depth != 8)
			msg = "channel precision != 8 bits";
		if(color_type & PNG_COLOR_MASK_PALETTE)
			msg = "color type is invalid (must be direct color)";
		if(msg)
			goto fail;

		const size_t img_size = pitch * h;
		Handle img_hm;
			// cannot free old t->hm until after png_read_end,
			// but need to set to this handle afterwards => need tmp var.
		img = (u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
		if(!img)
		{
			err = ERR_NO_MEM;
			goto fail;
		}

		int ret = alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows);
		if(ret < 0)
		{
			err = ret;
			goto fail;
		}

		png_read_image(png_ptr, (png_bytepp)rows);
		png_read_end(png_ptr, info_ptr);

		assert(f.p == file && f.size == file_size && f.pos == f.size);

		// store image info
		mem_free_h(t->hm);
		t->hm    = img_hm;
		t->ofs   = 0;	// libpng returns decoded image data; no header
		t->w     = w;
		t->h     = h;
		t->bpp   = bpp;
		t->flags = flags;

		err = 0;
	}

	// shared cleanup
	ret:
	free(rows);

	if(png_ptr)
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return err;
}


// write libpng output to PNG file
static void png_write(png_struct* png_ptr, u8* data, png_size_t length)
{
	void* p = (void*)data;
	Handle hf = *(Handle*)png_ptr->io_ptr;
	if(vfs_io(hf, length, &p) != length)
		png_error(png_ptr, "png_write: !");
}

static void png_flush(png_structp png_ptr)
{
}


// limitation: palette images aren't supported
static int png_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	const char* msg = 0;
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
		png_set_write_fn(png_ptr, &hf, png_write, png_flush);

		png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		int ret = alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows);
		if(ret < 0)
		{
			err = ret;
			goto fail;
		}

		png_set_rows(png_ptr, info_ptr, (png_bytepp)rows);
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);
	
		err = 0;

	}

	// shared cleanup
	ret:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(rows);
	vfs_close(hf);

	return err;
}

#endif	// #ifndef NO_PNG


//////////////////////////////////////////////////////////////////////////////
//
// JP2
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_JP2

static inline bool jp2_fmt(u8* p, size_t size)
{
	ONCE(jas_init());

	jas_stream_t* stream = jas_stream_memopen((char*)p, size);
	return jp2_validate(stream) >= 0;
}


static inline bool jp2_ext(const char* ext)
{
	return !stricmp(ext, ".jp2");
}


static int jp2_decode(TexInfo* t, const char* fn, u8* file, size_t file_size)
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
	return ERR_NOT_IMPLEMENTED;
}

#endif	// #ifndef NO_JP2


//////////////////////////////////////////////////////////////////////////////
//
// JPG
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NO_JPG

static inline bool jpg_fmt(const u8* p, size_t size)
{
	UNUSED(size);	// size >= 4, we only need 2 bytes

	// JFIF requires SOI marker at start of stream.
	// we compare single bytes to be endian-safe.
	return p[0] == 0xff && p[1] == 0xd8;
}


static inline bool jpg_ext(const char* ext)
{
	return !stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg");
}


//
// error handler, shared by jpg_(en|de)code:
//
// the JPEG library's standard error handler (jerror.c) is divided into
// several "methods" which we can override individually. This allows
// adjusting the behavior without duplicating a lot of code, which may
// have to be updated with each future release.
//
// we here override error_exit to return control to the library's caller
// (i.e. jpg_(de|en)code) when a fatal error occurs, rather than calling exit.
//
// the replacement error_exit does a longjmp back to the caller's
// setjmp return point. it needs access to the jmp_buf,
// so we store it in a "subclass" of jpeg_error_mgr.
//

struct JpgErrMgr
{
	struct jpeg_error_mgr pub;	// "public" fields

	jmp_buf call_site; // jump here (back to JPEG lib caller) on error 
	char msg[JMSG_LENGTH_MAX];	// description of first error encountered
		// must store per JPEG context for thread safety.
		// initialized as part of JPEG context error setup.
};

METHODDEF(void) jpg_error_exit(j_common_ptr cinfo)
{
	// get subclass
	JpgErrMgr* err_mgr = (JpgErrMgr*)cinfo->err;

	// "output" error message (i.e. store in JpgErrMgr;
	// call_site is responsible for displaying it via debug_out)
	(*cinfo->err->output_message)(cinfo);

	// jump back to call site, i.e. jpg_(de|en)code
	longjmp(err_mgr->call_site, 1);
}


// stores message in JpgErrMgr for later output by jpg_(de|en)code.
// note: don't display message here, so the caller can
//   add some context (whether encoding or decoding, and filename).
METHODDEF(void) jpg_output_message(j_common_ptr cinfo)
{
	// get subclass
	JpgErrMgr* err_mgr = (JpgErrMgr*)cinfo->err;

	// this context already had an error message; don't overwrite it.
	// (subsequent errors probably aren't related to the real problem).
	// note: was set to '\0' by JPEG context error setup.
	if(err_mgr->msg[0] != '\0')
		return;

	// generate the message and store it
	(*cinfo->err->format_message)(cinfo, err_mgr->msg);
}


static int jpg_decode(TexInfo* t, const char* fn, u8* file, size_t file_size)
{
	const char* msg = 0;
	int err = -1;

	// freed when ret is reached:
	struct jpeg_decompress_struct cinfo;
		// contains the JPEG decompression parameters and pointers to
		// working space (allocated as needed by the JPEG library).
	RowArray rows = 0;
		// array of pointers to scanlines in img, set by alloc_rows.
		// jpeg won't output more than a few scanlines at a time,
		// so we need an output loop anyway, but passing at least 2..4
		// rows is more efficient in low-quality modes (due to less copying).

	// freed when fail is reached:
	u8* img = 0;	// decompressed image memory

	// set up our error handler, which overrides jpeg's default
	// write-to-stderr-and-exit behavior.
	// notes:
	// - must be done before jpeg_create_decompress, in case that fails
	//   (unlikely, but possible if out of memory).
	// - valid over cinfo lifetime (avoids dangling pointer in cinfo)
	JpgErrMgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpg_error_exit;
	jerr.pub.output_message = jpg_output_message;
	jerr.msg[0] = '\0';
		// required for "already have message" check in output_message
	if(setjmp(jerr.call_site))
	{
fail:
		// either JPEG has raised an error, or code below failed.
		// warn user, and skip to cleanup code.
		debug_out("jpg_decode: %s: %s\n", fn, msg? msg : "unknown");
		goto ret;
	}


	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, file, file_size);


	//
	// read header, determine format
	//

	(void) jpeg_read_header(&cinfo, TRUE);
		// we can ignore the return value since:
		// - suspension is not possible with the mem data source
		// - we passed TRUE to raise an error if table-only JPEG file

	int bpp = cinfo.num_components * 8;
		// preliminary; set below to reflect output params

	// make sure we get a color format we know
	// (exception: if bpp = 8, go grayscale below)
	// necessary to support non-standard CMYK files written by Photoshop.
	cinfo.out_color_space = JCS_RGB;

	int flags = 0;
	if(bpp == 8)
	{
		flags |= TEX_GRAY;
		cinfo.out_color_space = JCS_GRAYSCALE;
	}

	// lower quality, but faster
	cinfo.dct_method = JDCT_IFAST;
	cinfo.do_fancy_upsampling = FALSE;


	(void) jpeg_start_decompress(&cinfo);
		// we can ignore the return value since
		// suspension is not possible with the mem data source.

	// scaled output image dimensions and final bpp are now available.
	int w = cinfo.output_width;
	int h = cinfo.output_height;
	bpp = cinfo.output_components * 8;

	// note: since we've set out_color_space, JPEG will always
	// return an acceptable image format; no need to check.


	//
	// allocate memory for uncompressed image
	//

	size_t pitch = w * bpp / 8;
		// needed by alloc_rows
	const size_t img_size = pitch * h;
	Handle img_hm;
		// cannot free old t->hm until after jpeg_finish_decompress,
		// but need to set to this handle afterwards => need tmp var.
	img = (u8*)mem_alloc(img_size, 64*KB, 0, &img_hm);
	if(!img)
	{
		err = ERR_NO_MEM;
		goto fail;
	}
	int ret = alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows);
	if(ret < 0)
	{
		err = ret;
		goto fail;
	}


	// could use cinfo.output_scanline to keep track of progress,
	// but we need to count lines_left anyway (paranoia).
	JSAMPARRAY row = (JSAMPARRAY)rows;
	JDIMENSION lines_left = h;
	while(lines_left != 0)
	{
		JDIMENSION lines_read = jpeg_read_scanlines(&cinfo, row, lines_left);
		row += lines_read;
		lines_left -= lines_read;

		// we've decoded in-place; no need to further process
	}

	(void)jpeg_finish_decompress(&cinfo);
		// we can ignore the return value since suspension
		// is not possible with the mem data source.

	if(jerr.pub.num_warnings != 0)
		debug_out("jpg_decode: corrupt-data warning(s) occurred\n");

	// store image info
	mem_free_h(t->hm);
	t->hm    = img_hm;
	t->ofs   = 0;	// jpeg returns decoded image data; no header
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	err = 0;

	// shared cleanup
	ret:
	free(rows);

	jpeg_destroy_decompress(&cinfo);
		// releases a "good deal" of memory

	return err;
}



// limitation: palette images aren't supported
static int jpg_encode(TexInfo* t, const char* fn, u8* img, size_t img_size)
{
	const char* msg = 0;
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
		png_set_write_fn(png_ptr, &hf, png_write, png_flush);

		png_set_IHDR(png_ptr, info_ptr, w, h, 8, color_type,
			PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		int ret = alloc_rows(img, h, pitch, TEX_TOP_DOWN, rows);
		if(ret < 0)
		{
			err = ret;
			goto fail;
		}

		png_set_rows(png_ptr, info_ptr, (png_bytepp)rows);
		png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);

		err = 0;

	}

	// shared cleanup
	ret:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	free(rows);
	vfs_close(hf);

	return err;
}

#endif	// #ifndef NO_JPG


//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////



static const Codec codecs[] =
{
#ifndef NO_DDS
	CODEC(dds),
#endif
#ifndef NO_PNG
	CODEC(png),
#endif
#ifndef NO_JPG
	CODEC(jpg),
#endif
#ifndef NO_BMP
	CODEC(bmp),
#endif
#ifndef NO_TGA
	CODEC(tga),
#endif
#ifndef NO_JP2
	CODEC(jp2),
#endif

// must be last, as raw_fmt always returns true!
#ifndef NO_RAW
	CODEC(raw)
#endif

};

static const int num_codecs = ARRAY_SIZE(codecs);



int tex_load_mem(Handle hm, const char* fn, TexInfo* t)
{
	size_t size;
	void* _p = mem_get_ptr(hm, &size);

	// guarantee *_valid routines 4 header bytes
	if(size < 4)
		return ERR_CORRUPTED;
	t->hm = hm;

	int err = -1;
	// initial value, in case no codec is found

	u8* p = (u8*)_p;
		// more convenient to pass loaders u8 - less casting.
		// not const, because image may have to be flipped (in-place).

	// find codec that understands the data, and decode
	const Codec* c = codecs;
	for(int i = 0; i < num_codecs; i++, c++)
		if(c->is_fmt(p, size))
			return c->decode(t, fn, p, size);

	return ERR_UNKNOWN_FORMAT;
}


int tex_load(const char* fn, TexInfo* t)
{
	// load file
	void* p; size_t size;	// unused
	Handle hm = vfs_load(fn, p, size);
	CHECK_ERR(hm);	// (need handle below; can't test return value directly)
	int ret = tex_load_mem(hm, fn, t);
	mem_free_h(hm);
	if(ret < 0)
		memset(t, 0, sizeof(TexInfo));
	return ret;
}


int tex_free(TexInfo* t)
{
	mem_free_h(t->hm);
	return 0;
}


int tex_write(const char* fn, int w, int h, int bpp, int flags, void* img)
{
	const size_t img_size = w * h * bpp / 8;

	const char* ext = strrchr(fn, '.');
	if(!ext)
		return -1;

	TexInfo t =
	{
		0,	// handle - not needed by encoders
		0,	// image data offset
		w, h, bpp, flags
	};

	const Codec* c = codecs;
	for(int i = 0; i < num_codecs; i++, c++)
		if(c->is_ext(ext))
		{
			CHECK_ERR(c->encode(&t, fn, (u8*)img, img_size));
			return 0;
		}

	// no codec found
	return ERR_UNKNOWN_FORMAT;
}
