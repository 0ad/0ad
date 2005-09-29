// OpenGL texture API
//
// Copyright (c) 2003-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "lib.h"
#include "../res.h"
#include "ogl.h"
#include "tex.h"
#include "ogl_tex.h"


//----------------------------------------------------------------------------
// OpenGL helper routines
//----------------------------------------------------------------------------

static bool filter_valid(GLint filter)
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
	default:
		return false;
	}
}


static bool wrap_valid(GLint wrap)
{
	switch(wrap)
	{
	case GL_CLAMP:
	case GL_CLAMP_TO_EDGE:
	case GL_CLAMP_TO_BORDER:
	case GL_REPEAT:
	case GL_MIRRORED_REPEAT:
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


//----------------------------------------------------------------------------
// quality mechanism
//----------------------------------------------------------------------------

static GLint default_filter = GL_LINEAR;	// one of the GL *minify* filters
static uint default_q_flags = OGL_TEX_FULL_QUALITY;	// OglTexQualityFlags

// are the given q_flags valid? (used when checking parameter validity)
static bool q_flags_valid(uint q_flags)
{
	const uint bits = OGL_TEX_FULL_QUALITY|OGL_TEX_HALF_BPP|OGL_TEX_HALF_RES;
	// unrecognized bits are set - invalid
	if((q_flags & ~bits) != 0)
		return false;
	// "full quality" but other reduction bits are set - invalid
	if(q_flags & OGL_TEX_FULL_QUALITY && q_flags & ~OGL_TEX_FULL_QUALITY)
		return false;
	return true;
}


// change default settings - these affect performance vs. quality.
// may be overridden for individual textures via parameter to
// ogl_tex_upload or ogl_tex_set_filter, respectively.
// 
// pass 0 to keep the current setting; defaults and legal values are:
// - q_flags: OGL_TEX_FULL_QUALITY; combination of OglTexQualityFlags 
// - filter: GL_LINEAR; any valid OpenGL minification filter
void ogl_tex_set_defaults(uint q_flags, GLint filter)
{
	if(q_flags)
	{
		debug_assert(q_flags_valid(q_flags));
		default_q_flags = q_flags;
	}

	if(filter)
	{
		debug_assert(filter_valid(filter));
		default_filter = filter;
	}
}


// determine OpenGL texture format, given <bpp> and Tex <flags>.
// also choose an internal format based on the given q_flags.
static int get_gl_fmt(uint bpp, uint flags, uint q_flags, GLenum* fmt, GLint* int_fmt)
{
	const bool alpha = (flags & TEX_ALPHA) != 0;
	const bool bgr   = (flags & TEX_BGR  ) != 0;
	const bool grey  = (flags & TEX_GREY ) != 0;
	const uint dxt   = flags & TEX_DXT;

	// true => 4 bits per component; otherwise, 8
	const bool half_bpp = (q_flags & OGL_TEX_HALF_BPP) != 0;

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

	// uncompressed
	switch(bpp)
	{
	case 8:
		debug_assert(grey);
		*fmt = GL_LUMINANCE;
		*int_fmt = half_bpp? GL_LUMINANCE4 : GL_LUMINANCE8;
		return 0;
	case 16:
		*fmt = GL_LUMINANCE_ALPHA;
		*int_fmt = half_bpp? GL_LUMINANCE4_ALPHA4 : GL_LUMINANCE8_ALPHA8;
		return 0;
	case 24:
		debug_assert(!alpha);
		*fmt = bgr? GL_BGR : GL_RGB;
		// note: BGR can't be used as internal format
		*int_fmt = half_bpp? GL_RGB4 : GL_RGB8;
		return 0;
	case 32:
		debug_assert(alpha);
		*fmt = bgr? GL_BGRA : GL_RGBA;
		// note: BGR can't be used as internal format
		*int_fmt = half_bpp? GL_RGBA4 : GL_RGBA8;
		return 0;
	default:
		debug_warn("get_gl_fmt: invalid bpp");
		return ERR_TEX_FMT_INVALID;
	}

	UNREACHABLE;
}


//----------------------------------------------------------------------------
// texture state to allow seamless reload
//----------------------------------------------------------------------------

// see "Texture Parameters" in docs.

// all GL state tied to the texture that must be reapplied after reload.
// (this mustn't get too big, as it's stored in the already sizeable OglTex)
struct OglTexState
{
	// glTexParameter
	// note: there are more options, but they do not look to
	//       be important and will not be applied after a reload!
	//       in particular, LOD_BIAS isn't needed because that is set for
	//       the entire texturing unit via glTexEnv.
	// .. texture filter
	//    note: this is the minification filter value; magnification filter
	//          is GL_NEAREST if it's GL_NEAREST, otherwise GL_LINEAR.
	//          we don't store mag_filter explicitly because it
	//          doesn't appear useful - either apps can tolerate LINEAR, or
	//          mipmaps aren't called for and filter could be NEAREST anyway).
	GLint filter;
	// .. wrap mode
	//    note: to simplify things, we assume that apps will never want to
	//          set S/T modes independently. it that becomes necessary,
	//          it's easy to add.
	GLint wrap;
};


// fill the given state object with default values.
static void state_set_to_defaults(OglTexState* ots)
{
	ots->filter = default_filter;
	ots->wrap = GL_REPEAT;
}


// send all state to OpenGL (actually the currently bound texture).
// called from ogl_tex_upload.
static void state_latch(OglTexState* ots)
{
	// filter
	const GLint filter = ots->filter;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	const GLint mag_filter = (filter == GL_NEAREST)? GL_NEAREST : GL_LINEAR;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	// wrap
	const GLint wrap = ots->wrap;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	// .. only CLAMP and REPEAT are guaranteed to be available.
	//    if we're using one of the others, we squelch the error that
	//    may have resulted if this GL implementation is old.
	if(wrap != GL_CLAMP && wrap != GL_REPEAT)
		oglSquelchError(GL_INVALID_ENUM);
}


//----------------------------------------------------------------------------
// texture resource object
//----------------------------------------------------------------------------

// ideally we would split OglTex into data and state objects as in
// snd.cpp's SndData / VSrc. this gives us the benefits of caching while 
// still leaving each "instance" (state object, which owns a data reference)
// free to change its state. however, unlike in OpenAL, there is no state
// independent of the data object - all parameters are directly tied to the
// GL texture object. therefore, splitting them up is impossible.
// (we shouldn't even keep the texel data in memory since that's already
// covered by the FS cache).
//
// given that multiple "instances" share the state stored here, we conclude:
// - a refcount is necessary to prevent ogl_tex_upload from freeing
//   <t> as long as other instances are active.
// - concurrent use risks cross-talk (if the 2nd "instance" changes state and
//   the first is reloaded, its state may change to that of the 2nd)
//
// as bad as it sounds, the latter issue isn't a problem: we do not expect
// multiple instances of the same texture where someone changes its filter.
// even if it is reloaded, the differing state is not critical.
// the alternative is even worse: disabling *all* caching/reuse would
// really hurt performance and h_mgr doesn't support only disallowing
// reuse of active objects (this would break the index lookup code, since
// multiple instances may then exist).

struct OglTex
{
	Tex t;

	// allocated by OglTex_reload; indicates the texture is currently uploaded.
	GLuint id;

	// ogl_tex_upload calls get_gl_fmt to determine these from <t>.
	// however, its caller may override those values via parameters.
	// note: these are stored here to allow retrieving via ogl_tex_get_format;
	// they are only used within ogl_tex_upload.
	GLenum fmt;
	GLint int_fmt;

	OglTexState state;

	// OglTexQualityFlags
	uint q_flags : 8;

	// to which Texture Mapping Unit was this bound?
	uint tmu : 8;

	// flags influencing reload behavior
	// .. either we have the texture in memory (referenced by t.hm),
	//    or it's already been uploaded to OpenGL => no reload necessary.
	//    needs to be a flag so it can be reset in Tex_dtor.
	uint is_loaded : 1;
	// .. texture has been uploaded to OpenGL => that needs to be done
	//    when reloading it.
	uint has_been_uploaded : 1;
	// ..
	uint is_currently_uploaded : 1;
	// .. this texture wasn't actually loaded from disk - we just
	//    copied a Tex object into t. if set, actual reloads are disallowed
	//    (see ogl_tex_load), but we still need this to avoid calling
	//    tex_load in reload() triggered by h_alloc.
	uint was_wrapped : 1;
};

H_TYPE_DEFINE(OglTex);

static void OglTex_init(OglTex* ot, va_list args)
{
	Tex* wrapped_tex = va_arg(args, Tex*);
	if(wrapped_tex)
	{
		// note: this only happens once; ogl_tex_wrap makes sure
		// this OglTex cannot be reloaded, so it's safe.
		ot->t = *wrapped_tex;
		ot->was_wrapped = 1;
	}

	state_set_to_defaults(&ot->state);
	ot->q_flags = default_q_flags;
}

static void OglTex_dtor(OglTex* ot)
{
	(void)tex_free(&ot->t);

	glDeleteTextures(1, &ot->id);
	ot->id = 0;

	ot->is_currently_uploaded = 0;

	// need to clear this so actual reloads (triggered by h_reload)
	// actually reload.
	ot->is_loaded = 0;
}

static int OglTex_reload(OglTex* ot, const char* fn, Handle h)
{
	// make sure the texture has been loaded
	// .. already done (<h> had been freed but not yet unloaded)
	if(ot->is_loaded)
		return 0;
	// .. load from file - but only if we weren't wrapping an existing
	//    Tex object (i.e. copy its values and be done).
	if(!ot->was_wrapped)
		RETURN_ERR(tex_load(fn, &ot->t));
	ot->is_loaded = 1;

	glGenTextures(1, &ot->id);

	// if it had already been uploaded before this reload,
	// re-upload it (this also does state_latch).
	if(ot->has_been_uploaded)
		(void)ogl_tex_upload(h);

	return 0;
}


// load and return a handle to the texture given in <fn>.
// for a list of supported formats, see tex.h's tex_load.
Handle ogl_tex_load(const char* fn, uint flags)
{
	Tex* wrapped_tex = 0;	// we're loading from file
	return h_alloc(H_OglTex, fn, flags, wrapped_tex);
}


// make the given Tex object ready for use as an OpenGL texture
// and return a handle to it. this will be as if its contents
// had been loaded by ogl_tex_load.
//
// we need only add bookkeeping information and "wrap" it in
// a resource object (accessed via Handle), hence the name.
//
// <fn> isn't strictly needed but should describe the texture so that
// h_filename will return a meaningful comment for debug purposes.
Handle ogl_tex_wrap(Tex* t, const char* fn, uint flags)
{
	// this object may not be backed by a file ("may", because
	// someone could do tex_load and then ogl_tex_wrap).
	// if h_mgr asks for a reload, the dtor will be called but
	// we won't be able to reconstruct it. therefore, disallow reloads.
	// (they are improbable anyway since caller is supposed to pass a
	// 'descriptive comment' instead of filename, but don't rely on that)
	flags |= RES_DISALLOW_RELOAD;
	return h_alloc(H_OglTex, fn, flags, t);
}


// free all resources associated with the texture and make further
// use of it impossible. (subject to refcount)
int ogl_tex_free(Handle& ht)
{
	return h_free(ht, H_OglTex);
}




static int ogl_tex_validate(const uint line, const OglTex* ot)
{
	RETURN_ERR(tex_validate(line, &ot->t));

	const char* msg = 0;
	int err = -1;

	// width, height
	// (note: this is done here because tex.cpp doesn't impose any
	// restrictions on dimensions, while OpenGL does).
	GLsizei w = (GLsizei)ot->t.w;
	GLsizei h = (GLsizei)ot->t.h;
	// .. if w or h is 0, texture file probably not loaded successfully.
	if(w == 0 || h == 0)
		msg = "width or height is 0 - texture probably not loaded successfully";
	// .. greater than max supported tex dimension?
	//    no-op if oglInit not yet called
	if(w > (GLsizei)ogl_max_tex_size || h > (GLsizei)ogl_max_tex_size)
		msg = "texture dimensions exceed OpenGL implementation limit";
	// .. both NV_texture_rectangle and subtexture require work for the client
	//    (changing tex coords) => we'll just disallow non-power of 2 textures.
	//    TODO: ARB_texture_non_power_of_two
	if(!is_pow2(w) || !is_pow2(h))
		msg = "width or height is not a power-of-2";

	// upload parameters, set by ogl_tex_upload(Handle), or 0
	GLint filter = ot->state.filter;
	GLint wrap   = ot->state.wrap;
	if(filter != 0 && !filter_valid(filter))
		msg = "invalid filter";
	if(wrap != 0 && !wrap_valid(wrap))
		msg = "invalid wrap mode";
	if(!q_flags_valid(ot->q_flags))
		msg = "invalid q_flags";
	if(ot->tmu >= 128)
		msg = "TMU invalid? it's >= 128!";
	// .. note: don't check ot->fmt and ot->int_fmt - they aren't set
	//    until during ogl_tex_upload.

	if(msg)
	{
		debug_printf("ogl_tex_validate at line %d failed: %s (error code %d)\n", line, msg, err);
		debug_warn("ogl_tex_validate failed");
		return err;
	}

	return 0;
}

#define CHECK_OGL_TEX(ot) CHECK_ERR(ogl_tex_validate(__LINE__, ot))


//----------------------------------------------------------------------------
// state setters (see "Texture Parameters" in docs)

// rationale: these must be called before uploading; this simplifies
// things and avoids calling glTexParameter twice.
static void warn_if_uploaded(Handle ht, const OglTex* ot)
{
	int refs = h_get_refcnt(ht);

	if(refs == 1 && ot->is_currently_uploaded)
		debug_warn("ogl_tex_set_*: texture already uploaded and shouldn't be changed");
}


// override default filter (as set above) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// filter is as defined by OpenGL; it is applied for both minification and
// magnification (for rationale and details, see OglTexState)
int ogl_tex_set_filter(Handle ht, GLint filter)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	if(!filter_valid(filter))
		CHECK_ERR(ERR_INVALID_PARAM);

	warn_if_uploaded(ht, ot);
	ot->state.filter = filter;
	return 0;
}


// override default wrap mode (GL_REPEAT) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// wrap is as defined by OpenGL and applies to both S and T coordinates
// (rationale: see OglTexState).
int ogl_tex_set_wrap(Handle ht, GLint wrap)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	if(!wrap_valid(wrap))
		CHECK_ERR(ERR_INVALID_PARAM);

	warn_if_uploaded(ht, ot);
	ot->state.wrap = wrap;
	return 0;
}


