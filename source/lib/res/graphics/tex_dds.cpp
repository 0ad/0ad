#include "precompiled.h"

#include "lib/byte_order.h"
#include "lib/res/mem.h"
#include "tex_codec.h"

// NOTE: the convention is bottom-up for DDS, but there's no way to tell.

//-----------------------------------------------------------------------------
// S3TC decompression
//-----------------------------------------------------------------------------

// note: this code is not so efficient (mostly due to splitting it up
// into function calls for readability). that's because it's only used to
// emulate hardware S3TC support - if that isn't available, everything will
// be dog-slow anyway due to increased vmem usage.

// pixel colors are stored as uint[4]. uint rather than u8 protects from
// overflow during calculations, and padding to an even size is a bit
// more efficient (even though we don't need the alpha component).
enum RGBA {	R, G, B, A };


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
static inline uint unpack_to_8(u16 c, uint bits_below, uint num_bits)
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

	uint dxt;
};


static void s3tc_precalc_alpha(uint dxt, const u8* restrict a_block, S3tcBlock* restrict b)
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


static void s3tc_precalc_color(uint dxt, const u8* restrict c_block, S3tcBlock* restrict b)
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


static void s3tc_precalc_block(uint dxt, const u8* restrict block, S3tcBlock* restrict b)
{
	b->dxt = dxt;

	// (careful, 'dxt != 1' doesn't work - there's also DXT1a)
	const u8* a_block = block;
	const u8* c_block = (dxt == 3 || dxt == 5)? block+8 : block;

	s3tc_precalc_alpha(dxt, a_block, b);
	s3tc_precalc_color(dxt, c_block, b);
}


