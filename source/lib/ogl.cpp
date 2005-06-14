#include "precompiled.h"

#include "lib.h"
#include "sdl.h"
#include "ogl.h"
#include "detect.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
	// cannot get rid of glu32 - seems to be loaded by opengl32,
	// even though dependency walker marks it as demand-loaded.
#endif


// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (CALL_CONV *name) params;
#define FUNC2(ret, nameARB, nameCore, version, params) ret (CALL_CONV *nameARB) params;
#include "glext_funcs.h"
#undef FUNC2
#undef FUNC
}


static const char* exts = NULL;


// check if the extension <ext> is supported by the OpenGL implementation
bool oglHaveExtension(const char* ext)
{
	assert(exts && "call oglInit before using this function");

	const char *p = exts, *end;

	// make sure ext is valid & doesn't contain spaces
	if(!ext || ext == '\0' || strchr(ext, ' '))
		return false;

	for(;;)
	{
		p = strstr(p, ext);
		if(!p)
			return false; // <ext> string not found - extension not supported
		end = p + strlen(ext); // end of current substring

		// make sure the substring found is an entire extension string,
		// i.e. it starts and ends with ' '
		if(p == exts || *(p-1) == ' ')		// valid start?
			if(*end == ' ' || *end == '\0') // valid end?
				return true;
		p = end;
	}
}


// check if the OpenGL implementation is at least at <version>.
// (format: "%d.%d" major minor)
bool oglHaveVersion(const char* desired_version)
{
	int desired_major, desired_minor;
	if(sscanf(desired_version, "%d.%d", &desired_major, &desired_minor) != 2)
	{
		debug_warn("oglHaveVersion: invalid version string");
		return false;
	}

	int major, minor;
	const char* version = (const char*)glGetString(GL_VERSION);
	if(!version || sscanf(version, "%d.%d", &major, &minor) != 2)
	{
		debug_warn("oglHaveVersion: GL_VERSION invalid");
		return false;
	}

	return (major > desired_major
			|| (major == desired_major && minor >= desired_minor));
}


#ifdef OGL_CHECKS
void oglCheck()
{
	unsigned int err = glGetError();
	if (err != GL_NO_ERROR)
	{
		debug_printf("GL errors!\n");

		#define E(e) case e: debug_printf("%s\n", #e); break;
		switch (err)
		{
			E(GL_INVALID_ENUM)
			E(GL_INVALID_VALUE)
			E(GL_INVALID_OPERATION)
			E(GL_STACK_OVERFLOW)
			E(GL_STACK_UNDERFLOW)
			E(GL_OUT_OF_MEMORY)
		default:;
		}
		#undef E
		debug_break();
	}
}
#endif // #ifdef OGL_CHECKS

void oglPrintErrors()
{
#define E(e) case e: debug_printf("%s\n", #e); break;

	for(;;)
		switch(glGetError())
		{
			E(GL_INVALID_ENUM)
			E(GL_INVALID_VALUE)
			E(GL_INVALID_OPERATION)
			E(GL_STACK_OVERFLOW)
			E(GL_STACK_UNDERFLOW)
			E(GL_OUT_OF_MEMORY)

		default:
			return;
		}
}


int max_tex_size;				// [pixels]
int tex_units;
int max_VAR_elements = -1;		// GF2: 64K; GF3: 1M
bool tex_compression_avail;		// S3TC / DXT{1,3,5}
int video_mem;					// [MiB]; approximate


// gfx_card and gfx_drv_ver are unchanged on failure.
int ogl_get_gfx_info()
{
	const char* vendor   = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version  = (const char*)glGetString(GL_VERSION);

	// can fail if OpenGL not yet initialized,
	// or if called between glBegin and glEnd.
	if(!vendor || !renderer || !version)
		return -1;

	strcpy_s(gfx_card, sizeof(gfx_card), vendor);

	// reduce string to "ATI" or "NVIDIA"
	if(!strcmp(gfx_card, "ATI Technologies Inc."))
		gfx_card[3] = 0;
	if(!strcmp(gfx_card, "NVIDIA Corporation"))
		gfx_card[6] = 0;

	strcat_s(gfx_card, sizeof(gfx_card), renderer);
		// don't bother cutting off the crap at the end.
		// too risky, and too many different strings.

	snprintf(gfx_drv_ver, sizeof(gfx_drv_ver), "OpenGL %s", version);
		// add "OpenGL" to differentiate this from the real driver version
		// (returned by platform-specific detect routines).

	return 0;
}


