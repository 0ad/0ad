#include "precompiled.h"

#include "lib/byte_order.h"
#include "tex_codec.h"

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


enum RGBA {	R, G, B };


static inline void mix_2_3(uint dst[4], uint c0[4], uint c1[4])
{
	for(int i = 0; i < 3; i++) dst[i] = (c0[i]*2 + c1[i] + 1)/3;
}

static inline void mix_avg(uint dst[4], uint c0[4], uint c1[4])
{
	for(int i = 0; i < 3; i++) dst[i] = (c0[i]+c1[i])/2;
}

static inline uint access_bit_tbl(const u8* ptbl, uint idx, uint bit_width)
{
	const u32 tbl = *(const u32*)ptbl;
	uint val = tbl >> (idx*bit_width);
	val &= (1u << bit_width)-1;
	return val;
}

static inline uint access_bit_tbl64(const u8* ptbl, uint idx, uint bit_width)
{
	const u64 tbl = *(const u64*)ptbl;
	uint val = (uint)(tbl >> (idx*bit_width));
	val &= (1u << bit_width)-1;
	return val;
}


// c = color_tbl (shorthand)
static void precalc_color(int dxt, const u8* color_block, uint c[4][4])
{
	const u16 c0 = *(u16*)(color_block+0);
	const u16 c1 = *(u16*)(color_block+2);

	// Unpack 565, and copy high bits to low bits
	c[0][R] = ((c0>>8)&0xF8) | ((c0>>13)&7);
	c[0][G] = ((c0>>3)&0xFC) | ((c0>>9 )&3);
	c[0][B] = ((c0<<3)&0xF8) | ((c0>>2 )&7);
	c[1][R] = ((c1>>8)&0xF8) | ((c1>>13)&7);
	c[1][G] = ((c1>>3)&0xFC) | ((c1>>9 )&3);
	c[1][B] = ((c1<<3)&0xF8) | ((c1>>2 )&7);

	if((dxt != 1 || c0 > c1))
	{
		mix_2_3(c[2], c[0], c[1]);				// c2 = 2/3*c0 + 1/3*c1
		mix_2_3(c[3], c[1], c[0]);				// c3 = 1/3*c0 + 2/3*c1
	}
	// DXT1 special case: 
	else
	{
		mix_avg(c[2], c[0], c[1]);				// c2 = (c0+c1)/2
		for(int i = 0; i < 4; i++) c[3][i] = 0;	// c3 = black
	}
}


static void precalc_dxt5_alpha(const u8* alpha_block, u8 dxt5_alpha_tbl[8])
{
	const uint a0 = alpha_block[0], a1 = alpha_block[1];

	dxt5_alpha_tbl[0] = a0;
	dxt5_alpha_tbl[1] = a1;
	if(a0 > a1)
	{
		dxt5_alpha_tbl[2] = (6*a0 + 1*a1 + 3)/7;
		dxt5_alpha_tbl[3] = (5*a0 + 2*a1 + 3)/7;
		dxt5_alpha_tbl[4] = (4*a0 + 3*a1 + 3)/7;
		dxt5_alpha_tbl[5] = (3*a0 + 4*a1 + 3)/7;
		dxt5_alpha_tbl[6] = (2*a0 + 5*a1 + 3)/7;
		dxt5_alpha_tbl[7] = (1*a0 + 6*a1 + 3)/7;
	}
	else
	{
		dxt5_alpha_tbl[2] = (4*a0 + 1*a1 + 2)/5;
		dxt5_alpha_tbl[3] = (3*a0 + 2*a1 + 2)/5;
		dxt5_alpha_tbl[4] = (2*a0 + 3*a1 + 2)/5;
		dxt5_alpha_tbl[5] = (1*a0 + 4*a1 + 2)/5;
		dxt5_alpha_tbl[6] = 0;
		dxt5_alpha_tbl[7] = 255;
	}
}


