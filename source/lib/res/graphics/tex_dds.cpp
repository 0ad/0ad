#include "precompiled.h"

#include "lib/byte_order.h"
#include "lib/res/mem.h"
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


// pixel colors are stored as uint[4]. uint rather than u8 protects from
// overflow during calculations, and padding to an even size is a bit
// more efficient (even though we don't need the alpha component).
enum RGBA {	R, G, B, A };

// DXT1a requires special handling in precalc_color (alpha cannot
// be determined later because it depends on the color block).
// we encode this as another value in <dxt> because it's easier than
// passing around the TEX_ALPHA flag.
const int DXT1A = 11;



static inline void mix_2_3(uint dst[4], uint c0[4], uint c1[4])
{
	for(int i = 0; i < 3; i++) dst[i] = (c0[i]*2 + c1[i] + 1)/3;
}

static inline void mix_avg(uint dst[4], uint c0[4], uint c1[4])
{
	for(int i = 0; i < 3; i++) dst[i] = (c0[i]+c1[i])/2;
}

static inline uint access_bit_tbl(u32 tbl, uint idx, uint bit_width)
{
	uint val = tbl >> (idx*bit_width);
	val &= (1u << bit_width)-1;
	return val;
}

static inline uint access_bit_tbl64(u64 tbl, uint idx, uint bit_width)
{
	uint val = (uint)(tbl >> (idx*bit_width));
	val &= (1u << bit_width)-1;
	return val;
}

// extract a range of bits and expand to 8 bits (by replicating
// MS bits - see http://www.mindcontrol.org/~hplus/graphics/expand-bits.html ;
// this is also the algorithm used by graphics cards when decompressing S3TC).
// used to convert 565 to 32bpp RGB.
static uint unpack_to_8(u16 c, uint bits_below, uint num_bits)
{
	const uint num_filler_bits = 8-num_bits;
	const uint field = bits(c, bits_below, bits_below+num_bits-1);
	const uint filler = field >> (8-num_bits);
	return (field << num_filler_bits) | filler;
}


// for efficiency, we precalculate as much as possible about a block
// and store it here.
struct S3tcBlock
{
	// the 4 color choices for each pixel (RGBA)
	uint c[4][4];	// c[i][RGBA_component]

	// (DXT5 only) the 8 alpha choices
	u8 dxt5_a_tbl[8];

	// alpha block; interpretation depends on dxt.
	u64 a_bits;

	// table of 2-bit color selectors
	u32 c_selectors;
};


static void precalc_alpha(int dxt, const u8* a_block, S3tcBlock* b)
{
	// read block contents
	const uint a0 = a_block[0], a1 = a_block[1];
	b->a_bits = read_le64(a_block);	// see below

	if(dxt == 5)
	{
		// skip a0,a1 bytes (data is little endian)
		b->a_bits >>= 16;

		const bool is_dxt5_special_combination = (a0 <= a1);
		u8* a = b->dxt5_a_tbl;	// shorthand
		if(is_dxt5_special_combination)
		{
			a[0] = a0;
			a[1] = a1;
			a[2] = (4*a0 + 1*a1 + 2)/5;
			a[3] = (3*a0 + 2*a1 + 2)/5;
			a[4] = (2*a0 + 3*a1 + 2)/5;
			a[5] = (1*a0 + 4*a1 + 2)/5;
			a[6] = 0;
			a[7] = 255;
		}
		else
		{
			a[0] = a0;
			a[1] = a1;
			a[2] = (6*a0 + 1*a1 + 3)/7;
			a[3] = (5*a0 + 2*a1 + 3)/7;
			a[4] = (4*a0 + 3*a1 + 3)/7;
			a[5] = (3*a0 + 4*a1 + 3)/7;
			a[6] = (2*a0 + 5*a1 + 3)/7;
			a[7] = (1*a0 + 6*a1 + 3)/7;
		}
	}
}


static void precalc_color(int dxt, const u8* c_block, S3tcBlock* b)
{
	// read block contents
	// .. S3TC reference colors (565 format). the color table is generated
	//    from some combination of these, depending on their ordering.
	u16 rc[2];
	for(int i = 0; i < 2; i++)
		rc[i] = read_le16(c_block + 2*i);
	// .. table of 2-bit color selectors
	b->c_selectors = read_le32(c_block+4);

	const bool is_dxt1_special_combination =
		(dxt == 1 || dxt == DXT1A) && rc[0] <= rc[1];

	// c0 and c1 are the values of rc[], converted to 32bpp
	for(int i = 0; i < 2; i++)
	{
		b->c[i][R] = unpack_to_8(rc[i], 11, 5);
		b->c[i][G] = unpack_to_8(rc[i],  5, 6);
		b->c[i][B] = unpack_to_8(rc[i],  0, 5);
	}

	// c2 and c3 are combinations of c0 and c1:
	if(is_dxt1_special_combination)
	{
		mix_avg(b->c[2], b->c[0], b->c[1]);			// c2 = (c0+c1)/2
		for(int i = 0; i < 3; i++) b->c[3][i] = 0;	// c3 = black
		b->c[3][A] = (dxt == DXT1A)? 0 : 255;		// (transparent iff DXT1a)
	}
	else
	{
		mix_2_3(b->c[2], b->c[0], b->c[1]);			// c2 = 2/3*c0 + 1/3*c1
		mix_2_3(b->c[3], b->c[1], b->c[0]);			// c3 = 1/3*c0 + 2/3*c1
	}
}


