#include "precompiled.h"

#include "res.h"
#include "ogl.h"
#include "tex.h"
#include "ogl_tex.h"

#include "lib.h"


int tex_filter = GL_LINEAR;
uint tex_bpp = 32;				// 16 or 32


//////////////////////////////////////////////////////////////////////////////
//
// OpenGL helper routines
//
//////////////////////////////////////////////////////////////////////////////

static bool fmt_is_s3tc(GLenum fmt)
{
	// specified to be contiguous, but this is safer.
	switch(fmt)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return true;
	default:
		return false;
	}
}


static bool filter_uses_mipmaps(GLint filter)
{
	switch(filter)
	{
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		return true;
	default:
		return false;
	}
}


// determine OpenGL texture format, given <bpp> and TexInfo <flags>.
// also choose an internal format based on the global <tex_bpp>
// performance vs. quality setting.
//
// rationale: we override the user's previous internal format preference.
// this is reasonable: 1) internal format is mostly performance optimization
// 2) if it does turn out to be significant, better to reevaluate the
// format decision after a reload than keep the user's setting.
static int get_gl_fmt(int bpp, int flags, GLenum* fmt, GLenum* int_fmt)
{
	const bool alpha = (flags & TEX_ALPHA) != 0;
	const bool bgr   = (flags & TEX_BGR  ) != 0;
	const bool gray  = (flags & TEX_GRAY ) != 0;
	const int  dxt   = flags & TEX_DXT;

	// in case we fail
	*fmt     = 0;
	*int_fmt = 0;

	// S3TC
	if(dxt != 0)
	{
		switch(dxt)
		{
		case 1:
			*fmt = alpha? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		case 3:
			*fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		case 5:
			*fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		default:
			debug_warn("get_gl_fmt: invalid DXT value");
			return ERR_TEX_FMT_INVALID;
		}

		// note: S3TC textures don't need an internal format, since they're
		// uploaded via glCompressedTexImage2DARB. we'll set it anyway for
		// consistency.
		*int_fmt = *fmt;
		return 0;
	}


	// true => 8 bits per component; otherwise, 4
	const bool high_quality = (tex_bpp == 32);

	switch(bpp)
	{
	case 8:
		*fmt = GL_LUMINANCE;
		*int_fmt = high_quality? GL_LUMINANCE8 : GL_LUMINANCE4;
		return 0;
	case 16:
		*fmt = GL_LUMINANCE_ALPHA;
		*int_fmt = high_quality? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE4_ALPHA4;
		return 0;
	case 24:
		assert(!alpha);
		*fmt = bgr? GL_BGR : GL_RGB;
		*int_fmt = high_quality? GL_RGB8 : GL_RGB4;
			// note: BGR can't be used as internal format
		return 0;
	case 32:
		assert(alpha);
		*fmt = bgr? GL_BGRA : GL_RGBA;
		*int_fmt = high_quality? GL_RGBA8 : GL_RGBA4;
			// note: BGR can't be used as internal format
		return 0;
	default:
		debug_warn("get_gl_fmt: invalid bpp");
		return ERR_TEX_FMT_INVALID;
	}

	// unreachable
	assert(0);
}


// return a token for glTexParameteri that will enable automatic mipmap
// generation, or GL_FALSE if the GL implementation doesn't support it.
// does not cache the result.
//
// rationale: we don't assume GL_GENERATE_MIPMAP and GL_GENERATE_MIPMAP_SGIS
// have the same values, although this is implied by the spec governing
// 'promoted' ARB extensions. checking for both the old extension and
// core 1.4 is future-proof.
static GLint detect_auto_mipmap_gen()
{
	// OpenGL 1.4 core. we don't assume this is supported by the driver,
	// since software implementations usually only have 1.1.
	if(oglHaveVersion("1.4"))
		return GL_GENERATE_MIPMAP;

	// widespread extension
	if(oglHaveExtension("GL_SGIS_generate_mipmap"))
		return GL_GENERATE_MIPMAP_SGIS;

	// neither supported; need to build manually, e.g. with gluBuild2DMipmaps.
	return GL_FALSE;
}


//////////////////////////////////////////////////////////////////////////////
//
// texture resource implementation
//
//////////////////////////////////////////////////////////////////////////////

