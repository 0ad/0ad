#include "precompiled.h"

#define CHEEZY_NOMIPMAP

#include "res.h"
#include "ogl.h"
#include "tex.h"
#include "ogl_tex.h"

#include "lib.h"


static int get_gl_fmt(TexInfo* ti, GLenum* fmt)
{
	const bool alpha = (ti->flags & TEX_ALPHA) != 0;
	const bool bgr = (ti->flags & TEX_BGR) != 0;
	const bool gray = (ti->flags & TEX_GRAY) != 0;

	switch(ti->flags & TEX_DXT)
	{
	case 1:
		*fmt = alpha? GL_COMPRESSED_RGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		return 0;
	case 3:
		*fmt = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		return 0;
	case 5:
		*fmt = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		return 0;
	}

	switch(ti->bpp)
	{
	case 8:
		*fmt = GL_LUMINANCE;
		return 0;

	case 16:
		*fmt = GL_LUMINANCE_ALPHA;
		return 0;
	case 24:
		if(!alpha)
		{
			*fmt = bgr? GL_BGR : GL_RGB;
			return 0;
		}
		break;
	case 32:
		if(alpha)
		{
			*fmt = bgr? GL_BGRA : GL_RGBA;
			return 0;
		}
		break;
	}

	return ERR_TEX_FMT_INVALID;
}


struct Tex
{
	TexInfo ti;

	// determined from TexInfo by Tex_reload
	GLenum fmt;

	// allocated by Tex_reload
	GLuint id;

	// set from user param by tex_upload
	GLint filter;
	GLenum int_fmt;

	// flags influencing reload behavior
	bool is_loaded;
	bool has_been_uploaded;
};

H_TYPE_DEFINE(Tex);

static void Tex_init(Tex* t, va_list args)
{
	UNUSED(t);
	UNUSED(args);
}

static void Tex_dtor(Tex* t)
{
	tex_free(&t->ti);

	glDeleteTextures(1, &t->id);

	// need to clear this so actual reloads (triggered by h_reload)
	// actually reload.
	t->is_loaded = false;
}