const char* oglExtList()
{
	assert(exts && "call oglInit before using this function");
	return exts;
}


void CALL_CONV oglEmulateCompressedTexImage2D (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);

// call after each video mode change
void oglInit()
{
	exts = (const char*)glGetString(GL_EXTENSIONS);
	if(!exts)
	{
		debug_warn("oglInit called before OpenGL is ready for use");
	}

	// import functions
#define FUNC(ret, name, params) *(void**)&name = SDL_GL_GetProcAddress(#name);
#define FUNC2(ret, nameARB, nameCore, version, params) \
	nameARB = NULL; \
	if(oglHaveVersion(version)) \
		*(void**)&nameARB = SDL_GL_GetProcAddress(#nameCore); \
	if(!nameARB) /* use the ARB name if the driver lied about what version it supports */ \
		*(void**)&nameARB = SDL_GL_GetProcAddress(#nameARB);
#include "glext_funcs.h"
#undef FUNC2
#undef FUNC
		// It should be safe to load the ARB function pointers even if the
		// extension isn't advertised, since we won't actually use them without
		// checking for the extension.

	// detect OpenGL / graphics card caps

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &tex_units);
	// make sure value is -1 if not supported
	if(oglHaveExtension("GL_NV_vertex_array_range"))
		glGetIntegerv(GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV, &max_VAR_elements);

	tex_compression_avail = (oglHaveExtension("GL_ARB_texture_compression") || oglHaveVersion("1.3")) &&
							(oglHaveExtension("GL_EXT_texture_compression_s3tc"));
	// TODO: GL_S3_s3tc? It uses different enumerants (GL_RGB_S3TC etc), so the
	// texture loading code would need to be changed; and it is not clear whether
	// it supports the full range of DXT1/3/5. (There seems to be no specification;
	// and many header files don't have GL_RGBA_DXT5_S3TC, suggesting that the
	// drivers don't all support that.)

	if(!tex_compression_avail)
	{
		// If there's no hardware support for compressed textures, do the
		// decompression in software (but first let the user know it's probably not
		// going to be very fast).
		wdisplay_msg(L"Performance warning", L"Your graphics card does not support compressed textures. The game will try to continue anyway, but may be slower than expected. Please try updating your graphics drivers; if that doesn't help, please try upgrading your hardware.");
			// TODO: i18n
		glCompressedTexImage2DARB = oglEmulateCompressedTexImage2D;
		
		// Leave tex_compression_avail == false, so that it indicates the presence
		// of hardware-supported texture compression.
	}

	video_mem = (SDL_GetVideoInfo()->video_mem) / 1048576;	// [MiB]
	// TODO: add sizeof(FB)?
}


void CALL_CONV oglEmulateCompressedTexImage2D
	(GLenum target, GLint level, GLenum internalformat,
	 GLsizei width, GLsizei height, GLint border,
	 GLsizei imageSize, const GLvoid* data)
{
	// Software emulation of compressed-texture support, for really old
	// cards/drivers that can't do it (but which do support everything else
	// we strictly require). They probably don't have enough VRAM for all the
	// textures, and are slow anyway, so it's not going to be a pleasant way
	// of playing; but at least it's better than nothing.

	GLenum base_fmt = (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ? GL_RGB : GL_RGBA);
	
	// TODO: handle small (<4x4) images correctly
	GLsizei blocks_w = (GLsizei)(round_up(width, 4) / 4);
	GLsizei blocks_h = (GLsizei)(round_up(height, 4) / 4);
	GLsizei blocks = blocks_w * blocks_h;
	GLsizei size = blocks * 16 * (base_fmt == GL_RGB ? 3 : 4);
	GLsizei pitch = size / (blocks_h*4);
	void* rgb_data = malloc(size);

	bool dxt1 = (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
	bool dxt3 = (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
	bool dxt5 = (internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
	assert(dxt1 || dxt3 || dxt5);

	assert(imageSize == blocks * (dxt1? 8 : 16));

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
					int a;
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

	glTexImage2D(target, level, base_fmt==GL_RGB? GL_RGB8 : GL_RGBA8, width, height, border, base_fmt, GL_UNSIGNED_BYTE, rgb_data);

	free(rgb_data);
}
