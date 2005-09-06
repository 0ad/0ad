#include "precompiled.h"

#include "lib/byte_order.h"
#include "tex_codec.h"


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


static int tga_transform(Tex* UNUSED(t), uint UNUSED(transforms))
{
	return TEX_CODEC_CANNOT_HANDLE;
}


// requirements: uncompressed, direct colour, bottom up
static int tga_decode(u8* file, size_t file_size, Tex* t, const char** perr_msg)
{
	TgaHeader* hdr = (TgaHeader*)file;

	// the first TGA header doesn't have a magic field;
	// we can only check if the first 4 bytes are valid
	// .. not direct colour
	if(hdr->colour_map_type != 0)
		return TEX_CODEC_CANNOT_HANDLE;
	// .. wrong colour type (not uncompressed greyscale or RGB)
	if(hdr->img_type != TGA_TRUE_COLOUR && hdr->img_type != TGA_GREY)
		return TEX_CODEC_CANNOT_HANDLE;

	// make sure we can access all header fields
	const size_t hdr_size = sizeof(TgaHeader) + hdr->img_id_len;
	if(file_size < hdr_size)
	{
		*perr_msg = "header not completely read";
fail:
		return ERR_CORRUPTED;
	}

	const u8 type  = hdr->img_type;
	const uint w   = read_le16(&hdr->w);
	const uint h   = read_le16(&hdr->h);
	const uint bpp = hdr->bpp;
	const u8 desc  = hdr->img_desc;

	const u8 alpha_bits = desc & 0x0f;
	const int orientation = (desc & TGA_TOP_DOWN)? TEX_TOP_DOWN : TEX_BOTTOM_UP;
	const size_t img_size = tex_img_size(t);

	int flags = 0;
	if(alpha_bits != 0)
		flags |= TEX_ALPHA;
	if(bpp == 8)
		flags |= TEX_GREY;
	if(type == TGA_TRUE_COLOUR)
		flags |= TEX_BGR;

	// sanity checks
	const char* err = 0;
	// .. storing right-to-left is just stupid;
	//    we're not going to bother converting it.
	if(desc & TGA_RIGHT_TO_LEFT)
		err = "image is stored right-to-left";
	if(bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
		err = "invalid bpp";
	if(file_size < hdr_size + img_size)
		err = "size < image size";
	if(err)
	{
		*perr_msg = err;
		goto fail;
	}

	t->ofs   = hdr_size;
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;

	tex_codec_set_orientation(t, orientation);

	return 0;
}


static int tga_encode(const char* ext, Tex* t, DynArray* da, const char** UNUSED(perr_msg))
{
	if(stricmp(ext, "tga"))
		return TEX_CODEC_CANNOT_HANDLE;

	u8 img_desc = 0;
	if(t->flags & TEX_TOP_DOWN)
		img_desc |= TGA_TOP_DOWN;
	if(t->bpp == 32)
		img_desc |= 8;	// size of alpha channel
	TgaImgType img_type = (t->flags & TEX_GREY)? TGA_GREY : TGA_TRUE_COLOUR;

	int transforms = t->flags;
	transforms &= ~TEX_ORIENTATION;	// no flip needed - we can set top-down bit.
	transforms ^= TEX_BGR;			// TGA is native BGR.

	const TgaHeader hdr =
	{
		0,				// no image identifier present
		0,				// no colour map present
		(u8)img_type,
		{0,0,0,0,0},	// unused (colour map)
		0, 0,			// unused (origin)
		t->w,
		t->h,
		t->bpp,
		img_desc
	};
	const size_t hdr_size = sizeof(hdr);
	return tex_codec_write(t, transforms, &hdr, hdr_size, da);
}

TEX_CODEC_REGISTER(tga);
