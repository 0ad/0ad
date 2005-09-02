#include "precompiled.h"

#include "lib/byte_order.h"
#include "tex_codec.h"

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


static int bmp_transform(Tex* t, int new_flags)
{
	return TEX_CODEC_CANNOT_HANDLE;
}


// requirements: uncompressed, direct colour, bottom up
static int bmp_decode(u8* file, size_t file_size, Tex* t, const char** perr_msg)
{
	// check header signature (bfType == "BM"?).
	// we compare single bytes to be endian-safe.
	if(file[0] != 'B' || file[1] != 'M')
		return TEX_CODEC_CANNOT_HANDLE;

	const size_t hdr_size = sizeof(BmpHeader);

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		*perr_msg = "header not completely read";
fail:
		return ERR_CORRUPTED;
	}

	const BmpHeader* hdr = (const BmpHeader*)file;
	const long w       = (long)read_le32(&hdr->biWidth);
	const long h_      = (long)read_le32(&hdr->biHeight);
	const u16 bpp      = read_le16(&hdr->biBitCount);
	const u32 compress = read_le32(&hdr->biCompression);
	const u32 ofs      = read_le32(&hdr->bfOffBits);

	const int orientation = (h_ < 0)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	const long h = abs(h_);
	const size_t pitch = w * bpp/8;
	const size_t img_size = h * pitch;
	u8* const img = file + ofs;

	int flags = TEX_BGR;
	if(bpp == 32)
		flags |= TEX_ALPHA;

	// sanity checks
	const char* err = 0;
	if(compress != BI_RGB)
		err = "compressed";
	if(bpp != 24 && bpp != 32)
		err = "invalid bpp (not direct colour)";
	if(file_size < ofs+img_size)
		err = "image not completely read";
	if(err)
	{
		*perr_msg = err;
		goto fail;
	}

	t->ofs   = ofs;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	tex_codec_set_orientation(t, orientation);

	return 0;
}


static int bmp_encode(const char* ext, Tex* t, u8** out, size_t* out_size, const char** perr_msg)
{
	if(stricmp(ext, "bmp"))
		return TEX_CODEC_CANNOT_HANDLE;
/*
	CHECK_ERR(fmt_8_or_24_or_32(t->bpp, t->flags));

	const size_t hdr_size = sizeof(BmpHeader);	// needed for BITMAPFILEHEADER
	const size_t file_size = hdr_size + img_size;
	const long h = (t->flags & TEX_TOP_DOWN)? -(int)t->h : t->h;

	int transforms = t->flags;
	transforms &= ~TEX_ORIENTATION;	// no flip needed - we can set top-down bit.
	transforms ^= TEX_BGR;			// BMP is native BGR.
	transform(t, img, transforms);

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
		h,
		1,					// biPlanes
		t->bpp,
		BI_RGB,				// biCompression
		(u32)img_size,		// biSizeImage
		0, 0, 0, 0			// unused (bi?PelsPerMeter, biClr*)
	};

	return write_img(fn, &hdr, sizeof(hdr), img, img_size);
*/
	return 0;
}

TEX_CODEC_REGISTER(bmp);
