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
 * wrapper for all OpenGL texturing calls. provides caching, hotloading
 * and lifetime management.
 */

#include "precompiled.h"
#include "ogl_tex.h"

#include <cstdio>

#include "lib/app_hooks.h"
#include "lib/ogl.h"
#include "lib/bits.h"
#include "lib/sysdep/gfx.h"
#include "lib/tex/tex.h"

#include "lib/res/h_mgr.h"
#include "lib/fnv_hash.h"


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
#if !CONFIG2_GLES
	case GL_CLAMP:
	case GL_CLAMP_TO_BORDER:
#endif
	case GL_CLAMP_TO_EDGE:
	case GL_REPEAT:
	case GL_MIRRORED_REPEAT:
		return true;
	default:
		return false;
	}
}


static bool are_mipmaps_needed(size_t width, size_t height, GLint filter)
{
	// can't upload the entire texture; we're going to skip some
	// levels until it no longer exceeds the OpenGL dimension limit.
	if((GLint)width > ogl_max_tex_size || (GLint)height > ogl_max_tex_size)
		return true;

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


static bool fmt_is_s3tc(GLenum fmt)
{
	switch(fmt)
	{
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
#if !CONFIG2_GLES
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
#endif
		return true;
	default:
		return false;
	}
}


// determine OpenGL texture format, given <bpp> and Tex <flags>.
static GLint choose_fmt(size_t bpp, size_t flags)
{
	const bool alpha = (flags & TEX_ALPHA) != 0;
	const bool bgr   = (flags & TEX_BGR  ) != 0;
	const bool grey  = (flags & TEX_GREY ) != 0;
	const size_t dxt   = flags & TEX_DXT;

	// S3TC
	if(dxt != 0)
	{
		switch(dxt)
		{
		case DXT1A:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case 1:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
#if !CONFIG2_GLES
		case 3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case 5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#endif
		default:
			DEBUG_WARN_ERR(ERR::LOGIC);	// invalid DXT value
			return 0;
		}
	}

	// uncompressed
	switch(bpp)
	{
	case 8:
		ENSURE(grey);
		return GL_LUMINANCE;
	case 16:
		return GL_LUMINANCE_ALPHA;
	case 24:
		ENSURE(!alpha);
#if CONFIG2_GLES
		// GLES never supports BGR
		ENSURE(!bgr);
		return GL_RGB;
#else
		return bgr? GL_BGR : GL_RGB;
#endif
	case 32:
		ENSURE(alpha);
		// GLES can support BGRA via GL_EXT_texture_format_BGRA8888
		// (TODO: can we rely on support for that extension?)
		return bgr? GL_BGRA_EXT : GL_RGBA;
	default:
		DEBUG_WARN_ERR(ERR::LOGIC);	// invalid bpp
		return 0;
	}

	UNREACHABLE;
}


//----------------------------------------------------------------------------
// quality mechanism
//----------------------------------------------------------------------------

static GLint default_filter = GL_LINEAR;	// one of the GL *minify* filters
static int default_q_flags = OGL_TEX_FULL_QUALITY;	// OglTexQualityFlags

static bool q_flags_valid(int q_flags)
{
	const size_t bits = OGL_TEX_FULL_QUALITY|OGL_TEX_HALF_BPP|OGL_TEX_HALF_RES;
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
void ogl_tex_set_defaults(int q_flags, GLint filter)
{
	if(q_flags)
	{
		ENSURE(q_flags_valid(q_flags));
		default_q_flags = q_flags;
	}

	if(filter)
	{
		ENSURE(filter_valid(filter));
		default_filter = filter;
	}
}


// choose an internal format for <fmt> based on the given q_flags.
static GLint choose_int_fmt(GLenum fmt, int q_flags)
{
	// true => 4 bits per component; otherwise, 8
	const bool half_bpp = (q_flags & OGL_TEX_HALF_BPP) != 0;

	// early-out for S3TC textures: they don't need an internal format
	// (because upload is via glCompressedTexImage2DARB), but we must avoid
	// triggering the default case below. we might as well return a
	// meaningful value (i.e. int_fmt = fmt).
	if(fmt_is_s3tc(fmt))
		return fmt;

#if CONFIG2_GLES

	UNUSED2(half_bpp);

	// GLES only supports internal format == external format
	return fmt;

#else

	switch(fmt)
	{
	// 8bpp
	case GL_LUMINANCE:
		return half_bpp? GL_LUMINANCE4 : GL_LUMINANCE8;
	case GL_INTENSITY:
		return half_bpp? GL_INTENSITY4 : GL_INTENSITY8;
	case GL_ALPHA:
		return half_bpp? GL_ALPHA4 : GL_ALPHA8;

	// 16bpp
	case GL_LUMINANCE_ALPHA:
		return half_bpp? GL_LUMINANCE4_ALPHA4 : GL_LUMINANCE8_ALPHA8;

	// 24bpp
	case GL_RGB:
	case GL_BGR:	// note: BGR can't be used as internal format
		return half_bpp? GL_RGB4 : GL_RGB8;

	// 32bpp
	case GL_RGBA:
	case GL_BGRA:	// note: BGRA can't be used as internal format
		return half_bpp? GL_RGBA4 : GL_RGBA8;

	default:
		{
		wchar_t buf[100];
		swprintf_s(buf, ARRAY_SIZE(buf), L"choose_int_fmt: fmt 0x%x isn't covered! please add it", fmt);
		DEBUG_DISPLAY_ERROR(buf);
		DEBUG_WARN_ERR(ERR::LOGIC);	// given fmt isn't covered! please add it.
		// fall back to a reasonable default
		return half_bpp? GL_RGB4 : GL_RGB8;
		}
	}

	UNREACHABLE;

#endif	// #if CONFIG2_GLES
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
	GLint wrap_s;
	GLint wrap_t;
	// .. anisotropy
	//    note: ignored unless EXT_texture_filter_anisotropic is supported.
	GLfloat anisotropy;
};


// fill the given state object with default values.
static void state_set_to_defaults(OglTexState* ots)
{
	ots->filter = default_filter;
	ots->wrap_s = GL_REPEAT;
	ots->wrap_t = GL_REPEAT;
	ots->anisotropy = 1.0f;
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
	const GLint wrap_s = ots->wrap_s;
	const GLint wrap_t = ots->wrap_t;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
	// .. only CLAMP and REPEAT are guaranteed to be available.
	//    if we're using one of the others, we squelch the error that
	//    may have resulted if this GL implementation is old.
#if !CONFIG2_GLES
	if((wrap_s != GL_CLAMP && wrap_s != GL_REPEAT) || (wrap_t != GL_CLAMP && wrap_t != GL_REPEAT))
		ogl_SquelchError(GL_INVALID_ENUM);
#endif

	// anisotropy
	const GLfloat anisotropy = ots->anisotropy;
	if (anisotropy != 1.0f && ogl_tex_has_anisotropy())
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}
}


//----------------------------------------------------------------------------
// texture resource object
//----------------------------------------------------------------------------

// ideally we would split OglTex into data and state objects as in
// SndData / VSrc. this gives us the benefits of caching while still
// leaving each "instance" (state object, which owns a data reference)
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

// note: make sure these values fit inside OglTex.flags (only 16 bits)
enum OglTexFlags
{
	// "the texture is currently uploaded"; reset in dtor.
	OT_IS_UPLOADED = 1,

	// "the enclosed Tex object is valid and holds a texture";
	// reset in dtor and after ogl_tex_upload's tex_free.
	OT_TEX_VALID = 2,
	//size_t tex_valid : 1;

	// "reload() should automatically re-upload the texture" (because
	// it had been uploaded before the reload); never reset.
	OT_NEED_AUTO_UPLOAD = 4,

	// (used for validating flags)
	OT_ALL_FLAGS = OT_IS_UPLOADED|OT_TEX_VALID|OT_NEED_AUTO_UPLOAD
};

struct OglTex
{
	Tex t;

	// allocated by OglTex_reload; indicates the texture is currently uploaded.
	GLuint id;

	// ogl_tex_upload calls choose_fmt to determine these from <t>.
	// however, its caller may override those values via parameters.
	// note: these are stored here to allow retrieving via ogl_tex_get_format;
	// they are only used within ogl_tex_upload.
	GLenum fmt;
	GLint int_fmt;

	OglTexState state;

	// OglTexQualityFlags
	u8 q_flags;

	// to which Texture Mapping Unit was this bound?
	u8 tmu;

	u16 flags;

	u32 uploaded_size;
};

H_TYPE_DEFINE(OglTex);

static void OglTex_init(OglTex* ot, va_list args)
{
	Tex* wrapped_tex = va_arg(args, Tex*);
	if(wrapped_tex)
	{
		ot->t = *wrapped_tex;
		// indicate ot->t is now valid, thus skipping loading from file.
		// note: ogl_tex_wrap prevents actual reloads from happening.
		ot->flags |= OT_TEX_VALID;
	}

	state_set_to_defaults(&ot->state);
	ot->q_flags = default_q_flags;
}

static void OglTex_dtor(OglTex* ot)
{
	if(ot->flags & OT_TEX_VALID)
	{
		ot->t.free();
		ot->flags &= ~OT_TEX_VALID;
	}

	// note: do not check if OT_IS_UPLOADED is set, because we allocate
	// OglTex.id without necessarily having done an upload.
	glDeleteTextures(1, &ot->id);
	ot->id = 0;
	ot->flags &= ~OT_IS_UPLOADED;
	ot->uploaded_size = 0;
}

static Status OglTex_reload(OglTex* ot, const PIVFS& vfs, const VfsPath& pathname, Handle h)
{
	// we're reusing a freed but still in-memory OglTex object
	if(ot->flags & OT_IS_UPLOADED)
		return INFO::OK;

	// if we don't already have the texture in memory (*), load from file.
	// * this happens if the texture is "wrapped".
	if(!(ot->flags & OT_TEX_VALID))
	{
		shared_ptr<u8> file; size_t fileSize;
		RETURN_STATUS_IF_ERR(vfs->LoadFile(pathname, file, fileSize));
		if(ot->t.decode(file, fileSize) >= 0)
			ot->flags |= OT_TEX_VALID;
	}

	glGenTextures(1, &ot->id);

	// if it had already been uploaded before this reload,
	// re-upload it (this also does state_latch).
	if(ot->flags & OT_NEED_AUTO_UPLOAD)
		(void)ogl_tex_upload(h);

	return INFO::OK;
}

static Status OglTex_validate(const OglTex* ot)
{
	if(ot->flags & OT_TEX_VALID)
	{
		RETURN_STATUS_IF_ERR(ot->t.validate());

		// width, height
		// (note: this is done here because tex.cpp doesn't impose any
		// restrictions on dimensions, while OpenGL does).
		size_t w = ot->t.m_Width;
		size_t h = ot->t.m_Height;
		// .. == 0; texture file probably not loaded successfully.
		if(w == 0 || h == 0)
			WARN_RETURN(ERR::_11);
		// .. not power-of-2.
		//    note: we can't work around this because both NV_texture_rectangle
		//    and subtexture require work for the client (changing tex coords).
		//    TODO: ARB_texture_non_power_of_two
		if(!is_pow2(w) || !is_pow2(h))
			WARN_RETURN(ERR::_13);

		// no longer verify dimensions are less than ogl_max_tex_size,
		// because we just use the higher mip levels in that case.
	}

	// texture state
	if(!filter_valid(ot->state.filter))
		WARN_RETURN(ERR::_14);
	if(!wrap_valid(ot->state.wrap_s))
		WARN_RETURN(ERR::_15);
	if(!wrap_valid(ot->state.wrap_t))
		WARN_RETURN(ERR::_16);

	// misc
	if(!q_flags_valid(ot->q_flags))
		WARN_RETURN(ERR::_17);
	if(ot->tmu >= 128)	// unexpected that there will ever be this many
		WARN_RETURN(ERR::_18);
	if(ot->flags > OT_ALL_FLAGS)
		WARN_RETURN(ERR::_19);
	// .. note: don't check ot->fmt and ot->int_fmt - they aren't set
	//    until during ogl_tex_upload.

	return INFO::OK;
}

static Status OglTex_to_string(const OglTex* ot, wchar_t* buf)
{
	swprintf_s(buf, H_STRING_LEN, L"OglTex id=%u flags=%x", ot->id, (unsigned int)ot->flags);
	return INFO::OK;
}


// load and return a handle to the texture given in <pathname>.
// for a list of supported formats, see tex.h's tex_load.
Handle ogl_tex_load(const PIVFS& vfs, const VfsPath& pathname, size_t flags)
{
	Tex* wrapped_tex = 0;	// we're loading from file
	return h_alloc(H_OglTex, vfs, pathname, flags, wrapped_tex);
}


// return Handle to an existing object, if it has been loaded and
// is still in memory; otherwise, a negative error code.
Handle ogl_tex_find(const VfsPath& pathname)
{
	const uintptr_t key = fnv_hash(pathname.string().c_str(), pathname.string().length()*sizeof(pathname.string()[0]));
	return h_find(H_OglTex, key);
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
// note: because we cannot guarantee that callers will pass distinct
// "filenames", caching is disabled for the created object. this avoids
// mistakenly reusing previous objects that share the same comment.
Handle ogl_tex_wrap(Tex* t, const PIVFS& vfs, const VfsPath& pathname, size_t flags)
{
	// this object may not be backed by a file ("may", because
	// someone could do tex_load and then ogl_tex_wrap).
	// if h_mgr asks for a reload, the dtor will be called but
	// we won't be able to reconstruct it. therefore, disallow reloads.
	// (they are improbable anyway since caller is supposed to pass a
	// 'descriptive comment' instead of filename, but don't rely on that)
	// also disable caching as explained above.
	flags |= RES_DISALLOW_RELOAD|RES_NO_CACHE;
	return h_alloc(H_OglTex, vfs, pathname, flags, t);
}


// free all resources associated with the texture and make further
// use of it impossible. (subject to refcount)
Status ogl_tex_free(Handle& ht)
{
	return h_free(ht, H_OglTex);
}


//----------------------------------------------------------------------------
// state setters (see "Texture Parameters" in docs)
//----------------------------------------------------------------------------

// we require the below functions be called before uploading; this avoids
// potentially redundant glTexParameter calls (we'd otherwise need to always
// set defaults because we don't know if an override is forthcoming).

// raise a debug warning if the texture has already been uploaded
// (except in the few cases where this is allowed; see below).
// this is so that you will notice incorrect usage - only one instance of a
// texture should be active at a time, because otherwise they vie for
// control of one shared OglTexState.
static void warn_if_uploaded(Handle ht, const OglTex* ot)
{
#ifndef NDEBUG
	// we do not require users of this module to remember if they've
	// already uploaded a texture (inconvenient). since they also can't
	// tell if the texture was newly loaded (due to h_alloc interface),
	// we have to squelch this warning in 2 cases:
	// - it's ogl_tex_loaded several times (i.e. refcount > 1) and the
	//   caller (typically a higher-level LoadTexture) is setting filter etc.
	// - caller is using our Handle as a caching mechanism, and calls
	//   ogl_tex_set_* before every use of the texture. note: this
	//   need not fall under the above check, e.g. if freed but cached.
	//   workaround is that ogl_tex_set_* won't call us if the
	//   same state values are being set (harmless anyway).
	intptr_t refs = h_get_refcnt(ht);
	if(refs > 1)
		return;	// don't complain

	if(ot->flags & OT_IS_UPLOADED)
		DEBUG_WARN_ERR(ERR::LOGIC);	// ogl_tex_set_*: texture already uploaded and shouldn't be changed
#else
	// (prevent warnings; the alternative of wrapping all call sites in
	// #ifndef is worse)
	UNUSED2(ht);
	UNUSED2(ot);
#endif
}


// override default filter (as set above) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// filter is as defined by OpenGL; it is applied for both minification and
// magnification (for rationale and details, see OglTexState)
Status ogl_tex_set_filter(Handle ht, GLint filter)
{
	H_DEREF(ht, OglTex, ot);

	if(!filter_valid(filter))
		WARN_RETURN(ERR::INVALID_PARAM);

	if(ot->state.filter != filter)
	{
		warn_if_uploaded(ht, ot);
		ot->state.filter = filter;
	}
	return INFO::OK;
}


// override default wrap mode (GL_REPEAT) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// wrap is as defined by OpenGL.
Status ogl_tex_set_wrap(Handle ht, GLint wrap_s, GLint wrap_t)
{
	H_DEREF(ht, OglTex, ot);

	if(!wrap_valid(wrap_s))
		WARN_RETURN(ERR::INVALID_PARAM);

	if(!wrap_valid(wrap_t))
		WARN_RETURN(ERR::INVALID_PARAM);

	if(ot->state.wrap_s != wrap_s || ot->state.wrap_t != wrap_t)
	{
		warn_if_uploaded(ht, ot);
		ot->state.wrap_s = wrap_s;
		ot->state.wrap_t = wrap_t;
	}
	return INFO::OK;
}


// override default anisotropy for this texture.
// must be called before uploading (raises a warning if called afterwards).
Status ogl_tex_set_anisotropy(Handle ht, GLfloat anisotropy)
{
	H_DEREF(ht, OglTex, ot);

	if(anisotropy < 1.0f)
		WARN_RETURN(ERR::INVALID_PARAM);

	if(ot->state.anisotropy != anisotropy)
	{
		warn_if_uploaded(ht, ot);
		ot->state.anisotropy = anisotropy;
	}
	return INFO::OK;
}


//----------------------------------------------------------------------------
// upload
//----------------------------------------------------------------------------

// OpenGL has several features that are helpful for uploading but not
// available in all implementations. we check for their presence but
// provide for user override (in case they don't work on a card/driver
// combo we didn't test).

// tristate; -1 is undecided
static int have_auto_mipmap_gen = -1;
static int have_s3tc = -1;
static int have_anistropy = -1;

// override the default decision and force/disallow use of the
// given feature. should be called from ah_override_gl_upload_caps.
void ogl_tex_override(OglTexOverrides what, OglTexAllow allow)
{
	ENSURE(allow == OGL_TEX_ENABLE || allow == OGL_TEX_DISABLE);
	const bool enable = (allow == OGL_TEX_ENABLE);

	switch(what)
	{
	case OGL_TEX_S3TC:
		have_s3tc = enable;
		break;
	case OGL_TEX_AUTO_MIPMAP_GEN:
		have_auto_mipmap_gen = enable;
		break;
	case OGL_TEX_ANISOTROPY:
		have_anistropy = enable;
		break;
	default:
		DEBUG_WARN_ERR(ERR::LOGIC);	// invalid <what>
		break;
	}
}


// detect caps (via OpenGL extension list) and give an app_hook the chance to
// override this (e.g. via list of card/driver combos on which S3TC breaks).
// called once from the first ogl_tex_upload.
static void detect_gl_upload_caps()
{
	// note: gfx_card will be empty if running in quickstart mode;
	// in that case, we won't be able to check for known faulty cards.

	// detect features, but only change the variables if they were at
	// "undecided" (if overrides were set before this, they must remain).
	if(have_auto_mipmap_gen == -1)
	{
		have_auto_mipmap_gen = ogl_HaveExtension("GL_SGIS_generate_mipmap");
	}
	if(have_s3tc == -1)
	{
#if CONFIG2_GLES
		// some GLES implementations have GL_EXT_texture_compression_dxt1
		// but that only supports DXT1 so we can't use it anyway
		have_s3tc = 0;
#else
		// note: we don't bother checking for GL_S3_s3tc - it is incompatible
		// and irrelevant (was never widespread).
		have_s3tc = ogl_HaveExtensions(0, "GL_ARB_texture_compression", "GL_EXT_texture_compression_s3tc", NULL) == 0;
#endif
	}
	if(have_anistropy == -1)
	{
		have_anistropy = ogl_HaveExtension("GL_EXT_texture_filter_anisotropic");
	}

	// allow app hook to make ogl_tex_override calls
	if(AH_IS_DEFINED(override_gl_upload_caps))
	{
		ah_override_gl_upload_caps();
	}
	// no app hook defined - have our own crack at blacklisting some hardware.
	else
	{
		const std::wstring cardName = gfx::CardName();
		// rationale: janwas's laptop's S3 card blows up if S3TC is used
		// (oh, the irony). it'd be annoying to have to share this between all
		// projects, hence this default implementation here.
		if(cardName == L"S3 SuperSavage/IXC 1014")
			ogl_tex_override(OGL_TEX_S3TC, OGL_TEX_DISABLE);
	}
}


// take care of mipmaps. if they are called for by <filter>, either
// arrange for OpenGL to create them, or see to it that the Tex object
// contains them (if need be, creating them in software).
// sets *plevels_to_skip to influence upload behavior (depending on
// whether mipmaps are needed and the quality settings).
// returns 0 to indicate success; otherwise, caller must disable
// mipmapping by switching filter to e.g. GL_LINEAR.
static Status get_mipmaps(Tex* t, GLint filter, int q_flags, int* plevels_to_skip)
{
	// decisions:
	// .. does filter call for uploading mipmaps?
	const bool need_mipmaps = are_mipmaps_needed(t->m_Width, t->m_Height, filter);
	// .. does the image data include mipmaps? (stored as separate
	//    images after the regular texels)
	const bool includes_mipmaps = (t->m_Flags & TEX_MIPMAPS) != 0;
	// .. is this texture in S3TC format? (more generally, "compressed")
	const bool is_s3tc = (t->m_Flags & TEX_DXT) != 0;

	*plevels_to_skip = TEX_BASE_LEVEL_ONLY;
	if(!need_mipmaps)
		return INFO::OK;

	// image already contains pregenerated mipmaps; we need do nothing.
	// this is the nicest case, because they are fastest to load
	// (no extra processing needed) and typically filtered better than
	// if automatically generated.
	if(includes_mipmaps)
		*plevels_to_skip = 0;	// t contains mipmaps
	// OpenGL supports automatic generation; we need only
	// activate that and upload the base image.
#if !CONFIG2_GLES
	else if(have_auto_mipmap_gen)
	{
		// note: we assume GL_GENERATE_MIPMAP and GL_GENERATE_MIPMAP_SGIS
		// have the same values - it's heavily implied by the spec
		// governing 'promoted' ARB extensions and just plain makes sense.
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
#endif
	// image is S3TC-compressed and the previous 2 alternatives weren't
	// available; we're going to cheat and just disable mipmapping.
	// rationale: having tex_transform add mipmaps would be slow (since
	// all<->all transforms aren't implemented, it'd have to decompress
	// from S3TC first), and DDS images ought to include mipmaps!
	else if(is_s3tc)
		return ERR::FAIL;	// NOWARN
	// image is uncompressed and we're on an old OpenGL implementation;
	// we will generate mipmaps in software.
	else
	{
		RETURN_STATUS_IF_ERR(t->transform_to(t->m_Flags|TEX_MIPMAPS));
		*plevels_to_skip = 0;	// t contains mipmaps
	}

	// t contains mipmaps; we can apply our resolution reduction trick:
	if(*plevels_to_skip == 0)
	{
		// if OpenGL's texture dimension limit is too small, use the
		// higher mipmap levels. NB: the minimum guaranteed size is
		// far too low, and menu background textures may be large.
		GLint w = (GLint)t->m_Width, h = (GLint)t->m_Height;
		while(w > ogl_max_tex_size || h > ogl_max_tex_size)
		{
			(*plevels_to_skip)++;
			w /= 2; h /= 2;	// doesn't matter if either dimension drops to 0
		}

		// this saves texture memory by skipping some of the lower
		// (high-resolution) mip levels.
		//
		// note: we don't just use GL_TEXTURE_BASE_LEVEL because it would
		// require uploading unused levels, which is wasteful.
		// .. can be expanded to reduce to 1/4, 1/8 by encoding factor in q_flags.
		if(q_flags & OGL_TEX_HALF_RES)
			(*plevels_to_skip)++;
	}

	return INFO::OK;
}


// tex_util_foreach_mipmap callbacks: upload the given level to OpenGL.

struct UploadParams
{
	GLenum fmt;
	GLint int_fmt;
	u32* uploaded_size;
};

static void upload_level(size_t level, size_t level_w, size_t level_h, const u8* RESTRICT level_data, size_t level_data_size, void* RESTRICT cbData)
{
	const UploadParams* up = (const UploadParams*)cbData;
	glTexImage2D(GL_TEXTURE_2D, (GLint)level, up->int_fmt, (GLsizei)level_w, (GLsizei)level_h, 0, up->fmt, GL_UNSIGNED_BYTE, level_data);
	*up->uploaded_size += (u32)level_data_size;
}

static void upload_compressed_level(size_t level, size_t level_w, size_t level_h, const u8* RESTRICT level_data, size_t level_data_size, void* RESTRICT cbData)
{
	const UploadParams* up = (const UploadParams*)cbData;
	pglCompressedTexImage2DARB(GL_TEXTURE_2D, (GLint)level, up->fmt, (GLsizei)level_w, (GLsizei)level_h, 0, (GLsizei)level_data_size, level_data);
	*up->uploaded_size += (u32)level_data_size;
}

// upload the texture in the specified (internal) format.
// split out of ogl_tex_upload because it was too big.
//
// pre: <t> is valid for OpenGL use; texture is bound.
static void upload_impl(Tex* t, GLenum fmt, GLint int_fmt, int levels_to_skip, u32* uploaded_size)
{
	const GLsizei w  = (GLsizei)t->m_Width;
	const GLsizei h  = (GLsizei)t->m_Height;
	const size_t bpp   = t->m_Bpp;
	const u8* data = (const u8*)t->get_data();
	const UploadParams up = { fmt, int_fmt, uploaded_size };

	if(t->m_Flags & TEX_DXT)
		tex_util_foreach_mipmap(w, h, bpp, data, levels_to_skip, 4, upload_compressed_level, (void*)&up);
	else
		tex_util_foreach_mipmap(w, h, bpp, data, levels_to_skip, 1, upload_level, (void*)&up);
}


// upload the texture to OpenGL.
// if not 0, parameters override the following:
//   fmt_ovr     : OpenGL format (e.g. GL_RGB) decided from bpp / Tex flags;
//   q_flags_ovr : global default "quality vs. performance" flags;
//   int_fmt_ovr : internal format (e.g. GL_RGB8) decided from fmt / q_flags.
//
// side effects:
// - enables texturing on TMU 0 and binds the texture to it;
// - frees the texel data! see ogl_tex_get_data.
Status ogl_tex_upload(const Handle ht, GLenum fmt_ovr, int q_flags_ovr, GLint int_fmt_ovr)
{
	ONCE(detect_gl_upload_caps());

	H_DEREF(ht, OglTex, ot);
	Tex* t = &ot->t;
	ENSURE(q_flags_valid(q_flags_ovr));
	// we don't bother verifying *fmt_ovr - there are too many values

	// upload already happened; no work to do.
	// (this also happens if a cached texture is "loaded")
	if(ot->flags & OT_IS_UPLOADED)
		return INFO::OK;

	if(ot->flags & OT_TEX_VALID)
	{
		// decompress S3TC if that's not supported by OpenGL.
		if((t->m_Flags & TEX_DXT) && !have_s3tc)
			(void)t->transform_to(t->m_Flags & ~TEX_DXT);

		// determine fmt and int_fmt, allowing for user override.
		ot->fmt = choose_fmt(t->m_Bpp, t->m_Flags);
		if(fmt_ovr) ot->fmt = fmt_ovr;
		if(q_flags_ovr) ot->q_flags = q_flags_ovr;
		ot->int_fmt = choose_int_fmt(ot->fmt, ot->q_flags);
		if(int_fmt_ovr) ot->int_fmt = int_fmt_ovr;

		ot->uploaded_size = 0;

		// now actually send to OpenGL:
		ogl_WarnIfError();
		{
			// (note: we know ht is valid due to H_DEREF, but ogl_tex_bind can
			// fail in debug builds if OglTex.id isn't a valid texture name)
			RETURN_STATUS_IF_ERR(ogl_tex_bind(ht, ot->tmu));
			int levels_to_skip;
			if(get_mipmaps(t, ot->state.filter, ot->q_flags, &levels_to_skip) < 0)
				// error => disable mipmapping
				ot->state.filter = GL_LINEAR;
			// (note: if first time, applies our defaults/previous overrides;
			// otherwise, replays all state changes)
			state_latch(&ot->state);
			upload_impl(t, ot->fmt, ot->int_fmt, levels_to_skip, &ot->uploaded_size);
		}
		ogl_WarnIfError();

		ot->flags |= OT_IS_UPLOADED;

		// see rationale for <refs> at declaration of OglTex.
		intptr_t refs = h_get_refcnt(ht);
		if(refs == 1)
		{
			// note: we verify above that OT_TEX_VALID is set
			ot->flags &= ~OT_TEX_VALID;
		}
	}

	ot->flags |= OT_NEED_AUTO_UPLOAD;

	return INFO::OK;
}


//----------------------------------------------------------------------------
// getters
//----------------------------------------------------------------------------

// retrieve texture dimensions and bits per pixel.
// all params are optional and filled if non-NULL.
Status ogl_tex_get_size(Handle ht, size_t* w, size_t* h, size_t* bpp)
{
	H_DEREF(ht, OglTex, ot);

	if(w)
		*w = ot->t.m_Width;
	if(h)
		*h = ot->t.m_Height;
	if(bpp)
		*bpp = ot->t.m_Bpp;
	return INFO::OK;
}


// retrieve TexFlags and the corresponding OpenGL format.
// the latter is determined during ogl_tex_upload and is 0 before that.
// all params are optional and filled if non-NULL.
Status ogl_tex_get_format(Handle ht, size_t* flags, GLenum* fmt)
{
	H_DEREF(ht, OglTex, ot);

	if(flags)
		*flags = ot->t.m_Flags;
	if(fmt)
	{
		ENSURE(ot->flags & OT_IS_UPLOADED);
		*fmt = ot->fmt;
	}
	return INFO::OK;
}


// retrieve pointer to texel data.
//
// note: this memory is freed after a successful ogl_tex_upload for
// this texture. after that, the pointer we retrieve is NULL but
// the function doesn't fail (negative return value) by design.
// if you still need to get at the data, add a reference before
// uploading it or read directly from OpenGL (discouraged).
Status ogl_tex_get_data(Handle ht, u8** p)
{
	H_DEREF(ht, OglTex, ot);

	*p = ot->t.get_data();
	return INFO::OK;
}

Status ogl_tex_get_uploaded_size(Handle ht, size_t* size)
{
	H_DEREF(ht, OglTex, ot);

	*size = ot->uploaded_size;
	return INFO::OK;
}

// retrieve colour of 1x1 mipmap level
extern Status ogl_tex_get_average_colour(Handle ht, u32* p)
{
	H_DEREF(ht, OglTex, ot);
	warn_if_uploaded(ht, ot);

	*p = ot->t.get_average_colour();
	return INFO::OK;
}

//----------------------------------------------------------------------------
// misc API
//----------------------------------------------------------------------------

// bind the texture to the specified unit [number] in preparation for
// using it in rendering. if <ht> is 0, texturing is disabled instead.
//
// side effects:
// - changes the active texture unit;
// - (if return value is 0:) texturing was enabled/disabled on that unit.
//
// notes:
// - assumes multitexturing is available.
// - not necessary before calling ogl_tex_upload!
// - on error, the unit's texture state is unchanged; see implementation.
Status ogl_tex_bind(Handle ht, size_t unit)
{
	// note: there are many call sites of glActiveTextureARB, so caching
	// those and ignoring redundant sets isn't feasible.
	pglActiveTextureARB((int)(GL_TEXTURE0+unit));

	// special case: disable texturing
	if(ht == 0)
	{
		glDisable(GL_TEXTURE_2D);
		return INFO::OK;
	}

	// if this fails, the texture unit's state remains unchanged.
	// we don't bother catching that and disabling texturing because a
	// debug warning is raised anyway, and it's quite unlikely.
	H_DEREF(ht, OglTex, ot);
	ot->tmu = (u8)unit;

	// if 0, there's a problem in the OglTex reload/dtor logic.
	// binding it results in whiteness, which can have many causes;
	// we therefore complain so this one can be ruled out.
	ENSURE(ot->id != 0);

#if !CONFIG2_GLES
	glEnable(GL_TEXTURE_2D);
#endif
	glBindTexture(GL_TEXTURE_2D, ot->id);
	return INFO::OK;
}

Status ogl_tex_get_texture_id(Handle ht, GLuint* id)
{
	*id = 0;
	H_DEREF(ht, OglTex, ot);
	*id = ot->id;
	return INFO::OK;
}

// apply the specified transforms (as in tex_transform) to the image.
// must be called before uploading (raises a warning if called afterwards).
Status ogl_tex_transform(Handle ht, size_t transforms)
{
	H_DEREF(ht, OglTex, ot);
	Status ret = ot->t.transform(transforms);
	return ret;
}


// change the pixel format to that specified by <new_flags>.
// (note: this is equivalent to ogl_tex_transform(ht, ht_flags^new_flags).
Status ogl_tex_transform_to(Handle ht, size_t new_flags)
{
	H_DEREF(ht, OglTex, ot);
	Status ret = ot->t.transform_to(new_flags);
	return ret;
}


// return whether native S3TC support is available
bool ogl_tex_has_s3tc()
{
	// ogl_tex_upload must be called before this
	ENSURE(have_s3tc != -1);

	return (have_s3tc != 0);
}

// return whether anisotropic filtering support is available
bool ogl_tex_has_anisotropy()
{
	// ogl_tex_upload must be called before this
	ENSURE(have_anistropy != -1);

	return (have_anistropy != 0);
}
