#include "precompiled.h"

#include "../res.h"
#include "ogl.h"
#include "tex.h"
#include "ogl_tex.h"

#include "lib.h"


GLint ogl_tex_filter = GL_LINEAR;
uint ogl_tex_bpp = 32;				// 16 or 32


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
	}
	return false;
}


static bool filter_is_known(GLint filter)
{
	switch(filter)
	{
	case GL_NEAREST:
	case GL_LINEAR:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		return true;
	}
	return false;
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


// determine OpenGL texture format, given <bpp> and Tex <flags>.
// also choose an internal format based on the global <ogl_tex_bpp>
// performance vs. quality setting.
//
// rationale: we override the user's previous internal format preference.
// this is reasonable: 1) internal format is mostly performance optimization
// 2) if it does turn out to be significant, better to reevaluate the
// format decision after a reload than keep the user's setting.
static int get_gl_fmt(int bpp, int flags, GLenum* fmt, GLint* int_fmt)
{
	const bool alpha = (flags & TEX_ALPHA) != 0;
	const bool bgr   = (flags & TEX_BGR  ) != 0;
	const bool grey  = (flags & TEX_GREY ) != 0;
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
	const bool high_quality = (ogl_tex_bpp == 32);

	switch(bpp)
	{
	case 8:
		debug_assert(grey);
		*fmt = GL_LUMINANCE;
		*int_fmt = high_quality? GL_LUMINANCE8 : GL_LUMINANCE4;
		return 0;
	case 16:
		*fmt = GL_LUMINANCE_ALPHA;
		*int_fmt = high_quality? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE4_ALPHA4;
		return 0;
	case 24:
		debug_assert(!alpha);
		*fmt = bgr? GL_BGR : GL_RGB;
		*int_fmt = high_quality? GL_RGB8 : GL_RGB4;
			// note: BGR can't be used as internal format
		return 0;
	case 32:
		debug_assert(alpha);
		*fmt = bgr? GL_BGRA : GL_RGBA;
		*int_fmt = high_quality? GL_RGBA8 : GL_RGBA4;
			// note: BGR can't be used as internal format
		return 0;
	default:
		debug_warn("get_gl_fmt: invalid bpp");
		return ERR_TEX_FMT_INVALID;
	}

	UNREACHABLE;
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

struct OglTex
{
	Tex t;

	// allocated by Tex_reload; indicates the texture is currently uploaded.
	GLuint id;

	// determined from Tex by gl_get_fmt (called from Tex_reload);
	// user settings passed to ogl_tex_upload will override this until the
	// next actual reload.
	GLenum fmt;
	GLint int_fmt;

	// set to default <ogl_tex_filter> by Tex_init; user settings passed to
	// ogl_tex_upload will permanently override this.
	GLint filter;

	// flags influencing reload behavior
	bool is_loaded;
		// either we have the texture in memory (referenced by t.hm),
		// or it's already been uploaded to OpenGL => no reload necessary.
		// needs to be a flag so it can be reset in Tex_dtor.
	bool has_been_uploaded;

	// to which Texture Mapping Unit was this bound?
	// used when re-binding after reload.
	uint tmu : 8;
};

H_TYPE_DEFINE(OglTex);

static void OglTex_init(OglTex* ot, va_list UNUSED(args))
{
	// set to default (once)
	ot->filter = ogl_tex_filter;
}

static void OglTex_dtor(OglTex* ot)
{
	tex_free(&ot->t);

	glDeleteTextures(1, &ot->id);
	ot->id = 0;

	// need to clear this so actual reloads (triggered by h_reload)
	// actually reload.
	ot->is_loaded = false;
}


static int OglTex_reload(OglTex* ot, const char* fn, Handle h)
{
	if(ot->is_loaded)
		return 0;

	Tex* const t = &ot->t;
	CHECK_ERR(tex_load(fn, t));
	CHECK_ERR(get_gl_fmt(t->bpp, t->flags, &ot->fmt, &ot->int_fmt));
		// always override previous settings, since format in
		// texture file may have changed (e.g. 24 -> 32 bpp).

	ot->is_loaded = true;

	glGenTextures(1, &ot->id);

	// re-upload if necessary
	if(ot->has_been_uploaded)
		CHECK_ERR(ogl_tex_upload(h, ot->filter, ot->int_fmt));

	return 0;
}


Handle ogl_tex_load(const char* fn, int scope)
{
	return h_alloc(H_OglTex, fn, scope);
}


int ogl_tex_free(Handle& ht)
{
	return h_free(ht, H_OglTex);
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to current active unit
void CRenderer::BindTexture(int unit,GLuint tex)
{
#if 0
glActiveTextureARB(GL_TEXTURE0+unit);
if (tex==m_ActiveTextures[unit]) return;

if (tex) {
glBindTexture(GL_TEXTURE_2D,tex);
if (!m_ActiveTextures[unit]) {
glEnable(GL_TEXTURE_2D);
}
} else if (m_ActiveTextures[unit]) {
glDisable(GL_TEXTURE_2D);
}
m_ActiveTextures[unit]=tex;
#endif

glActiveTextureARB(GL_TEXTURE0+unit);

glBindTexture(GL_TEXTURE_2D,tex);
if (tex) {
glEnable(GL_TEXTURE_2D);
} else {
glDisable(GL_TEXTURE_2D);
}
m_ActiveTextures[unit]=tex;
}
*/


// note: there are many call sites of glActiveTextureARB, so caching
// those and ignoring redundant sets isn't feasible.
int ogl_tex_bind(const Handle h, GLenum unit)
{
	int id = 0;

	// special case: avoid dereference and disable texturing directly.
	if(h == 0)
		goto disable_texturing;

	{
	// (we can't use H_DEREF because it exits immediately)
	OglTex* ot = H_USER_DATA(h, OglTex);
	if(!ot)
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return ERR_INVALID_HANDLE;
	}

#ifndef NDEBUG
	if(!ot->id)
	{
		debug_warn("ogl_tex_bind: OglTex.id is not a valid texture");
		return -1;
	}
#endif

	id = ot->id;
	ot->tmu = unit;
	}

disable_texturing:
	glActiveTextureARB(GL_TEXTURE0+unit);
	glBindTexture(GL_TEXTURE_2D, id);
	if(id)
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);
	return 0;
}


static int ogl_tex_validate(const uint line, const OglTex* ot)
{
	const char* msg = 0;
	int err = -1;

	// pointer to texture data
	size_t tex_file_size;
	void* tex_file = mem_get_ptr(ot->t.hm, &tex_file_size);
	if(!tex_file)
		msg = "texture file not loaded";
	// possible causes: texture file header is invalid,
	// or file wasn't loaded completely.
	if(ot->t.ofs > tex_file_size)
		msg = "offset to texture data exceeds file size";

	// width, height
	GLsizei w = (GLsizei)ot->t.w;
	GLsizei h = (GLsizei)ot->t.h;
	// if w or h is 0, texture file probably not loaded successfully.
	if(w == 0 || h == 0)
		msg = "width or height is 0 - texture probably not loaded successfully";
	// greater than max supported tex dimension?
	// no-op if oglInit not yet called
	if(w > (GLsizei)ogl_max_tex_size || h > (GLsizei)ogl_max_tex_size)
		msg = "texture dimensions exceed OpenGL implementation limit";
	// both NV_texture_rectangle and subtexture require work for the client
	// (changing tex coords) => we'll just disallow non-power of 2 textures.
	// TODO: ARB_texture_non_power_of_two
	if(!is_pow2(w) || !is_pow2(h))
		msg = "width or height is not a power-of-2";

	// texel format
	GLenum fmt = (GLenum)ot->fmt;
	if(!fmt)
		msg = "texel format is 0";
	// can't really check against a list of valid formats - loaders
	// may define their own. not necessary anyway - if non-0, assume
	// loader knows what it's doing, and that the format is valid.

	// bits per pixel
	u32 bpp = ot->t.bpp;
	// half-hearted sanity check: must be divisible by 4.
	// don't bother checking all values.
	if(bpp % 4 || bpp > 32)
		msg = "invalid bpp? should be one of {4,8,16,24,32}";

	// upload parameters, set by ogl_tex_upload(Handle), or 0
	GLint filter  = ot->filter;
	if(filter != 0 && !filter_is_known(filter))
		msg = "invalid filter";
	// as with the texel format above, there is not anything we can do
	// to verify ot->int_fmt is correct (even 0 is valid).
	if(ot->tmu >= 128)
		msg = "TMU invalid? it's >= 128!";

	if(msg)
	{
		debug_printf("tex_validate at line %d failed: %s (error code %d)\n", line, msg, err);
		debug_warn("tex_validate failed");
		return err;
	}

	return 0;
}

#define CHECK_OGL_TEX(t) CHECK_ERR(ogl_tex_validate(__LINE__, t))


int ogl_tex_upload(const Handle ht, GLint filter_ovr, GLint int_fmt_ovr, GLenum fmt_ovr)
{
	H_DEREF(ht, OglTex, ot);

	// someone's requesting upload, but has already been uploaded.
	// this happens if a cached texture is "loaded". no work to do.
	if(ot->id && ot->t.hm <= 0)
		return 0;

	CHECK_OGL_TEX(ot);	// must come after check above to avoid false alarms

	const char* fn = h_filename(ht);
	if(!fn)
	{
		fn = "(could not determine filename)";
		debug_warn("ogl_tex_upload(Handle): h_filename failed");
	}

	// allow user override of format/settings
	if(filter_ovr) ot->filter = filter_ovr;
	if(int_fmt_ovr) ot->int_fmt = int_fmt_ovr;
	if(fmt_ovr) ot->fmt = fmt_ovr;

	// convenient local copies. note: have been validated by CHECK_TEX.
	GLsizei w      = (GLsizei)ot->t.w;
	GLsizei h      = (GLsizei)ot->t.h;
	u32 bpp        = ot->t.bpp;	// used for S3TC/mipmap size calc
	GLenum fmt     = ot->fmt;
	GLint filter   = ot->filter;
	GLint int_fmt  = ot->int_fmt;
	void* tex_data = tex_get_data(&ot->t);

	// does filter call for uploading mipmaps?
	const bool need_mipmaps = filter_uses_mipmaps(filter);
	static GLint auto_mipmap_gen;
	ONCE(auto_mipmap_gen = detect_auto_mipmap_gen());


	// we know ht is valid (H_DEREF above), but ogl_tex_bind can
	// fail in debug builds if OglTex.id isn't a valid texture name
	CHECK_ERR(ogl_tex_bind(ht, ot->tmu));

	// set upload params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	const GLint mag_filter = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
		// magnify can only be linear or nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	/*
		There are various combinations of desires/abilities, relating to how
		(and whether) mipmaps should be generated. Currently there are only
		2^4 such combinations:

		    /mipmaps available in texture
		    #######
		/mipmaps needed
		#######
		.---+---+---+---.
		| Au| Mu| Nu| Nu|#-auto mipmap generation available
		|---+---+---+---|#
		| Ac| Mc| Nc| Nc|# #-texture is compressed
		|---+---+---+---|# #
		| X | Mc| Nc| Nc|  #
		|---+---+---+---|  #
		| G | Mu| Nu| Nu|
		`---+---+---+---'

		Au (auto_uncomp)   = auto_mipmap_gen, then 'Nu'
		Ac (auto_comp)     = auto_mipmap_gen, then 'Nc'
		X  (broken_comp)   = failure; just fall back to GL_LINEAR and 'Nc'
		G  (glubuild)      = gluBuild2DMipmaps
		Mu (mipped_uncomp) = glTexImage2D, mipmap levels
		Mc (mipped_comp)   = glCompressedTexImage2DARB, mipmap levels
		Nu (normal_uncomp) = glTexImage2D
		Nc (normal_comp)   = glCompressedTexImage2DARB

		if (Au || Ac)
			enable automatic mipmap generation
			switch to 'N*'
		if (X)
			set GL_LINEAR
			switch to 'Nc'
		if (G)
			gluBuild2DMipmaps
		if (Nu)
			glTexImage2D
		if (Nc)
			glCompressedTexImage2DARB
		if (Mu)
			for each mipmap level
				glTexImage2D
		if (Mc)
			for each mipmap level
				glCompressedTexImage2DARB

		[This documentation feels like more than is really necessary, but
		hopefully it'll prevent the logic getting horribly tangled...]
	*/

	bool is_compressed = fmt_is_s3tc(fmt);
	bool has_mipmaps = (ot->t.flags & TEX_MIPMAPS ? true : false);

	enum UploadState
	{
		auto_uncomp, auto_comp,
		mipped_uncomp, mipped_comp,
		normal_uncomp, normal_comp,
		broken_comp,
		glubuild
	};
	static const int states[4][4] = {
		{ auto_uncomp, mipped_uncomp, normal_uncomp, normal_uncomp },
		{ auto_comp,   mipped_comp,   normal_comp,   normal_comp   },
		{ broken_comp, mipped_comp,   normal_comp,   normal_comp   },
		{ glubuild,    mipped_uncomp, normal_uncomp, normal_uncomp }
	};
	int state = states[auto_mipmap_gen ? (is_compressed ? 1 : 0) : (is_compressed ? 2 : 3)]  // row
	                  [need_mipmaps    ? (has_mipmaps   ? 1 : 0) : (has_mipmaps   ? 2 : 3)]; // column

	if(state == auto_uncomp || state == auto_comp)
	{
		glTexParameteri(GL_TEXTURE_2D, auto_mipmap_gen, GL_TRUE);
		state = (state == auto_uncomp)? normal_uncomp : normal_comp;
	}

	if(state == broken_comp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		state = normal_comp;
	}

	if(state == glubuild)
		gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, w, h, fmt, GL_UNSIGNED_BYTE, tex_data);
	else if(state == normal_uncomp)
		glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, tex_data);
	else if(state == normal_comp)
	{
		const GLsizei tex_size = w * h * bpp / 8;
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, fmt, w, h, 0, tex_size, tex_data);
	}
	else if(state == mipped_uncomp || state == mipped_comp)
	{
		int level = 0;
		GLsizei level_w = w;
		GLsizei level_h = h;
		char* mipmap_data = (char*)tex_data;

		while(level_w || level_h) // loop until the 1x1 mipmap level has just been processed
		{
			// If the texture is non-square, one of the dimensions will become
			// 0 before the other. To satisfy OpenGL's expectations, change it
			// back to 1.
			if(level_w == 0) level_w = 1;
			if(level_h == 0) level_h = 1;

			GLsizei tex_size;
			if(state == mipped_uncomp)
			{
				tex_size = level_w * level_h * bpp/8;
				glTexImage2D(GL_TEXTURE_2D, level, int_fmt, level_w, level_h, 0, fmt, GL_UNSIGNED_BYTE, mipmap_data);
			}
			else // state == mipped_comp
			{
				// Round up to an integer number of 4x4 blocks
				tex_size = (GLsizei)(round_up(level_w, 4) * round_up(level_h, 4) * bpp/8);
				glCompressedTexImage2DARB(GL_TEXTURE_2D, level, fmt, level_w, level_h, 0, tex_size, mipmap_data);
			}

			mipmap_data += tex_size;
			level++;
			level_w /= 2;
			level_h /= 2;
		}
	}
	else
		debug_warn("Invalid state in ogl_tex_upload");

	mem_free_h(ot->t.hm);

	ot->has_been_uploaded = true;

	oglCheck();

	return 0;
}


int ogl_tex_get_size(Handle ht, int* w, int* h, int* bpp)
{
	H_DEREF(ht, OglTex, ot);
	if(w)
		*w = ot->t.w;
	if(h)
		*h = ot->t.h;
	if(bpp)
		*bpp = ot->t.bpp;
	return 0;
}


int ogl_tex_get_format(Handle ht, int* flags, GLenum* fmt)
{
	H_DEREF(ht, OglTex, ot);
	if(flags)
		*flags = ot->t.flags;
	if(fmt)
		*fmt = ot->fmt;
	return 0;
}


int ogl_tex_get_data(Handle ht, void** p)
{
	H_DEREF(ht, OglTex, ot);
	*p = tex_get_data(&ot->t);
	return 0;
}
