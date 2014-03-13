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
 * TGA codec.
 */

#include "precompiled.h"

#include "lib/byte_order.h"
#include "tex_codec.h"
#include "lib/bits.h"

#pragma pack(push, 1)

enum TgaImgType
{
	TGA_TRUE_COLOUR = 2,	// uncompressed 24 or 32 bit direct RGB
	TGA_GREY        = 3		// uncompressed 8 bit direct greyscale
};

enum TgaImgDesc
{
	TGA_RIGHT_TO_LEFT = BIT(4),
	TGA_TOP_DOWN      = BIT(5),
};

typedef struct
{
	u8 img_id_len;			// 0 - no image identifier present
	u8 colour_map_type;		// 0 - no colour map present
	u8 img_type;			// see TgaImgType
	u8 colour_map[5];		// unused

	u16 x_origin;			// unused
	u16 y_origin;			// unused

	u16 w;
	u16 h;
	u8 bpp;					// bits per pixel

	u8 img_desc;
}
TgaHeader;

// TGA file: header [img id] [colour map] image data

#pragma pack(pop)


Status TexCodecTga::transform(Tex* UNUSED(t), size_t UNUSED(transforms)) const
{
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


bool TexCodecTga::is_hdr(const u8* file) const
{
	TgaHeader* hdr = (TgaHeader*)file;

	// the first TGA header doesn't have a magic field;
	// we can only check if the first 4 bytes are valid
	// .. not direct colour
	if(hdr->colour_map_type != 0)
		return false;
	// .. wrong colour type (not uncompressed greyscale or RGB)
	if(hdr->img_type != TGA_TRUE_COLOUR && hdr->img_type != TGA_GREY)
		return false;

	// note: we can't check img_id_len or colour_map[0] - they are
	// undefined and may assume any value.

	return true;
}


bool TexCodecTga::is_ext(const OsPath& extension) const
{
	return extension == L".tga";
}


size_t TexCodecTga::hdr_size(const u8* file) const
{
	size_t hdr_size = sizeof(TgaHeader);
	if(file)
	{
		TgaHeader* hdr = (TgaHeader*)file;
		hdr_size += hdr->img_id_len;
	}
	return hdr_size;
}


// requirements: uncompressed, direct color, bottom up
Status TexCodecTga::decode(rpU8 data, size_t UNUSED(size), Tex* RESTRICT t) const
{
	const TgaHeader* hdr = (const TgaHeader*)data;
	const u8 type  = hdr->img_type;
	const size_t w   = read_le16(&hdr->w);
	const size_t h   = read_le16(&hdr->h);
	const size_t bpp = hdr->bpp;
	const u8 desc  = hdr->img_desc;

	size_t flags = 0;
	flags |= (desc & TGA_TOP_DOWN)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	if(desc & 0x0F)	// alpha bits
		flags |= TEX_ALPHA;
	if(bpp == 8)
		flags |= TEX_GREY;
	if(type == TGA_TRUE_COLOUR)
		flags |= TEX_BGR;

	// sanity checks
	// .. storing right-to-left is just stupid;
	//    we're not going to bother converting it.
	if(desc & TGA_RIGHT_TO_LEFT)
		WARN_RETURN(ERR::TEX_INVALID_LAYOUT);

	t->m_Width  = w;
	t->m_Height = h;
	t->m_Bpp    = bpp;
	t->m_Flags  = flags;
	return INFO::OK;
}


Status TexCodecTga::encode(Tex* RESTRICT t, DynArray* RESTRICT da) const
{
	u8 img_desc = 0;
	if(t->m_Flags & TEX_TOP_DOWN)
		img_desc |= TGA_TOP_DOWN;
	if(t->m_Bpp == 32)
		img_desc |= 8;	// size of alpha channel
	TgaImgType img_type = (t->m_Flags & TEX_GREY)? TGA_GREY : TGA_TRUE_COLOUR;

	size_t transforms = t->m_Flags;
	transforms &= ~TEX_ORIENTATION;	// no flip needed - we can set top-down bit.
	transforms ^= TEX_BGR;			// TGA is native BGR.

	const TgaHeader hdr =
	{
		0,				// no image identifier present
		0,				// no colour map present
		(u8)img_type,
		{0,0,0,0,0},	// unused (colour map)
		0, 0,			// unused (origin)
		(u16)t->m_Width,
		(u16)t->m_Height,
		(u8)t->m_Bpp,
		img_desc
	};
	const size_t hdr_size = sizeof(hdr);
	return tex_codec_write(t, transforms, &hdr, hdr_size, da);
}

