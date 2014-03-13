/* Copyright (c) 2010 Wildfire Games
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
 * Windows BMP codec
 */

#include "precompiled.h"

#include "lib/byte_order.h"
#include "tex_codec.h"

#pragma pack(push, 1)

struct BmpHeader
{ 
	// BITMAPFILEHEADER
	u16 bfType;			// "BM"
	u32 bfSize;			// of file
	u16 bfReserved1;
	u16 bfReserved2;
	u32 bfOffBits;		// offset to image data

	// BITMAPINFOHEADER
	u32 biSize;
	i32 biWidth;
	i32 biHeight;
	u16 biPlanes;
	u16 biBitCount;
	u32 biCompression;
	u32 biSizeImage;
	// the following are unused and zeroed when writing:
	i32 biXPelsPerMeter;
	i32 biYPelsPerMeter;
	u32 biClrUsed;
	u32 biClrImportant;
};

#pragma pack(pop)

#define BI_RGB 0		// biCompression


Status TexCodecBmp::transform(Tex* UNUSED(t), size_t UNUSED(transforms)) const
{
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


bool TexCodecBmp::is_hdr(const u8* file) const
{
	// check header signature (bfType == "BM"?).
	// we compare single bytes to be endian-safe.
	return (file[0] == 'B' && file[1] == 'M');
}


bool TexCodecBmp::is_ext(const OsPath& extension) const
{
	return extension == L".bmp";
}


size_t TexCodecBmp::hdr_size(const u8* file) const
{
	const size_t hdr_size = sizeof(BmpHeader);
	if(file)
	{
		BmpHeader* hdr = (BmpHeader*)file;
		const u32 ofs = read_le32(&hdr->bfOffBits);
		ENSURE(ofs >= hdr_size && "bmp_hdr_size invalid");
		return ofs;
	}
	return hdr_size;
}


// requirements: uncompressed, direct colour, bottom up
Status TexCodecBmp::decode(rpU8 data, size_t UNUSED(size), Tex* RESTRICT t) const
{
	const BmpHeader* hdr = (const BmpHeader*)data;
	const long w       = (long)read_le32(&hdr->biWidth);
	const long h_      = (long)read_le32(&hdr->biHeight);
	const u16 bpp      = read_le16(&hdr->biBitCount);
	const u32 compress = read_le32(&hdr->biCompression);

	const long h = abs(h_);

	size_t flags = 0;
	flags |= (h_ < 0)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	if(bpp > 16)
		flags |= TEX_BGR;
	if(bpp == 32)
		flags |= TEX_ALPHA;

	// sanity checks
	if(compress != BI_RGB)
		WARN_RETURN(ERR::TEX_COMPRESSED);

	t->m_Width  = w;
	t->m_Height = h;
	t->m_Bpp    = bpp;
	t->m_Flags  = flags;
	return INFO::OK;
}


Status TexCodecBmp::encode(Tex* RESTRICT t, DynArray* RESTRICT da) const
{
	const size_t hdr_size = sizeof(BmpHeader);	// needed for BITMAPFILEHEADER
	const size_t img_size = t->img_size();
	const size_t file_size = hdr_size + img_size;
	const i32 h = (t->m_Flags & TEX_TOP_DOWN)? -(i32)t->m_Height : (i32)t->m_Height;

	size_t transforms = t->m_Flags;
	transforms &= ~TEX_ORIENTATION;	// no flip needed - we can set top-down bit.
	transforms ^= TEX_BGR;			// BMP is native BGR.

	const BmpHeader hdr =
	{
		// BITMAPFILEHEADER
		0x4D42,				// bfType = 'B','M'
		(u32)file_size,		// bfSize
		0, 0,				// bfReserved1,2
		hdr_size,			// bfOffBits

		// BITMAPINFOHEADER
		40,					// biSize = sizeof(BITMAPINFOHEADER)
		(i32)t->m_Width,
		h,
		1,					// biPlanes
		(u16)t->m_Bpp,
		BI_RGB,				// biCompression
		(u32)img_size,		// biSizeImage
		0, 0, 0, 0			// unused (bi?PelsPerMeter, biClr*)
	};
	return tex_codec_write(t, transforms, &hdr, hdr_size, da);
}
