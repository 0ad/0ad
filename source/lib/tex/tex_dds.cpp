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
 * DDS (DirectDraw Surface) codec.
 */

#include "precompiled.h"

#include "lib/byte_order.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/allocators/shared_ptr.h"
#include "tex_codec.h"


// NOTE: the convention is bottom-up for DDS, but there's no way to tell.


//-----------------------------------------------------------------------------
// S3TC decompression
//-----------------------------------------------------------------------------

// note: this code may not be terribly efficient. it's only used to
// emulate hardware S3TC support - if that isn't available, performance
// will suffer anyway due to increased video memory usage.


// for efficiency, we precalculate as much as possible about a block
// and store it here.
class S3tcBlock
{
public:
	S3tcBlock(size_t dxt, const u8* RESTRICT block)
		: dxt(dxt)
	{
		// (careful, 'dxt != 1' doesn't work - there's also DXT1a)
		const u8* a_block = block;
		const u8* c_block = (dxt == 3 || dxt == 5)? block+8 : block;

		PrecalculateAlpha(dxt, a_block);
		PrecalculateColor(dxt, c_block);
	}

	void WritePixel(size_t pixel_idx, u8* RESTRICT out) const
	{
		ENSURE(pixel_idx < 16);

		// pixel index -> color selector (2 bit) -> color
		const size_t c_selector = access_bit_tbl(c_selectors, pixel_idx, 2);
		for(int i = 0; i < 3; i++)
			out[i] = (u8)c[c_selector][i];

		// if no alpha, done
		if(dxt == 1)
			return;

		size_t a;
		if(dxt == 3)
		{
			// table of 4-bit alpha entries
			a = access_bit_tbl(a_bits, pixel_idx, 4);
			a |= a << 4; // expand to 8 bits (replicate high into low!)
		}
		else if(dxt == 5)
		{
			// pixel index -> alpha selector (3 bit) -> alpha
			const size_t a_selector = access_bit_tbl(a_bits, pixel_idx, 3);
			a = dxt5_a_tbl[a_selector];
		}
		// (dxt == DXT1A)
		else
			a = c[c_selector][A];
		out[A] = (u8)(a & 0xFF);
	}

private:
	// pixel colors are stored as size_t[4]. size_t rather than u8 protects from
	// overflow during calculations, and padding to an even size is a bit
	// more efficient (even though we don't need the alpha component).
	enum RGBA {	R, G, B, A };

	static inline void mix_2_3(size_t dst[4], size_t c0[4], size_t c1[4])
	{
		for(int i = 0; i < 3; i++) dst[i] = (c0[i]*2 + c1[i] + 1)/3;
	}

	static inline void mix_avg(size_t dst[4], size_t c0[4], size_t c1[4])
	{
		for(int i = 0; i < 3; i++) dst[i] = (c0[i]+c1[i])/2;
	}

	template<typename T>
	static inline size_t access_bit_tbl(T tbl, size_t idx, size_t bit_width)
	{
		size_t val = (tbl >> (idx*bit_width)) & bit_mask<T>(bit_width);
		return val;
	}

	// extract a range of bits and expand to 8 bits (by replicating
	// MS bits - see http://www.mindcontrol.org/~hplus/graphics/expand-bits.html ;
	// this is also the algorithm used by graphics cards when decompressing S3TC).
	// used to convert 565 to 32bpp RGB.
	static inline size_t unpack_to_8(u16 c, size_t bits_below, size_t num_bits)
	{
		const size_t num_filler_bits = 8-num_bits;
		const size_t field = (size_t)bits(c, bits_below, bits_below+num_bits-1);
		const size_t filler = field >> (num_bits-num_filler_bits);
		return (field << num_filler_bits) | filler;
	}

