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


static int dds_decompress(Tex* t)
{
	uint w = t->w, h = t->h;
	if(w==0 || h==0 || w%4 || h%4)
		return ERR_TEX_FMT_INVALID;

	int dxt = t->flags & TEX_DXT;
	debug_assert(dxt == 1 || dxt == 3 || dxt == 5);

	u8* data = tex_get_data(t);
/*
	GLsizei blocks_w = (GLsizei)(round_up(w, 4) / 4);
	GLsizei blocks_h = (GLsizei)(round_up(h, 4) / 4);
	GLsizei blocks = blocks_w * blocks_h;
	GLsizei size = blocks * 16 * (base_fmt == GL_RGB ? 3 : 4);
	GLsizei pitch = size / (blocks_h*4);
	UNUSED2(pitch);
	void* rgb_data = malloc(size);

	debug_assert(imageSize == blocks * (dxt1? 8 : 16));

	// This code is inefficient, but I don't care:

	for(GLsizei block_y = 0; block_y < blocks_h; ++block_y)
		for(GLsizei block_x = 0; block_x < blocks_w; ++block_x)
		{
			int c0_a = 255, c1_a = 255, c2_a = 255, c3_a = 255;
			u8* alpha = NULL;
			u8 dxt5alpha[8];

			if(!dxt1)
			{
				alpha = (u8*)data;
				data = (char*)data + 8;

				if(dxt5)
				{
					dxt5alpha[0] = alpha[0];
					dxt5alpha[1] = alpha[1];
					if(alpha[0] > alpha[1])
					{
						dxt5alpha[2] = (6*alpha[0] + 1*alpha[1] + 3)/7;
						dxt5alpha[3] = (5*alpha[0] + 2*alpha[1] + 3)/7;
						dxt5alpha[4] = (4*alpha[0] + 3*alpha[1] + 3)/7;
						dxt5alpha[5] = (3*alpha[0] + 4*alpha[1] + 3)/7;
						dxt5alpha[6] = (2*alpha[0] + 5*alpha[1] + 3)/7;
						dxt5alpha[7] = (1*alpha[0] + 6*alpha[1] + 3)/7;
					}
					else
					{
						dxt5alpha[2] = (4*alpha[0] + 1*alpha[1] + 2)/5;
						dxt5alpha[3] = (3*alpha[0] + 2*alpha[1] + 2)/5;
						dxt5alpha[4] = (2*alpha[0] + 3*alpha[1] + 2)/5;
						dxt5alpha[5] = (1*alpha[0] + 4*alpha[1] + 2)/5;
						dxt5alpha[6] = 0;
						dxt5alpha[7] = 255;
					}
				}
			}

			u16 c0   = *(u16*)( (char*)data + 0 );
			u16 c1   = *(u16*)( (char*)data + 2 );
			u32 bits = *(u32*)( (char*)data + 4 );

			data = (char*)data + 8;

			// Unpack 565, and copy high bits to low bits
			int c0_r = ((c0>>8)&0xF8) | ((c0>>13)&7);
			int c1_r = ((c1>>8)&0xF8) | ((c1>>13)&7);
			int c0_g = ((c0>>3)&0xFC) | ((c0>>9 )&3);
			int c1_g = ((c1>>3)&0xFC) | ((c1>>9 )&3);
			int c0_b = ((c0<<3)&0xF8) | ((c0>>2 )&7);
			int c1_b = ((c1<<3)&0xF8) | ((c1>>2 )&7);
			int c2_r, c2_g, c2_b;
			int c3_r, c3_g, c3_b;
			if(!dxt1 || c0 > c1)
			{
				c2_r = (c0_r*2+c1_r+1)/3; c2_g = (c0_g*2+c1_g+1)/3; c2_b = (c0_b*2+c1_b+1)/3;
				c3_r = (c0_r+2*c1_r+1)/3; c3_g = (c0_g+2*c1_g+1)/3; c3_b = (c0_b+2*c1_b+1)/3;
			}
			else
			{
				c2_r = (c0_r+c1_r)/2; c2_g = (c0_g+c1_g)/2; c2_b = (c0_b+c1_b)/2;
				c3_r = c3_g = c3_b = c3_a = 0;
			}

			if(base_fmt == GL_RGB)
			{
				int i = 0;
				for(int y = 0; y < 4; ++y)
				{
					u8* out = (u8*)rgb_data + ((block_y*4+y)*blocks_w*4 + block_x*4) * 3;
					for(int x = 0; x < 4; ++x, ++i)
					{
						switch((bits >> (2*i)) & 3) {
						case 0: *out++ = c0_r; *out++ = c0_g; *out++ = c0_b; break;
						case 1: *out++ = c1_r; *out++ = c1_g; *out++ = c1_b; break;
						case 2: *out++ = c2_r; *out++ = c2_g; *out++ = c2_b; break;
						case 3: *out++ = c3_r; *out++ = c3_g; *out++ = c3_b; break;
						}
					}
				}

			}
			else
			{
				int i = 0;
				for(int y = 0; y < 4; ++y)
				{
					u8* out = (u8*)rgb_data + ((block_y*4+y)*blocks_w*4 + block_x*4) * 4;
					for(int x = 0; x < 4; ++x, ++i)
					{
						int a = 0;	// squelch bogus uninitialized warning
						switch((bits >> (2*i)) & 3) {
						case 0: *out++ = c0_r; *out++ = c0_g; *out++ = c0_b; a = c0_a; break;
						case 1: *out++ = c1_r; *out++ = c1_g; *out++ = c1_b; a = c1_a; break;
						case 2: *out++ = c2_r; *out++ = c2_g; *out++ = c2_b; a = c2_a; break;
						case 3: *out++ = c3_r; *out++ = c3_g; *out++ = c3_b; a = c3_a; break;
						}
						if(dxt3)
						{
							a = (int)((*(u64*)alpha >> (4*i)) & 0xF);
							a |= a<<4; // copy low bits to high bits
						}
						else if(dxt5)
						{
							a = dxt5alpha[(*(u64*)(alpha+2) >> (3*i)) & 0x7];
						}
						*out++ = a;
					}
				}
			}
		}
	*/

	return 0;
}