static const uint* choose_color(uint pixel_idx, const u8* color_block, const uint c_tbl[4][4])
{
	debug_assert(pixel_idx < 16);

	// pixel index -> color selector (2 bit) -> color
	const uint c_idx = access_bit_tbl(color_block+4, pixel_idx, 2);
	return c_tbl[c_idx];
}


static uint choose_alpha(int dxt, uint pixel_idx, const u8* alpha_block, const u8 dxt5_alpha_tbl[8])
{
	debug_assert(pixel_idx < 16);

	uint a = 255;
	if(dxt == 3)
	{
		// table of 4-bit alpha entries
		a = access_bit_tbl64(alpha_block, pixel_idx, 4);
		a |= a << 4; // copy low bits to high bits
	}
	else if(dxt == 5)
	{
		// pixel index -> alpha selector (3 bit) -> alpha
		const uint a_idx = access_bit_tbl(alpha_block+2, pixel_idx, 3);
		a = dxt5_alpha_tbl[a_idx];
	}
	return a;
}



// in ogl_emulate_dds:	debug_assert(compressedimageSize == blocks * (dxt1? 8 : 16));


static int dds_decompress(Tex* t)
{
return ERR_NOT_IMPLEMENTED;

	uint w = t->w, h = t->h;
	if(w==0 || h==0 || w%4 || h%4)
		return ERR_TEX_FMT_INVALID;

	int dxt = t->flags & TEX_DXT;
	debug_assert(dxt == 1 || dxt == 3 || dxt == 5);

bool output_rgb = true;

	const u8* data = (const u8*)tex_get_data(t);

	uint blocks_w = (uint)(round_up(w, 4) / 4);
	uint blocks_h = (uint)(round_up(h, 4) / 4);
	uint blocks = blocks_w * blocks_h;
	size_t rgb_size = blocks * 16 * (output_rgb? 3 : 4);
	void* rgb_data = malloc(rgb_size);

	// This code is inefficient, but I don't care:
	for(uint block_y = 0; block_y < blocks_h; block_y++)
		for(uint block_x = 0; block_x < blocks_w; block_x++)
		{
			const u8* alpha_block = data;
			u8 dxt5_alpha_tbl[8];
			if(dxt == 5)
				precalc_dxt5_alpha(alpha_block, dxt5_alpha_tbl);
			if(dxt != 1)
				data += 8;

			const u8* color_block = data;
			uint color_tbl[4][4];	// c[i][RGBA_component]
			precalc_color(dxt, color_block, color_tbl);
			data += 8;

			uint pixel_idx = 0;
			if(output_rgb)
				for(int y = 0; y < 4; y++)
				{
					u8* out = (u8*)rgb_data + ((block_y*4+y)*blocks_w*4 + block_x*4) * 3;
					for(int x = 0; x < 4; x++)
					{
						const uint* c = choose_color(pixel_idx, color_block, color_tbl);
						*out++ = c[R]; *out++ = c[G]; *out++ = c[B];
						pixel_idx++;
					}
				}
			else
				for(int y = 0; y < 4; ++y)
				{
					u8* out = (u8*)rgb_data + ((block_y*4+y)*blocks_w*4 + block_x*4) * 4;
					for(int x = 0; x < 4; ++x)
					{
						const uint* c = choose_color(pixel_idx, color_block, color_tbl);
						*out++ = c[R]; *out++ = c[G]; *out++ = c[B];

						const uint a = choose_alpha(dxt, pixel_idx, alpha_block, dxt5_alpha_tbl);
						*out++ = a;

						pixel_idx++;
					}
				}
		}	// for block_x

	return 0;
}


static int dds_transform(Tex* t, uint transforms)
{
	const int is_dxt = t->flags & TEX_DXT, transform_dxt = transforms & TEX_DXT;
	// requesting decompression
	if(is_dxt && transform_dxt)
		return dds_decompress(t);
	// both are DXT (unsupported; there are no flags we can change while
	// compressed) or requesting compression (not implemented) or
	// both not DXT (nothing we can do) - bail.
	else
		return TEX_CODEC_CANNOT_HANDLE;
}