static void block_precalc(int dxt, const u8* block, S3tcBlock* b)
{
	// (careful, 'dxt != 1' doesn't work)
	const u8* a_block = block;
	const u8* c_block = (dxt == 3 || dxt == 5)? block+8 : block;

	precalc_alpha(dxt, a_block, b);
	precalc_color(dxt, c_block, b);
}


static void write_pixel(int dxt, uint pixel_idx, const S3tcBlock* b, u8* out)
{
	debug_assert(pixel_idx < 16);

	// pixel index -> color selector (2 bit) -> color
	const uint c_selector = access_bit_tbl(b->c_selectors, pixel_idx, 2);
	const uint* c = b->c[c_selector];
	for(int i = 0; i < 3; i++)
		out[i] = c[i];

	// if no alpha, done
	if(dxt == 1)
		return;

	uint a;
	if(dxt == 3)
	{
		// table of 4-bit alpha entries
		a = access_bit_tbl64(b->a_bits, pixel_idx, 4);
		a |= a << 4; // expand to 8 bits (replicate high into low!)
	}
	else if(dxt == 5)
	{
		// pixel index -> alpha selector (3 bit) -> alpha
		const uint a_selector = access_bit_tbl64(b->a_bits, pixel_idx, 3);
		a = b->dxt5_a_tbl[a_selector];
	}
	// (dxt == DXT1A)
	else
		a = c[A];
	out[A] = a;
}



// in ogl_emulate_dds:	debug_assert(compressedimageSize == blocks * (dxt1? 8 : 16));


// note: this code is grossly inefficient (mostly due to splitting it up
// into function calls for readability). that's because it's only used to
// emulate hardware S3TC support - if that isn't available, everything will
// be dog-slow anyway due to increased vmem usage.
static int dds_decompress(Tex* t)
{
	int dxt = t->flags & TEX_DXT;
	debug_assert(dxt == 1 || dxt == 3 || dxt == 5);
	if(t->flags & TEX_ALPHA)
		dxt = DXT1A;
	// due to the above, dxt == 1 is the only non-alpha case.
	// note: adding or stripping alpha channels during transform is not
	// our job; we merely output the same pixel format as given
	// (tex.cpp's plain transform could cover it, if ever needed).
	const uint bpp = (dxt != 1)? 32 : 24;

	// note: 1x1 images are legitimate (e.g. in mipmaps). they report their
	// width as such for glTexImage, but the S3TC data is padded to
	// 4x4 pixel block boundaries.
	const uint blocks_w = (uint)(round_up(t->w, 4) / 4);
	const uint blocks_h = (uint)(round_up(t->h, 4) / 4);
	const uint blocks = blocks_w * blocks_h;
	const size_t img_size = blocks * 16 * bpp/8;
	Handle hm;
	void* img_data = mem_alloc(img_size, 64*KiB, 0, &hm);

	const u8* s3tc_data = (const u8*)tex_get_data(t);
	const size_t s3tc_size = tex_img_size(t);

	for(uint block_y = 0; block_y < blocks_h; block_y++)
		for(uint block_x = 0; block_x < blocks_w; block_x++)
		{
			S3tcBlock b;
			block_precalc(dxt, s3tc_data, &b);
			s3tc_data += 16 * t->bpp/8;

			uint pixel_idx = 0;
			for(int y = 0; y < 4; y++)
			{
				u8* out = (u8*)img_data + ((block_y*4+y)*blocks_w*4 + block_x*4) * bpp/8;
				for(int x = 0; x < 4; x++)
				{
					write_pixel(dxt, pixel_idx, &b, out);
					out += bpp/8;
					pixel_idx++;
				}
			}
		}	// for block_x


	debug_assert(tex_get_data(t) == s3tc_data - s3tc_size);

	mem_free_h(t->hm);
	t->hm  = hm;
	t->ofs = 0;
	t->bpp = bpp;
	t->flags &= ~TEX_DXT;
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