//----------------------------------------------------------------------------
// upload

// bind the texture to the specified unit [number] in preparation for
// using it in rendering. assumes multitexturing is available.
// not necessary before calling ogl_tex_upload!
// side effects:
// - enables (or disables, if <ht> == 0) texturing on the given unit.
//
// note: there are many call sites of glActiveTextureARB, so caching
// those and ignoring redundant sets isn't feasible.
int ogl_tex_bind(const Handle ht, GLenum unit)
{
	int id = 0;

	// special case: avoid dereference and disable texturing directly.
	if(ht == 0)
		goto disable_texturing;

	{
		// (we can't use H_DEREF because it exits immediately)
		OglTex* ot = H_USER_DATA(ht, OglTex);
		if(!ot)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			CHECK_ERR(ERR_INVALID_HANDLE);
			UNREACHABLE;
		}

		CHECK_OGL_TEX(ot);

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
	if(id)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, id);
	}
	else
		glDisable(GL_TEXTURE_2D);
	return 0;
}


// this has gotten to be quite complex. due to various fallbacks, we
// implement this as an automaton with the following states:
enum UploadState
{
	auto_uncomp, auto_comp,
	mipped_uncomp, mipped_comp,
	normal_uncomp, normal_comp,
	broken_comp,
	glubuild
};