struct Tex
{
	TexInfo ti;

	// allocated by Tex_reload; indicates the texture is currently uploaded.
	GLuint id;

	// determined from TexInfo by gl_get_fmt (called from Tex_reload);
	// user settings passed to tex_upload will override this until the
	// next actual reload.
	GLenum fmt;
	GLenum int_fmt;

	// set to default <tex_filter> by Tex_init; user settings passed to
	// tex_upload will permanently override this.
	GLint filter;

	// flags influencing reload behavior
	bool is_loaded;
		// either we have the texture in memory (referenced by ti.hm),
		// or it's already been uploaded to OpenGL => no reload necessary.
		// needs to be a flag so it can be reset in Tex_dtor.
	bool has_been_uploaded;
};

H_TYPE_DEFINE(Tex);

static void Tex_init(Tex* t, va_list args)
{
	UNUSED(args);

	// set to default (once)
	t->filter = tex_filter;
}

static void Tex_dtor(Tex* t)
{
	tex_free(&t->ti);

	glDeleteTextures(1, &t->id);
	t->id = 0;

	// need to clear this so actual reloads (triggered by h_reload)
	// actually reload.
	t->is_loaded = false;
}


static int Tex_reload(Tex* t, const char* fn, Handle h)
{
	if(t->is_loaded)
		return 0;

	TexInfo* const ti = &t->ti;
	CHECK_ERR(tex_load(fn, ti));
	CHECK_ERR(get_gl_fmt(ti->bpp, ti->flags, &t->fmt, &t->int_fmt));
		// always override previous settings, since format in
		// texture file may have changed (e.g. 24 -> 32 bpp).

	t->is_loaded = true;

	glGenTextures(1, &t->id);

	// re-upload if necessary
	if(t->has_been_uploaded)
		CHECK_ERR(tex_upload(h, t->filter, t->int_fmt));

	return 0;
}


Handle tex_load(const char* fn, int scope)
{
	return h_alloc(H_Tex, fn, scope);
}


int tex_free(Handle& ht)
{
	return h_free(ht, H_Tex);
}


int tex_bind(const Handle h)
{
	Tex* t = H_USER_DATA(h, Tex);
	if(!t)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return ERR_INVALID_HANDLE;
	}

#ifndef NDEBUG
	if(!t->id)
	{
		debug_warn("tex_bind: Tex.id is not a valid texture");
		return -1;
	}
#endif

	glBindTexture(GL_TEXTURE_2D, t->id);
	return 0;
}

int tex_id(const Handle h)
{
	Tex* t = H_USER_DATA(h, Tex);
	return t ? t->id : 0;
}




static int tex_validate(const uint line, const Tex* t)
{
	const char* msg = 0;
	int err = -1;

	// pointer to texture data
	size_t tex_file_size;
	void* tex_file = mem_get_ptr(t->ti.hm, &tex_file_size);
	if(!tex_file)
		msg = "texture file not loaded";
	// possible causes: texture file header is invalid,
	// or file wasn't loaded completely.
	if(t->ti.ofs > tex_file_size)
		msg = "offset to texture data exceeds file size";

	// width, height
	GLsizei w = (GLsizei)t->ti.w;
	GLsizei h = (GLsizei)t->ti.h;
	// if w or h is 0, texture file probably not loaded successfully.
	if(w == 0 || h == 0)
		msg = "width or height is 0 - texture probably not loaded successfully";
	// greater than max supported tex dimension?
	// no-op if oglInit not yet called
	if(w > (GLsizei)max_tex_size || h > (GLsizei)max_tex_size)
		msg = "texture dimensions exceed OpenGL implementation limit";
	// both NV_texture_rectangle and subtexture require work for the client
	// (changing tex coords) => we'll just disallow non-power of 2 textures.
	// TODO: ARB_texture_non_power_of_two
	if(!is_pow2(w) || !is_pow2(h))
		msg = "width or height is not a power-of-2";

	// texel format
	GLenum fmt = (GLenum)t->fmt;
	if(!fmt)
		msg = "texel format is 0";
	// can't really check against a list of valid formats - loaders
	// may define their own. not necessary anyway - if non-0, assume
	// loader knows what it's doing, and that the format is valid.

	// bits per pixel
	u32 bpp = t->ti.bpp;
	// half-hearted sanity check: must be divisible by 4.
	// don't bother checking all values.
	if(bpp % 4 || bpp > 32)
		msg = "invalid bpp? should be one of {4,8,16,24,32}";

	// upload parameters, set by tex_upload(Handle), or 0
	GLint filter  = t->filter;
	GLenum int_fmt = t->int_fmt;
	// TODO: check if valid

	if(msg)
	{
		debug_out("tex_validate at line %d failed: %s (error code %d)\n", line, msg, err);
		debug_warn("tex_validate failed");
		return err;
	}

	return 0;
}