static int dds_transform(Tex* t, int new_flags)
{
	const int dxt = t->flags & TEX_DXT, new_dxt = new_flags & TEX_DXT;
	// requesting decompression
	if(dxt && !new_dxt)
		return dds_decompress(t);
	// both are DXT (unsupported; there are no flags we can change while
	// compressed) or requesting compression (not implemented) or
	// both not DXT (nothing we can do) - bail.
	else
		return TEX_CODEC_CANNOT_HANDLE;
}


static int dds_decode(u8* file, size_t file_size, Tex* t, const char** perr_msg)
{
	if(*(u32*)file != FOURCC('D','D','S',' '))
		return TEX_CODEC_CANNOT_HANDLE;

	const DDSURFACEDESC2* surf = (const DDSURFACEDESC2*)(file+4);
	const size_t hdr_size = 4+sizeof(DDSURFACEDESC2);

	// make sure we can access all header fields
	if(file_size < hdr_size)
	{
		*perr_msg = "header not completely read";
fail:
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
	const char* err = 0;
	if(file_size < hdr_size + img_size)
		err = "file size too small";
	if(w % 4 || h % 4)
		err = "image dimensions not padded to S3TC block size";
	if(!w || !h)
		err = "width or height = 0";
	if(bpp == 0)
		err = "invalid pixel format (not DXT{1,3,5})";
	if((sd_flags & sd_req_flags) != sd_req_flags)
		err = "missing one or more required fields (w, h, pixel format)";
	if(sizeof(DDPIXELFORMAT) != pf_size)
		err = "DDPIXELFORMAT size mismatch";
	if(sizeof(DDSURFACEDESC2) != sd_size)
		err = "DDSURFACEDESC2 size mismatch";
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
	return 0;
}


static int dds_encode(const char* UNUSED(ext), Tex* UNUSED(t), u8** UNUSED(out), size_t* UNUSED(out_size), const char** UNUSED(perr_msg))
{
	return ERR_NOT_IMPLEMENTED;
}

TEX_CODEC_REGISTER(dds);