// use parameters describing a texture to decide which upload method is
// called for; returns one of the above "states".
static UploadState determine_upload_state(GLenum fmt, GLint filter, uint tex_flags)
{
	/*
		There are various combinations of desires/abilities, relating to how
		(and whether) mipmaps should be generated. Currently there are only
		2^4 of them:

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

	// decisions:
	// .. does filter call for uploading mipmaps?
	const bool need_mipmaps = filter_uses_mipmaps(filter);
	// .. does this OpenGL implementation support auto mipmap generation?
	static const bool auto_mipmap_gen = oglHaveVersion("1.4") || oglHaveExtension("GL_SGIS_generate_mipmap");
	// .. is this texture in S3TC format? (more generally, "compressed")
	const bool is_s3tc = ogl_dxt_from_fmt(fmt) != 0;
	// .. does the image data include mipmaps? (stored as separate
	//    images after the regular texels)
	const bool includes_mipmaps = (tex_flags & TEX_MIPMAPS) != 0;

	static const UploadState states[4][4] =
	{
		{ auto_uncomp, mipped_uncomp, normal_uncomp, normal_uncomp },
		{ auto_comp,   mipped_comp,   normal_comp,   normal_comp   },
		{ broken_comp, mipped_comp,   normal_comp,   normal_comp   },
		{ glubuild,    mipped_uncomp, normal_uncomp, normal_uncomp }
	};
	const int row = auto_mipmap_gen ? (is_s3tc          ? 1 : 0) : (is_s3tc          ? 2 : 3);
	const int col = need_mipmaps    ? (includes_mipmaps ? 1 : 0) : (includes_mipmaps ? 2 : 3);
	return states[row][col];
}


// <data> holds the image as well as all mip levels (stored consecutively).
// upload them with the given internal format and quality flags.
//
// this is called whenever possible because pregenerated mipmaps are
// higher-quality and faster than gluBuildMipmaps resp. automatic generation.
//
// pre: w, h > 0; texture is bound.
static void upload_mipmaps(uint w, uint h, uint bpp, const u8* data,
	UploadState state, GLenum fmt, GLint int_fmt, uint q_flags)
{
	GLsizei level_w = w;
	GLsizei level_h = h;

	// resolution reduction (see OGL_TEX_HALF_RES for rationale).
	// this effectively reduces resolution by skipping some of the
	// lower (high-resolution) mip levels.
	//
	// we iterate through the loop (necessary to skip over image data),
	// but do not actually upload until the requisite number of
	// levels have been skipped (i.e. level == 0).
	//
	// note: we don't just use GL_TEXTURE_BASE_LEVEL because it would
	// require uploading unused levels, which is wasteful.
	// .. can be expanded to reduce to 1/4, 1/8 by encoding factor in q_flags.
	const uint reduce = (q_flags & OGL_TEX_HALF_RES)? 2 : 1;
	const uint levels_to_skip = log2(reduce);
	int level = -(int)levels_to_skip;

	// until at level 1x1:
	for(;;)
	{
		GLsizei mip_size;	// used to skip past this mip level in <data>
		if(state == mipped_uncomp)
		{
			mip_size = level_w * level_h * bpp/8;
			if(level >= 0)
				glTexImage2D(GL_TEXTURE_2D, level, int_fmt, level_w, level_h, 0, fmt, GL_UNSIGNED_BYTE, data);
		}
		else // state == mipped_comp
		{
			mip_size = (GLsizei)(round_up(level_w, 4) * round_up(level_h, 4) * bpp/8);
			if(level >= 0)
				glCompressedTexImage2DARB(GL_TEXTURE_2D, level, fmt, level_w, level_h, 0, mip_size, data);
		}
		data += mip_size;

		// 1x1 reached - done
		if(level_w == 1 && level_h == 1)
			break;
		level_w /= 2;
		level_h /= 2;
		// if the texture is non-square, one of the dimensions will become
		// 0 before the other. to satisfy OpenGL's expectations, change it
		// back to 1.
		if(level_w == 0) level_w = 1;
		if(level_h == 0) level_h = 1;
		level++;
	}
}


// upload manager: given all texture format details, determines the
// initial "state" and runs through the upload automaton.
//
// split out of ogl_tex_upload because it was too big.
//
// pre: <t> is valid for OpenGL use; texture is bound.
static void upload_impl(const Tex* t, GLenum fmt, GLint int_fmt, GLint filter, uint q_flags)
{
	// convenient local copies (t has been validated already).
	const GLsizei w  = (GLsizei)t->w;
	const GLsizei h  = (GLsizei)t->h;
	const uint bpp   = t->bpp;		// used for S3TC/mipmap size calc
	const uint flags = t->flags;	// tells us if img holds mipmaps
	const u8* data = (const u8*)tex_get_data(t);

	UploadState state = determine_upload_state(fmt, filter, flags);

	if(state == auto_uncomp || state == auto_comp)
	{
		// notes:
		// - if this state is reached, OpenGL supports auto mipmap gen
		//   (we check for that above)
		// - we assume GL_GENERATE_MIPMAP and GL_GENERATE_MIPMAP_SGIS
		//   have the same values - it's heavily implied by the spec
		//   governing 'promoted' ARB extensions and just plain makes sense.
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		state = (state == auto_comp)? normal_comp : normal_uncomp;
	}

	if(state == broken_comp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		state = normal_comp;
	}

	if(state == glubuild)
		gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, w, h, fmt, GL_UNSIGNED_BYTE, data);
	else if(state == normal_uncomp)
		glTexImage2D(GL_TEXTURE_2D, 0, int_fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
	else if(state == normal_comp)
	{
		const GLsizei tex_size = w * h * bpp / 8;
		glCompressedTexImage2DARB(GL_TEXTURE_2D, 0, fmt, w, h, 0, tex_size, data);
	}
	else if(state == mipped_uncomp || state == mipped_comp)
		upload_mipmaps(w, h, bpp, data, state, fmt, int_fmt, q_flags);
	else
		debug_warn("invalid state in ogl_tex_upload");

}


// upload the texture to OpenGL.
// if q_flags_ovr != 0, it overrides the default quality vs. perf. flags;
// if (int_)fmt_over != 0, it overrides the texture loader's decision.
// side effects:
// - enables texturing on TMU 0 and binds the texture to it;
// - frees the texel data! see ogl_tex_get_data.
int ogl_tex_upload(const Handle ht, uint q_flags_ovr, GLint int_fmt_ovr, GLenum fmt_ovr)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	debug_assert(q_flags_valid(q_flags_ovr));
	// we don't bother verifying *fmt_ovr - there are too many values

	const char* fn = h_filename(ht);
	if(!fn)
	{
		fn = "(could not determine filename)";
		debug_warn("ogl_tex_upload(Handle): h_filename failed");
	}

	// someone's requesting upload, but has already been uploaded.
	// this happens if a cached texture is "loaded". no work to do.
	if(ot->is_currently_uploaded)
		return 0;

	// determine fmt and int_fmt, allowing for user override.
	if(q_flags_ovr) ot->q_flags = q_flags_ovr;
	CHECK_ERR(get_gl_fmt(ot->t.bpp, ot->t.flags, ot->q_flags, &ot->fmt, &ot->int_fmt));
	if(int_fmt_ovr) ot->int_fmt = int_fmt_ovr;
	if(fmt_ovr) ot->fmt = fmt_ovr;

	// we know ht is valid (H_DEREF above), but ogl_tex_bind can
	// fail in debug builds if OglTex.id isn't a valid texture name
	CHECK_ERR(ogl_tex_bind(ht, ot->tmu));

	// if first time: apply our defaults/previous overrides;
	// otherwise, replays all state changes.
	state_latch(&ot->state);

	upload_impl(&ot->t, ot->fmt, ot->int_fmt, ot->state.filter, ot->q_flags);

	// see rationale for <refs> at declaration of OglTex.
	// note: tex_free is safe even if this OglTex was wrapped -
	//       the Tex contains a mem handle.
	int refs = h_get_refcnt(ht);
	if(refs == 1)
		tex_free(&ot->t);

	ot->has_been_uploaded = 1;
	ot->is_currently_uploaded = 1;

#ifndef NDEBUG
	oglCheck();
#endif
	return 0;
}


//----------------------------------------------------------------------------
// getters

// retrieve texture dimensions and bits per pixel.
// all params are optional and filled if non-NULL.
int ogl_tex_get_size(Handle ht, uint* w, uint* h, uint* bpp)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	if(w)
		*w = ot->t.w;
	if(h)
		*h = ot->t.h;
	if(bpp)
		*bpp = ot->t.bpp;
	return 0;
}


// retrieve Tex.flags and the corresponding OpenGL format.
// the latter is determined during ogl_tex_upload and is 0 before that.
// all params are optional and filled if non-NULL.
int ogl_tex_get_format(Handle ht, uint* flags, GLenum* fmt)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	if(flags)
		*flags = ot->t.flags;
	if(fmt)
	{
		if(!ot->has_been_uploaded)
			debug_warn("ogl_tex_get_format: hasn't been defined yet!");
		*fmt = ot->fmt;
	}
	return 0;
}


// retrieve pointer to texel data.
//
// note: this memory is freed after a successful ogl_tex_upload for
// this texture. after that, the pointer we retrieve is NULL but 	ps_dbg.exe!ogl_tex_set_filter(__int64 ht=476741374144, int filter=9729)  Line 490 + 0x4a	C++

// the function doesn't fail (negative return value) by design.
// if you still need to get at the data, add a reference before
// uploading it or read directly from OpenGL (discouraged).
int ogl_tex_get_data(Handle ht, void** p)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);

	*p = tex_get_data(&ot->t);
	return 0;
}


//----------------------------------------------------------------------------
// misc API

// apply the specified transforms (as in tex_transform) to the image.
// must be called before uploading (raises a warning if called afterwards).
int ogl_tex_transform(Handle ht, uint transforms)
{
	H_DEREF(ht, OglTex, ot);
	CHECK_OGL_TEX(ot);
	int ret = tex_transform(&ot->t, transforms);
	return ret;
}