static void s3tc_write_pixel(const S3tcBlock* restrict b, uint pixel_idx, u8* restrict out)
{
	debug_assert(pixel_idx < 16);

	// pixel index -> color selector (2 bit) -> color
	const uint c_selector = access_bit_tbl(b->c_selectors, pixel_idx, 2);
	const uint* c = b->c[c_selector];
	for(int i = 0; i < 3; i++)
		out[i] = c[i];

	// if no alpha, done
	if(b->dxt == 1)
		return;

	uint a;
	if(b->dxt == 3)
	{
		// table of 4-bit alpha entries
		a = access_bit_tbl64(b->a_bits, pixel_idx, 4);
		a |= a << 4; // expand to 8 bits (replicate high into low!)
	}
	else if(b->dxt == 5)
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


struct S3tcDecompressInfo
{
	uint dxt;
	uint s3tc_block_size;
	uint out_Bpp;
	u8* out;
};

static void s3tc_decompress_level(uint UNUSED(level), uint level_w, uint level_h,
	const u8* restrict level_data, size_t level_data_size, void* restrict ctx)
{
	S3tcDecompressInfo* di = (S3tcDecompressInfo*)ctx;
	const uint dxt             = di->dxt;
	const uint s3tc_block_size = di->s3tc_block_size;

	// note: 1x1 images are legitimate (e.g. in mipmaps). they report their
	// width as such for glTexImage, but the S3TC data is padded to
	// 4x4 pixel block boundaries.
	const uint blocks_w = (uint)round_up(level_w, 4) / 4;
	const uint blocks_h = (uint)round_up(level_h, 4) / 4;
	const u8* s3tc_data = level_data;
	debug_assert(level_data_size % s3tc_block_size == 0);

	for(uint block_y = 0; block_y < blocks_h; block_y++)
		for(uint block_x = 0; block_x < blocks_w; block_x++)
		{
			S3tcBlock b;
			s3tc_precalc_block(dxt, s3tc_data, &b);
			s3tc_data += s3tc_block_size;

			uint pixel_idx = 0;
			for(int y = 0; y < 4; y++)
			{
				// this is ugly, but advancing after x, y and block_y loops
				// is no better.
				u8* out = (u8*)di->out + ((block_y*4+y)*blocks_w*4 + block_x*4) * di->out_Bpp;
				for(int x = 0; x < 4; x++)
				{
					s3tc_write_pixel(&b, pixel_idx, out);
					out += di->out_Bpp;
					pixel_idx++;
				}
			}
		}	// for block_x

	debug_assert(s3tc_data == level_data + level_data_size);
	di->out += blocks_w*blocks_h * 16 * di->out_Bpp;
}


// decompress the given image (which is known to be stored as DXTn)
// effectively in-place. updates Tex fields.
static LibError s3tc_decompress(Tex* t)
{
	// alloc new image memory
	// notes:
	// - dxt == 1 is the only non-alpha case.
	// - adding or stripping alpha channels during transform is not
	//   our job; we merely output the same pixel format as given
	//   (tex.cpp's plain transform could cover it, if ever needed).
	const uint dxt = t->flags & TEX_DXT;
	const uint out_bpp = (dxt != 1)? 32 : 24;
	Handle hm;
	const size_t out_size = tex_img_size(t) * out_bpp / t->bpp;
	void* out_data = mem_alloc(out_size, 64*KiB, 0, &hm);
	if(!out_data)
		WARN_RETURN(ERR_NO_MEM);

	const uint s3tc_block_size = (dxt == 3 || dxt == 5)? 16 : 8;
	S3tcDecompressInfo di = { dxt, s3tc_block_size, out_bpp/8, (u8*)out_data };
	const u8* s3tc_data = tex_get_data(t);
	const int levels_to_skip = (t->flags & TEX_MIPMAPS)? 0 : TEX_BASE_LEVEL_ONLY;
	tex_util_foreach_mipmap(t->w, t->h, t->bpp, s3tc_data, levels_to_skip, 4, s3tc_decompress_level, &di);
	(void)mem_free_h(t->hm);
	t->hm  = hm;
	t->ofs = 0;
	t->bpp = out_bpp;
	t->flags &= ~TEX_DXT;
	return ERR_OK;
}


//-----------------------------------------------------------------------------
// DDS file format
//-----------------------------------------------------------------------------

// bit values and structure definitions taken from 
// http://msdn.microsoft.com/archive/en-us/directx9_c/directx/graphics/reference/DDSFileReference/ddsfileformat.asp

#pragma pack(push, 1)

// DDPIXELFORMAT.dwFlags
// we've seen some DXT3 files that don't have this set (which is nonsense;
// any image lacking alpha should be stored as DXT1). it's authoritative
// if fourcc is DXT1 (there's no other way to tell DXT1 and DXT1a apart)
// and ignored otherwise.
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040

typedef struct
{
	u32 dwSize;                       // size of structure (32)
	u32 dwFlags;                      // indicates which fields are valid
	u32 dwFourCC;                     // (DDPF_FOURCC) FOURCC code, "DXTn"
	u32 dwRGBBitCount;                // (DDPF_RGB) bits per pixel
	u32 dwRBitMask;
	u32 dwGBitMask;
	u32 dwBBitMask;
	u32 dwRGBAlphaBitMask;
}
DDPIXELFORMAT;

// DDCAPS2.dwCaps1
#define DDSCAPS_COMPLEX 0x00000008
#define DDSCAPS_TEXTURE 0x00001000
#define DDSCAPS_MIPMAP  0x00400000

// DDCAPS2.dwCaps2
#define DDSCAPS2_CUBEMAP           0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_VOLUME            0x00200000

typedef struct
{
	u32 dwCaps1;
	u32 dwCaps2;
	u32 Reserved[2];
}
DDCAPS2;

// DDSURFACEDESC2.dwFlags
#define DDSD_CAPS        0x00000001
#define DDSD_HEIGHT      0x00000002
#define DDSD_WIDTH       0x00000004
#define DDSD_PITCH       0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE  0x00080000
#define DDSD_DEPTH       0x00800000

typedef struct
{
	u32 dwSize;                    // size of structure (124)
	u32 dwFlags;                   // indicates which fields are valid
	u32 dwHeight;                  // (DDSD_HEIGHT) height of main image (pixels)
	u32 dwWidth;                   // (DDSD_WIDTH ) width  of main image (pixels)
	u32 dwPitchOrLinearSize;       // (DDSD_LINEARSIZE) total image size
	                               // (DDSD_PITCH) bytes per row (%4 = 0)
	u32 dwDepth;                   // (DDSD_DEPTH) vol. textures: vol. depth
	u32 dwMipMapCount;             // (DDSD_MIPMAPCOUNT) total # levels
	u32 dwReserved1[11];           // reserved
	DDPIXELFORMAT ddpfPixelFormat; // (DDSD_PIXELFORMAT) surface description
	DDCAPS2 ddsCaps;               // (DDSD_CAPS) misc. surface flags
	u32 dwReserved2;               // reserved
}
DDSURFACEDESC2;

#pragma pack(pop)


static bool is_valid_dxt(uint dxt)
{
	switch(dxt)
	{
	case 0:
	case 1:
	case DXT1A:
	case 3:
	case 5:
		return true;
	default:
		return false;
	}
	UNREACHABLE;
}


// extract all information from DDS pixel format and store in bpp, flags.
// pf points to the DDS file's header; all fields must be endian-converted
// before use.
// output parameters invalid on failure.
static LibError decode_pf(const DDPIXELFORMAT* pf, uint* bpp_, uint* flags_)
{
	uint bpp = 0;
	uint flags = 0;

	// check struct size
	if(read_le32(&pf->dwSize) != sizeof(DDPIXELFORMAT))
		WARN_RETURN(ERR_TEX_INVALID_SIZE);

	// determine type
	const u32 pf_flags = read_le32(&pf->dwFlags);
	// .. uncompressed
	if(pf_flags & DDPF_RGB)
	{
		const u32 pf_bpp    = read_le32(&pf->dwRGBBitCount);
		const u32 pf_r_mask = read_le32(&pf->dwRBitMask);
		const u32 pf_g_mask = read_le32(&pf->dwGBitMask);
		const u32 pf_b_mask = read_le32(&pf->dwBBitMask);
		const u32 pf_a_mask = read_le32(&pf->dwRGBAlphaBitMask);

		// (checked below; must be set in case below warning is to be
		// skipped)
		bpp = pf_bpp;

		if(pf_flags & DDPF_ALPHAPIXELS)
		{
			// something weird other than RGBA or BGRA
			if(pf_a_mask != 0xFF000000)
				goto unsupported_component_ordering;
			flags |= TEX_ALPHA;
		}

		// make sure component ordering is 0xBBGGRR = RGB (see below)
		if(pf_r_mask != 0xFF || pf_g_mask != 0xFF00 || pf_b_mask != 0xFF0000)
		{
			// DDPIXELFORMAT in theory supports any ordering of R,G,B,A.
			// we need to upload to OpenGL, which can only receive BGR(A) or
			// RGB(A). the former still requires conversion (done by driver),
			// so it's slower. since the very purpose of supporting uncompressed
			// DDS is storing images in a format that requires no processing,
			// we do not allow any weird orderings that require runtime work.
			// instead, the artists must export with the correct settings.
		unsupported_component_ordering:
			WARN_RETURN(ERR_TEX_FMT_INVALID);
		}

		CHECK_ERR(tex_validate_plain_format(bpp, flags));
	}
	// .. compressed
	else if(pf_flags & DDPF_FOURCC)
	{
		// set effective bpp and store DXT format in flags & TEX_DXT.
		// no endian conversion necessary - FOURCC() takes care of that.
		switch(pf->dwFourCC)
		{
		case FOURCC('D','X','T','1'):
			bpp = 4;
			if(pf_flags & DDPF_ALPHAPIXELS)
				flags |= DXT1A | TEX_ALPHA;
			else
				flags |= 1;
			break;
		case FOURCC('D','X','T','3'):
			bpp = 8;
			flags |= 3;
			flags |= TEX_ALPHA;	// see DDPF_ALPHAPIXELS decl
			break;
		case FOURCC('D','X','T','5'):
			bpp = 8;
			flags |= 5;
			flags |= TEX_ALPHA;	// see DDPF_ALPHAPIXELS decl
			break;

		default:
			WARN_RETURN(ERR_TEX_FMT_INVALID);
		}
	}
	// .. neither uncompressed nor compressed - invalid
	else
		WARN_RETURN(ERR_TEX_FMT_INVALID);

	*bpp_ = bpp;
	*flags_ = flags;
	return ERR_OK;
}


// extract all information from DDS header and store in w, h, bpp, flags.
// sd points to the DDS file's header; all fields must be endian-converted
// before use.
// output parameters invalid on failure.
static LibError decode_sd(const DDSURFACEDESC2* sd, uint* w_, uint* h_,
	uint* bpp_, uint* flags_)
{
	// check header size
	if(read_le32(&sd->dwSize) != sizeof(*sd))
		WARN_RETURN(ERR_CORRUPTED);

	// flags (indicate which fields are valid)
	const u32 sd_flags = read_le32(&sd->dwFlags);
	// .. not all required fields are present
	// note: we can't guess dimensions - the image may not be square.
	const u32 sd_req_flags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
	if((sd_flags & sd_req_flags) != sd_req_flags)
		WARN_RETURN(ERR_INCOMPLETE_HEADER);

	// image dimensions
	const u32 h = read_le32(&sd->dwHeight);
	const u32 w = read_le32(&sd->dwWidth);
	// .. not padded to S3TC block size
	if(w % 4 || h % 4)
		WARN_RETURN(ERR_TEX_INVALID_SIZE);

	// pixel format
	uint bpp, flags;
	RETURN_ERR(decode_pf(&sd->ddpfPixelFormat, &bpp, &flags));

	// verify pitch or linear size, if given
	const size_t pitch = w*bpp/8;
	const u32 sd_pitch_or_size = read_le32(&sd->dwPitchOrLinearSize);
	if(sd_flags & DDSD_PITCH)
	{
		if(sd_pitch_or_size != round_up(pitch, 4))
			WARN_RETURN(ERR_CORRUPTED);
	}
	if(sd_flags & DDSD_LINEARSIZE)
	{
		if(sd_pitch_or_size != pitch*h)
			WARN_RETURN(ERR_CORRUPTED);
	}
	// note: both flags set would be invalid; no need to check for that,
	// though, since one of the above tests would fail.

	// mipmaps
	if(sd_flags & DDSD_MIPMAPCOUNT)
	{
		const u32 mipmap_count = read_le32(&sd->dwMipMapCount);
		if(mipmap_count)
		{
			// mipmap chain is incomplete
			// note: DDS includes the base level in its count, hence +1.
			if(mipmap_count != log2(MAX(w,h))+1)
				WARN_RETURN(ERR_TEX_FMT_INVALID);
			flags |= TEX_MIPMAPS;
		}
	}

	// check for volume textures
	if(sd_flags & DDSD_DEPTH)
	{
		const u32 depth = read_le32(&sd->dwDepth);
		if(depth)
			WARN_RETURN(ERR_NOT_IMPLEMENTED);
	}

	// check caps
	const DDCAPS2* caps = &sd->ddsCaps;
	// .. this is supposed to be set, but don't bail if not (pointless)
	debug_assert(caps->dwCaps1 & DDSCAPS_TEXTURE);
	// .. sanity check: warn if mipmap flag not set (don't bail if not
	// because we've already made the decision).
	const bool mipmap_cap = (caps->dwCaps1 & DDSCAPS_MIPMAP) != 0;
	const bool mipmap_flag = (flags & TEX_MIPMAPS) != 0;
	debug_assert(mipmap_cap == mipmap_flag);
	// note: we do not check for cubemaps and volume textures (not supported)
	// because the file may still have useful data we can read.

	*w_ = w;
	*h_ = h;
	*bpp_ = bpp;
	*flags_ = flags;
	return ERR_OK;
}


//-----------------------------------------------------------------------------

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


static LibError dds_decode(DynArray* restrict da, Tex* restrict t)
{
	u8* file         = da->base;
	const DDSURFACEDESC2* sd = (const DDSURFACEDESC2*)(file+4);

	uint w, h;
	uint bpp, flags;
	RETURN_ERR(decode_sd(sd, &w, &h, &bpp, &flags));
	// note: cannot pass address of these directly to decode_sd because
	// they are bitfields.
	t->w     = w;
	t->h     = h;
	t->bpp   = bpp;
	t->flags = flags;
	return ERR_OK;
}


static LibError dds_encode(Tex* restrict UNUSED(t), DynArray* restrict UNUSED(da))
{
	// note: do not return ERR_NOT_IMPLEMENTED et al. because that would
	// break tex_write (which assumes either this, 0 or errors are returned).
	return ERR_TEX_CODEC_CANNOT_HANDLE;
}


static LibError dds_transform(Tex* t, uint transforms)
{
	uint dxt = t->flags & TEX_DXT;
	debug_assert(is_valid_dxt(dxt));

	const uint transform_dxt = transforms & TEX_DXT;
	// requesting decompression
	if(dxt && transform_dxt)
	{
		RETURN_ERR(s3tc_decompress(t));
		return ERR_OK;
	}
	// both are DXT (unsupported; there are no flags we can change while
	// compressed) or requesting compression (not implemented) or
	// both not DXT (nothing we can do) - bail.
	else
		return ERR_TEX_CODEC_CANNOT_HANDLE;
}


TEX_CODEC_REGISTER(dds);