static int Tex_reload(Tex* t, const char* fn, Handle h)
{
	if(t->is_loaded)
		return 0;

	CHECK_ERR(tex_load(fn, &t->ti));
	CHECK_ERR(get_gl_fmt(&t->ti, &t->fmt));
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


int tex_filter = GL_LINEAR;
uint tex_bpp = 32;				// 16 or 32


static int choose_upload_param(Tex* t, GLint* _filter, GLenum* _int_fmt)
{
	if(!*_filter)
		*_filter = tex_filter;

	// internal format already explicitly requested; we're done.
	if(*_int_fmt != 0)
		return 0;

	GLenum int_fmt = 0;

	// choose a specific, sized internal format for common formats,
	// based on the global tex_bpp setting (32 or 16 bpp textures).
	// could just let the driver choose, but this gives the user
	// control for performance/quality tweaking.
	// 
	// rare formats (e.g. GL_BGR) not in the switch statements
	// are handled below.

	// .. high quality, 32 bit textures (8 bits per component)
	if(tex_bpp == 32)
	{
		switch(t->fmt)
		{
		case GL_RGBA:
			int_fmt = GL_RGBA8;
			break;
		case GL_RGB:
			int_fmt = GL_RGB8;
			break;
		case GL_LUMINANCE_ALPHA:
			int_fmt = GL_LUMINANCE8_ALPHA8;
			break;
		case GL_ALPHA:
			int_fmt = GL_ALPHA8;
			break;
		case GL_LUMINANCE:
			int_fmt = GL_LUMINANCE8;
			break;
		}
	}
	// .. low quality, 16 bit textures (4 bits per component)
	else
	{
		switch(t->fmt)
		{
		case GL_RGBA:
			int_fmt = GL_RGBA4;
			break;
		case GL_RGB:
			int_fmt = GL_RGB4;
			break;
		case GL_LUMINANCE_ALPHA:
			int_fmt = GL_LUMINANCE4_ALPHA4;
			break;
		case GL_ALPHA:
			int_fmt = GL_ALPHA4;
			break;
		case GL_LUMINANCE:
			int_fmt = GL_LUMINANCE4;
			break;
		}
	}

	// fmt wasn't in the list above, so we haven't chosen
	// the internal format yet, and need to do so now.
	// set it to # of components in the texture.
	// note: can't use the texture data's format -
	// not all can be used as an internal format! (e.g. GL_BGR)
	if(!int_fmt)
		int_fmt = t->ti.bpp / 8;

	*_int_fmt = int_fmt;

	return 0;
}


int tex_upload(const Handle ht, int filter_ovr, int int_fmt_ovr, int fmt_ovr)
{
	H_DEREF(ht, Tex, t);
	
	// someone's requesting upload, but has already been uploaded.
	// this happens if a cached texture is "loaded". no work to do.
	if(t->id && t->ti.hm <= 0)
		return 0;
	
	CHECK_TEX(t);

	const char* fn = h_filename(ht);
	if(!fn)
	{
		fn = "(could not determine filename)";
		debug_warn("tex_upload(Handle): h_filename failed");
	}

	// note: vars already verified by CHECK_TEX.
	GLsizei w   = (GLsizei)t->ti.w;
	GLsizei h   = (GLsizei)t->ti.h;
	u32 bpp     = t->ti.bpp;	// not used directly in gl calls
	GLenum fmt  = t->fmt;
	// .. reference: changed by override below and choose_upload_param
	GLint& filter  = t->filter;
	GLenum& int_fmt = t->int_fmt;
	void* tex_data = (char*)mem_get_ptr(t->ti.hm) + t->ti.ofs;

	// allow override
	if(filter_ovr) filter = filter_ovr;
	if(int_fmt_ovr) int_fmt = int_fmt_ovr;
	if(fmt_ovr) fmt = fmt_ovr;

#ifdef CHEEZY_NOMIPMAP
	filter = GL_NEAREST_MIPMAP_LINEAR;
#endif
	CHECK_ERR(tex_bind(ht));
		// we know ht is valid (H_DEREF above), but tex_bind can
		// fail in debug builds if Tex.id isn't a valid texture name

	CHECK_ERR(choose_upload_param(t, &filter, &int_fmt));

	// set upload params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	const GLint mag = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
		// filter allows mipmaps; magnify can only be linear or nearest
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);

	// does filter call for uploading mipmaps?
	const bool mipmap = (filter == GL_NEAREST_MIPMAP_NEAREST || filter == GL_LINEAR_MIPMAP_NEAREST ||
		filter == GL_NEAREST_MIPMAP_LINEAR  || filter == GL_LINEAR_MIPMAP_LINEAR);

	// check if SGIS_generate_mipmap is available (once)
	static int sgm_avl = -1;
	if(sgm_avl == -1)
		sgm_avl = (int)oglExtAvail("GL_SGIS_generate_mipmap");

	// S3TC compressed
	if(fmt >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
		fmt <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		const int tex_size = w * h * bpp / 8;

		// RC, 020404: added mipmap generation for DDS textures using GL_SGIS_generate_mipmap - works fine
		// on ATI cards, verified by others under NVIDIA
		if(mipmap)
		{
			if (sgm_avl)
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
			else
				// ack - no (easy) way of generating mipmaps for compressed textures; switch back
				// to a linear minification filter
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}


		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, fmt, w, h, 0, tex_size, tex_data);
	}
	// normal
	else
	{

		// manual mipmap gen via GLU (box filter)
		if(mipmap && !sgm_avl)
			gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, w, h, fmt, GL_UNSIGNED_BYTE, tex_data);
		// auto mipmap gen, or no mipmap
		else
		{
			if(mipmap)
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

			glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, tex_data);

		}
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