	void PrecalculateAlpha(size_t dxt, const u8* RESTRICT a_block)
	{
		// read block contents
		const u8 a0 = a_block[0], a1 = a_block[1];
		a_bits = read_le64(a_block);	// see below

		if(dxt == 5)
		{
			// skip a0,a1 bytes (data is little endian)
			a_bits >>= 16;

			const bool is_dxt5_special_combination = (a0 <= a1);
			u8* a = dxt5_a_tbl;	// shorthand
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


	void PrecalculateColor(size_t dxt, const u8* RESTRICT c_block)
	{
		// read block contents
		// .. S3TC reference colors (565 format). the color table is generated
		//    from some combination of these, depending on their ordering.
		u16 rc[2];
		for(int i = 0; i < 2; i++)
			rc[i] = read_le16(c_block + 2*i);
		// .. table of 2-bit color selectors
		c_selectors = read_le32(c_block+4);

		const bool is_dxt1_special_combination = (dxt == 1 || dxt == DXT1A) && rc[0] <= rc[1];

		// c0 and c1 are the values of rc[], converted to 32bpp
		for(int i = 0; i < 2; i++)
		{
			c[i][R] = unpack_to_8(rc[i], 11, 5);
			c[i][G] = unpack_to_8(rc[i],  5, 6);
			c[i][B] = unpack_to_8(rc[i],  0, 5);
		}

		// c2 and c3 are combinations of c0 and c1:
		if(is_dxt1_special_combination)
		{
			mix_avg(c[2], c[0], c[1]);			// c2 = (c0+c1)/2
			for(int i = 0; i < 3; i++) c[3][i] = 0;	// c3 = black
			c[3][A] = (dxt == DXT1A)? 0 : 255;		// (transparent iff DXT1a)
		}
		else
		{
			mix_2_3(c[2], c[0], c[1]);			// c2 = 2/3*c0 + 1/3*c1
			mix_2_3(c[3], c[1], c[0]);			// c3 = 1/3*c0 + 2/3*c1
		}
	}

	// the 4 color choices for each pixel (RGBA)
	size_t c[4][4];	// c[i][RGBA_component]

	// (DXT5 only) the 8 alpha choices
	u8 dxt5_a_tbl[8];

	// alpha block; interpretation depends on dxt.
	u64 a_bits;

	// table of 2-bit color selectors
	u32 c_selectors;

	size_t dxt;
};


struct S3tcDecompressInfo
{
	size_t dxt;
	size_t s3tc_block_size;
	size_t out_Bpp;
	u8* out;
};

static void s3tc_decompress_level(size_t UNUSED(level), size_t level_w, size_t level_h,
	const u8* RESTRICT level_data, size_t level_data_size, void* RESTRICT cbData)
{
	S3tcDecompressInfo* di = (S3tcDecompressInfo*)cbData;
	const size_t dxt             = di->dxt;
	const size_t s3tc_block_size = di->s3tc_block_size;

	// note: 1x1 images are legitimate (e.g. in mipmaps). they report their
	// width as such for glTexImage, but the S3TC data is padded to
	// 4x4 pixel block boundaries.
	const size_t blocks_w = DivideRoundUp(level_w, size_t(4));
	const size_t blocks_h = DivideRoundUp(level_h, size_t(4));
	const u8* s3tc_data = level_data;
	ENSURE(level_data_size % s3tc_block_size == 0);

	for(size_t block_y = 0; block_y < blocks_h; block_y++)
	{
		for(size_t block_x = 0; block_x < blocks_w; block_x++)
		{
			S3tcBlock block(dxt, s3tc_data);
			s3tc_data += s3tc_block_size;

			size_t pixel_idx = 0;
			for(int y = 0; y < 4; y++)
			{
				// this is ugly, but advancing after x, y and block_y loops
				// is no better.
				u8* out = (u8*)di->out + ((block_y*4+y)*blocks_w*4 + block_x*4) * di->out_Bpp;
				for(int x = 0; x < 4; x++)
				{
					block.WritePixel(pixel_idx, out);
					out += di->out_Bpp;
					pixel_idx++;
				}
			}
		}
	}

	ENSURE(s3tc_data == level_data + level_data_size);
	di->out += blocks_w*blocks_h * 16 * di->out_Bpp;
}


// decompress the given image (which is known to be stored as DXTn)
// effectively in-place. updates Tex fields.
static Status s3tc_decompress(Tex* t)
{
	// alloc new image memory
	// notes:
	// - dxt == 1 is the only non-alpha case.
	// - adding or stripping alpha channels during transform is not
	//   our job; we merely output the same pixel format as given
	//   (tex.cpp's plain transform could cover it, if ever needed).
	const size_t dxt = t->m_Flags & TEX_DXT;
	const size_t out_bpp = (dxt != 1)? 32 : 24;
	const size_t out_size = t->img_size() * out_bpp / t->m_Bpp;
	shared_ptr<u8> decompressedData;
	AllocateAligned(decompressedData, out_size, pageSize);

	const size_t s3tc_block_size = (dxt == 3 || dxt == 5)? 16 : 8;
	S3tcDecompressInfo di = { dxt, s3tc_block_size, out_bpp/8, decompressedData.get() };
	const u8* s3tc_data = t->get_data();
	const int levels_to_skip = (t->m_Flags & TEX_MIPMAPS)? 0 : TEX_BASE_LEVEL_ONLY;
	tex_util_foreach_mipmap(t->m_Width, t->m_Height, t->m_Bpp, s3tc_data, levels_to_skip, 4, s3tc_decompress_level, &di);
	t->m_Data = decompressedData;
	t->m_DataSize = out_size;
	t->m_Ofs = 0;
	t->m_Bpp = out_bpp;
	t->m_Flags &= ~TEX_DXT;
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// DDS file format
//-----------------------------------------------------------------------------

// bit values and structure definitions taken from 
// http://msdn.microsoft.com/en-us/library/ee417785(VS.85).aspx

#pragma pack(push, 1)

// DDS_PIXELFORMAT.dwFlags
// we've seen some DXT3 files that don't have this set (which is nonsense;
// any image lacking alpha should be stored as DXT1). it's authoritative
// if fourcc is DXT1 (there's no other way to tell DXT1 and DXT1a apart)
// and ignored otherwise.
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040

struct DDS_PIXELFORMAT
{
	u32 dwSize;                       // size of structure (32)
	u32 dwFlags;                      // indicates which fields are valid
	u32 dwFourCC;                     // (DDPF_FOURCC) FOURCC code, "DXTn"
	u32 dwRGBBitCount;                // (DDPF_RGB) bits per pixel
	u32 dwRBitMask;
	u32 dwGBitMask;
	u32 dwBBitMask;
	u32 dwABitMask;                   // (DDPF_ALPHAPIXELS)
};


// DDS_HEADER.dwFlags (none are optional)
#define DDSD_CAPS        0x00000001
#define DDSD_HEIGHT      0x00000002
#define DDSD_WIDTH       0x00000004
#define DDSD_PITCH       0x00000008 // used when texture is uncompressed
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE  0x00080000 // used when texture is compressed
#define DDSD_DEPTH       0x00800000

// DDS_HEADER.dwCaps
#define DDSCAPS_MIPMAP   0x00400000 // optional
#define DDSCAPS_TEXTURE	 0x00001000 // required

struct DDS_HEADER
{
	// (preceded by the FOURCC "DDS ")
	u32 dwSize;                    // size of structure (124)
	u32 dwFlags;                   // indicates which fields are valid
	u32 dwHeight;                  // (DDSD_HEIGHT) height of main image (pixels)
	u32 dwWidth;                   // (DDSD_WIDTH ) width  of main image (pixels)
	u32 dwPitchOrLinearSize;       // (DDSD_LINEARSIZE) size [bytes] of top level
	                               // (DDSD_PITCH) bytes per row (%4 = 0)
	u32 dwDepth;                   // (DDSD_DEPTH) vol. textures: vol. depth
	u32 dwMipMapCount;             // (DDSD_MIPMAPCOUNT) total # levels
	u32 dwReserved1[11];           // reserved
	DDS_PIXELFORMAT ddpf;          // (DDSD_PIXELFORMAT) surface description
	u32 dwCaps;                    // (DDSD_CAPS) misc. surface flags
	u32 dwCaps2;
	u32 dwCaps3;
	u32 dwCaps4;
	u32 dwReserved2;               // reserved
};

#pragma pack(pop)


static bool is_valid_dxt(size_t dxt)
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
}


// extract all information from DDS pixel format and store in bpp, flags.
// pf points to the DDS file's header; all fields must be endian-converted
// before use.
// output parameters invalid on failure.
static Status decode_pf(const DDS_PIXELFORMAT* pf, size_t& bpp, size_t& flags)
{
	bpp = 0;
	flags = 0;

	// check struct size
	if(read_le32(&pf->dwSize) != sizeof(DDS_PIXELFORMAT))
		WARN_RETURN(ERR::TEX_INVALID_SIZE);

	// determine type
	const size_t pf_flags = (size_t)read_le32(&pf->dwFlags);
	// .. uncompressed RGB/RGBA
	if(pf_flags & DDPF_RGB)
	{
		const size_t pf_bpp    = (size_t)read_le32(&pf->dwRGBBitCount);
		const size_t pf_r_mask = (size_t)read_le32(&pf->dwRBitMask);
		const size_t pf_g_mask = (size_t)read_le32(&pf->dwGBitMask);
		const size_t pf_b_mask = (size_t)read_le32(&pf->dwBBitMask);
		const size_t pf_a_mask = (size_t)read_le32(&pf->dwABitMask);

		// (checked below; must be set in case below warning is to be
		// skipped)
		bpp = pf_bpp;

		if(pf_flags & DDPF_ALPHAPIXELS)
		{
			// something weird other than RGBA or BGRA
			if(pf_a_mask != 0xFF000000)
				WARN_RETURN(ERR::TEX_FMT_INVALID);
			flags |= TEX_ALPHA;
		}

		// make sure component ordering is 0xBBGGRR = RGB (see below)
		if(pf_r_mask != 0xFF || pf_g_mask != 0xFF00 || pf_b_mask != 0xFF0000)
		{
			// DDS_PIXELFORMAT in theory supports any ordering of R,G,B,A.
			// we need to upload to OpenGL, which can only receive BGR(A) or
			// RGB(A). the former still requires conversion (done by driver),
			// so it's slower. since the very purpose of supporting uncompressed
			// DDS is storing images in a format that requires no processing,
			// we do not allow any weird orderings that require runtime work.
			// instead, the artists must export with the correct settings.
			WARN_RETURN(ERR::TEX_FMT_INVALID);
		}

		RETURN_STATUS_IF_ERR(tex_validate_plain_format(bpp, (int)flags));
	}
	// .. uncompressed 8bpp greyscale
	else if(pf_flags & DDPF_ALPHAPIXELS)
	{
		const size_t pf_bpp    = (size_t)read_le32(&pf->dwRGBBitCount);
		const size_t pf_a_mask = (size_t)read_le32(&pf->dwABitMask);

		bpp = pf_bpp;

		if(pf_bpp != 8)
			WARN_RETURN(ERR::TEX_FMT_INVALID);

		if(pf_a_mask != 0xFF)
			WARN_RETURN(ERR::TEX_FMT_INVALID);
		flags |= TEX_GREY;

		RETURN_STATUS_IF_ERR(tex_validate_plain_format(bpp, (int)flags));
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
			WARN_RETURN(ERR::TEX_FMT_INVALID);
		}
	}
	// .. neither uncompressed nor compressed - invalid
	else
		WARN_RETURN(ERR::TEX_FMT_INVALID);

	return INFO::OK;
}


// extract all information from DDS header and store in w, h, bpp, flags.
// sd points to the DDS file's header; all fields must be endian-converted
// before use.
// output parameters invalid on failure.
static Status decode_sd(const DDS_HEADER* sd, size_t& w, size_t& h, size_t& bpp, size_t& flags)
{
	// check header size
	if(read_le32(&sd->dwSize) != sizeof(*sd))
		WARN_RETURN(ERR::CORRUPTED);

	// flags (indicate which fields are valid)
	const size_t sd_flags = (size_t)read_le32(&sd->dwFlags);
	// .. not all required fields are present
	// note: we can't guess dimensions - the image may not be square.
	const size_t sd_req_flags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
	if((sd_flags & sd_req_flags) != sd_req_flags)
		WARN_RETURN(ERR::TEX_INCOMPLETE_HEADER);

	// image dimensions
	h = (size_t)read_le32(&sd->dwHeight);
	w = (size_t)read_le32(&sd->dwWidth);

	// pixel format
	RETURN_STATUS_IF_ERR(decode_pf(&sd->ddpf, bpp, flags));

	// if the image is not aligned with the S3TC block size, it is stored
	// with extra pixels on the bottom left to fill up the space, so we need
	// to account for those when calculating how big it should be
	size_t stored_h, stored_w;
	if(flags & TEX_DXT)
	{
		stored_h = Align<4>(h);
		stored_w = Align<4>(w);
	}
	else
	{
		stored_h = h;
		stored_w = w;
	}

	// verify pitch or linear size, if given
	const size_t pitch = stored_w*bpp/8;
	const size_t sd_pitch_or_size = (size_t)read_le32(&sd->dwPitchOrLinearSize);
	if(sd_flags & DDSD_PITCH)
	{
		if(sd_pitch_or_size != Align<4>(pitch))
			DEBUG_WARN_ERR(ERR::CORRUPTED);
	}
	if(sd_flags & DDSD_LINEARSIZE)
	{
		// some DDS tools mistakenly store the total size of all levels,
		// so allow values close to that as well
		const ssize_t totalSize = ssize_t(pitch*stored_h*1.333333f);
		if(sd_pitch_or_size != pitch*stored_h && abs(ssize_t(sd_pitch_or_size)-totalSize) > 64)
			DEBUG_WARN_ERR(ERR::CORRUPTED);
	}
	// note: both flags set would be invalid; no need to check for that,
	// though, since one of the above tests would fail.

	// mipmaps
	if(sd_flags & DDSD_MIPMAPCOUNT)
	{
		const size_t mipmap_count = (size_t)read_le32(&sd->dwMipMapCount);
		if(mipmap_count)
		{
			// mipmap chain is incomplete
			// note: DDS includes the base level in its count, hence +1.
			if(mipmap_count != ceil_log2(std::max(w,h))+1)
				WARN_RETURN(ERR::TEX_FMT_INVALID);
			flags |= TEX_MIPMAPS;
		}
	}

	// check for volume textures
	if(sd_flags & DDSD_DEPTH)
	{
		const size_t depth = (size_t)read_le32(&sd->dwDepth);
		if(depth)
			WARN_RETURN(ERR::NOT_SUPPORTED);
	}

	// check caps
	// .. this is supposed to be set, but don't bail if not (pointless)
	ENSURE(sd->dwCaps & DDSCAPS_TEXTURE);
	// .. sanity check: warn if mipmap flag not set (don't bail if not
	// because we've already made the decision).
	const bool mipmap_cap = (sd->dwCaps & DDSCAPS_MIPMAP) != 0;
	const bool mipmap_flag = (flags & TEX_MIPMAPS) != 0;
	ENSURE(mipmap_cap == mipmap_flag);
	// note: we do not check for cubemaps and volume textures (not supported)
	// because the file may still have useful data we can read.

	return INFO::OK;
}


//-----------------------------------------------------------------------------

bool TexCodecDds::is_hdr(const u8* file) const
{
	return *(u32*)file == FOURCC('D','D','S',' ');
}


bool TexCodecDds::is_ext(const OsPath& extension) const
{
	return extension == L".dds";
}


size_t TexCodecDds::hdr_size(const u8* UNUSED(file)) const
{
	return 4+sizeof(DDS_HEADER);
}


Status TexCodecDds::decode(rpU8 data, size_t UNUSED(size), Tex* RESTRICT t) const
{
	const DDS_HEADER* sd = (const DDS_HEADER*)(data+4);
	RETURN_STATUS_IF_ERR(decode_sd(sd, t->m_Width, t->m_Height, t->m_Bpp, t->m_Flags));
	return INFO::OK;
}


Status TexCodecDds::encode(Tex* RESTRICT UNUSED(t), DynArray* RESTRICT UNUSED(da)) const
{
	// note: do not return ERR::NOT_SUPPORTED et al. because that would
	// break tex_write (which assumes either this, 0 or errors are returned).
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}


TIMER_ADD_CLIENT(tc_dds_transform);

Status TexCodecDds::transform(Tex* t, size_t transforms) const
{
	TIMER_ACCRUE(tc_dds_transform);

	size_t mipmaps = t->m_Flags & TEX_MIPMAPS;
	size_t dxt = t->m_Flags & TEX_DXT;
	ENSURE(is_valid_dxt(dxt));

	const size_t transform_mipmaps = transforms & TEX_MIPMAPS;
	const size_t transform_dxt = transforms & TEX_DXT;
	// requesting removal of mipmaps
	if(mipmaps && transform_mipmaps)
	{
		// we don't need to actually change anything except the flag - the
		// mipmap levels will just be treated as trailing junk
		t->m_Flags &= ~TEX_MIPMAPS;
		return INFO::OK;
	}
	// requesting decompression
	if(dxt && transform_dxt)
	{
		RETURN_STATUS_IF_ERR(s3tc_decompress(t));
		return INFO::OK;
	}
	// both are DXT (unsupported; there are no flags we can change while
	// compressed) or requesting compression (not implemented) or
	// both not DXT (nothing we can do) - bail.
	return INFO::TEX_CODEC_CANNOT_HANDLE;
}