static bool dds_is_hdr(const u8* file)
{
	return *(u32*)file == FOURCC('D','D','S',' ');
}


static bool dds_is_ext(const char* ext)
{
	return !stricmp(ext, "dds");
}


static size_t dds_hdr_size(const u8* UNUSED(file))
{
	return 4+sizeof(DDSURFACEDESC2);
}


static int dds_decode(DynArray* da, Tex* t)
{
	u8* file         = da->base;

	const DDSURFACEDESC2* hdr = (const DDSURFACEDESC2*)(file+4);
	const u32 sd_size   = read_le32(&hdr->dwSize);
	const u32 sd_flags  = read_le32(&hdr->dwFlags);
	const u32 h         = read_le32(&hdr->dwHeight);
	const u32 w         = read_le32(&hdr->dwWidth);
	      u32 mipmaps   = read_le32(&hdr->dwMipMapCount);
	const u32 pf_size   = read_le32(&hdr->ddpfPixelFormat.dwSize);
	const u32 pf_flags  = read_le32(&hdr->ddpfPixelFormat.dwFlags);
	const u32 fourcc    = hdr->ddpfPixelFormat.dwFourCC;
		// compared against FOURCC, which takes care of endian conversion.

	// we'll use these fields; make sure they're present below.
	// note: we can't guess image dimensions if not specified -
	//       the image isn't necessarily square.
	const u32 sd_req_flags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

	// make sure fields that aren't indicated as valid are zeroed.
	if(!(sd_flags & DDSD_MIPMAPCOUNT))
		mipmaps = 0;


	// determine flags and bpp.
	// we store DXT format (one of {1,3,5}) in flags & TEX_DXT.
	//
	// unfortunately there are problems with some DDS headers:
	// - DXTex doesn't set the required dwPitchOrLinearSize field -
	//   MS can't even write out their own file format correctly. *sigh*
	//   it's needed by OpenGL, so we calculate it from w, h, and bpp.
	// - pf_flags & DDPF_ALPHAPIXELS can only be used to check for
	//   DXT1a (the only way to detect it); we have observed some DXT3 files
	//   that don't have it set. grr
	int bpp = 0;
	int flags = 0;
	switch(fourcc)
	{
	case FOURCC('D','X','T','1'):
		bpp = 4;
		flags |= 1;
		if(pf_flags & DDPF_ALPHAPIXELS)
			flags |= TEX_ALPHA;
		break;
	case FOURCC('D','X','T','3'):
		bpp = 8;
		flags |= 3;
		flags |= TEX_ALPHA;
		break;
	case FOURCC('D','X','T','5'):
		bpp = 8;
		flags |= 5;
		flags |= TEX_ALPHA;
		break;
	}
	if(mipmaps)
		flags |= TEX_MIPMAPS;


	// sanity checks
	// .. dimensions not padded to S3TC block size
	if(w % 4 || h % 4)
		return ERR_TEX_INVALID_SIZE;
	// .. unknown FOURCC
	if((flags & TEX_DXT) == 0)
		return ERR_UNKNOWN_FORMAT;
	// .. missing required field(s)
	if((sd_flags & sd_req_flags) != sd_req_flags)
		return ERR_INCOMPLETE_HEADER;
	if(sizeof(DDPIXELFORMAT) != pf_size)
		return ERR_CORRUPTED;
	if(sizeof(DDSURFACEDESC2) != sd_size)
		return ERR_CORRUPTED;

	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;
	return 0;
}


static int dds_encode(Tex* UNUSED(t), DynArray* UNUSED(da))
{
	// note: do not return ERR_NOT_IMPLEMENTED et al. because that would
	// break tex_write (which assumes either this, 0 or errors are returned).
	return TEX_CODEC_CANNOT_HANDLE;
	
}

TEX_CODEC_REGISTER(dds);