#define CHECK_TEX(t) CHECK_ERR(tex_validate(__LINE__, t))


int tex_upload(const Handle ht, int filter_ovr, int int_fmt_ovr, int fmt_ovr)
{
	H_DEREF(ht, Tex, t);

	// someone's requesting upload, but has already been uploaded.
	// this happens if a cached texture is "loaded". no work to do.
	if(t->id && t->ti.hm <= 0)
		return 0;

	CHECK_TEX(t);	// must come after check above to avoid false alarms

	const char* fn = h_filename(ht);
	if(!fn)
	{
		fn = "(could not determine filename)";
		debug_warn("tex_upload(Handle): h_filename failed");
	}

	// allow user override of format/settings
	if(filter_ovr) t->filter = filter_ovr;
	if(int_fmt_ovr) t->int_fmt = int_fmt_ovr;
	if(fmt_ovr) t->fmt = fmt_ovr;

	// convenient local copies. note: have been validated by CHECK_TEX.
	GLsizei w      = (GLsizei)t->ti.w;
	GLsizei h      = (GLsizei)t->ti.h;
	u32 bpp        = t->ti.bpp;	// used for S3TC size calc
	GLenum fmt     = t->fmt;
	GLint filter   = t->filter;
	GLenum int_fmt = t->int_fmt;
	void* tex_data = (char*)mem_get_ptr(t->ti.hm) + t->ti.ofs;

	// does filter call for uploading mipmaps?
	const bool need_mipmaps = filter_uses_mipmaps(filter);
	static GLint auto_mipmap_gen;
	ONCE(auto_mipmap_gen = detect_auto_mipmap_gen());


	CHECK_ERR(tex_bind(ht));
		// we know ht is valid (H_DEREF above), but tex_bind can
		// fail in debug builds if Tex.id isn't a valid texture name

	// set upload params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	const GLint mag_filter = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
		// magnify can only be linear or nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	if(need_mipmaps)
	{
		if(auto_mipmap_gen != GL_FALSE)
			glTexParameteri(GL_TEXTURE_2D, auto_mipmap_gen, GL_TRUE);
		// note: also works on S3TC-compressed textures
	}

	if(fmt_is_s3tc(fmt))
	{
		// auto generation not supported and gluBuild2DMipmaps doesn't
		// work for precompressed textures => no easy way to generate
		// mipmaps. revert to a filter that doesn't require them.
		if(auto_mipmap_gen == GL_FALSE)
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		const GLsizei tex_size = w * h * bpp / 8;
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, fmt, w, h, 0, tex_size, tex_data);
	}
	// uncompressed texture
	else
	{
		// manual mipmap gen via GLU (box filter)
		if(need_mipmaps && !auto_mipmap_gen)
			gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, w, h, fmt, GL_UNSIGNED_BYTE, tex_data);
		// auto mipmap gen, or no mipmap
		else
			glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, tex_data);
	}

	mem_free_h(t->ti.hm);

	t->has_been_uploaded = true;

	return 0;
}




int tex_info(Handle ht, int* w, int* h, int* fmt, int* bpp, void** p)
{
	H_DEREF(ht, Tex, t);

	if(w)
		*w = t->ti.w;
	if(h)
		*h = t->ti.h;
	if(fmt)
		*fmt = t->fmt;
	if(bpp)
		*bpp = t->ti.bpp;
	if(p)
		*p = mem_get_ptr(t->ti.hm);

	return 0;
}
