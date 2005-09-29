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

/*

[KEEP IN SYNC WITH WIKI!]

Introduction
------------

This module simplifies use of textures in OpenGL. It provides an
easy-to-use load/upload/bind/free API that completely replaces
direct access to OpenGL's texturing calls.

It basically wraps tex.cpp's texture info in a resource object
(see h_mgr.h) that maintains associated GL state and provides for
reference counting, caching, hotloading and safe access.


Texture Parameters
------------------

OpenGL textures are conditioned on several parameters including
filter and wrap mode. These are typically set once when the texture is
created, but must survive reloads (1). To that end, all state (2) is
set via ogl_tex_set_* (instead of direct glTexParameter calls) and
re-applied after a reload.

(1) the purpose of hotloading is to permit artists to see their changes
    in-game without having to restart the map. reloads where the
    texture looks different due to changed state are useless.

(2) currently only filter and wrap mode. no other glTexParameter
    settings are used ATM; if that changes, add to OglTexState.


Uploading to OpenGL
-------------------

.. deserves some clarification. This entails calling glTexImage2D
(or its variants for mipmaps/compressed textures) and transfers
texture parameters and data from system memory to OpenGL
(and thereby usually video memory).

In so doing, choices are made as to the texture's internal representation
(how it is stored in vmem) - in particular, the bit depth.
This can trade performance (more/less data to copy) for quality
(fidelity to original).

We provide a mechanism that applies defaults to all uploads;
this allows a global "quality" setting that can boost performance on
older graphics cards without requiring anything else to be changed.
Textures with specific quality needs can override this via
ogl_tex_set_* or ogl_tex_upload parameters.


Caching and Texture Instances
-----------------------------

Caching is both an advantage and drawback. When opening the same
texture twice without previously freeing it, a reference to the
first instance is returned. Therefore, be advised that concurrent use of the
same texture but with differing parameters (e.g. upload quality) followed by 
a reload of the first instance will result in using the wrong parameters.
For background and rationale why this is acceptable, see struct OglTex.


Example Usage
-------------

Note: to keep the examples simple, we leave out error handling by
ignoring all return values. Each function will still raise a warning
(assert) if it fails and passing e.g. invalid Handles will only cause
the next function to fail, but real apps should check and report errors.

1) Basic usage: load texture from file.

Handle hTexture = ogl_tex_load("filename.dds");
(void)ogl_tex_upload(hTexture);

[when rendering:]
(void)ogl_tex_bind(hTexture);
[.. do something with OpenGL that uses the currently bound texture]

[at exit:]
// (done automatically, but this avoids it showing up as a leak)
(void)ogl_tex_free(hTexture);


2) Advanced usage: wrap existing texture data, override filter,
   specify internal_format and use multitexturing.
Tex t;
const uint flags = 0;	// image is plain RGB, default orientation
void* data = [pre-existing image]
(void)tex_wrap(w, h, 24, flags, data, &t);
Handle hCompositeAlphaMap = ogl_tex_wrap(&t, "(alpha map composite)");
(void)ogl_tex_set_filter(hCompositeAlphaMap, GL_LINEAR);
(void)ogl_tex_upload(hCompositeAlphaMap, 0, 0, GL_INTENSITY);
// (your responsibility! tex_wrap attaches a reference but it is
// removed by ogl_tex_upload.)
free(data);

[when rendering:]
(void)ogl_tex_bind(hCompositeAlphaMap, 1);
[.. do something with OpenGL that uses the currently bound texture]

[at exit:]
// (done automatically, but this avoids it showing up as a leak)
(void)ogl_tex_free(hCompositeAlphaMap);

*/

#ifndef OGL_TEX_H__
#define OGL_TEX_H__

#include "../handle.h"
#include "lib/types.h"
#include "lib/ogl.h"
#include "tex.h"


//
// quality mechanism
//

enum OglTexQualityFlags
{
	// emphatically require full quality for this texture.
	// (q_flags are invalid if this is set together with any other bit)
	// rationale: the value 0 is used to indicate "use default flags" in
	// ogl_tex_upload and ogl_tex_set_defaults, so this is the only
	// way we can say "disregard default and do not reduce anything".
	OGL_TEX_FULL_QUALITY = 0x20,

	// store the texture at half the normal bit depth
	// (4 bits per pixel component, as opposed to 8).
	// this increases performance on older graphics cards due to
	// decreased size in vmem. it has no effect on
	// compressed textures because they have a fixed internal format.
	OGL_TEX_HALF_BPP = 0x10,

	// store the texture at half its original resolution.
	// this increases performance on older graphics cards due to
	// decreased size in vmem. 
	// this is useful for also reducing quality of compressed textures,
	// which are not affected by OGL_TEX_HALF_BPP.
	// currently only implemented for images that contain mipmaps
	// (otherwise, we'd have to resample, which is slow).
	// note: scaling down to 1/4, 1/8, .. is easily possible without
	// extra work, so we leave some bits free for that.
	OGL_TEX_HALF_RES = 0x01
};

// change default settings - these affect performance vs. quality.
// may be overridden for individual textures via parameter to
// ogl_tex_upload or ogl_tex_set_filter, respectively.
// 
// pass 0 to keep the current setting; defaults and legal values are:
// - q_flags: OGL_TEX_FULL_QUALITY; combination of OglTexQualityFlags 
// - filter: GL_LINEAR; any valid OpenGL minification filter
extern void ogl_tex_set_defaults(uint q_flags, GLint filter);


//
// open/close
//

// load and return a handle to the texture given in <fn>.
// for a list of supported formats, see tex.h's tex_load.
extern Handle ogl_tex_load(const char* fn, uint flags = 0);

// make the given Tex object ready for use as an OpenGL texture
// and return a handle to it. this will be as if its contents
// had been loaded by ogl_tex_load.
//
// we need only add bookkeeping information and "wrap" it in
// a resource object (accessed via Handle), hence the name.
//
// <fn> isn't strictly needed but should describe the texture so that
// h_filename will return a meaningful comment for debug purposes.
extern Handle ogl_tex_wrap(Tex* t, const char* fn = 0, uint flags = 0);

// free all resources associated with the texture and make further
// use of it impossible. (subject to refcount)
extern int ogl_tex_free(Handle& ht);


//
// set texture parameters
//

// these must be called before uploading; this simplifies
// things and avoids calling glTexParameter twice.

// override default filter (as set above) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// filter is as defined by OpenGL; it is applied for both minification and
// magnification (for rationale and details, see OglTexState)
extern int ogl_tex_set_filter(Handle ht, GLint filter);

// override default wrap mode (GL_REPEAT) for this texture.
// must be called before uploading (raises a warning if called afterwards).
// wrap is as defined by OpenGL and applies to both S and T coordinates
// (rationale: see OglTexState).
extern int ogl_tex_set_wrap(Handle ht, GLint wrap);


//
// upload
//

// bind the texture to the specified unit [number] in preparation for
// using it in rendering. assumes multitexturing is available.
// not necessary before calling ogl_tex_upload!
// side effects:
// - enables (or disables, if <ht> == 0) texturing on the given unit.
extern int ogl_tex_bind(Handle ht, GLenum unit = 0);

// upload the texture to OpenGL.
// if not 0, parameters override the following:
//   fmt_ovr     : OpenGL format (e.g. GL_RGB) decided from bpp / Tex flags;
//   q_flags_ovr : global default "quality vs. performance" flags;
//   int_fmt_ovr : internal format (e.g. GL_RGB8) decided from fmt / q_flags.
//
// side effects:
// - enables texturing on TMU 0 and binds the texture to it;
// - frees the texel data! see ogl_tex_get_data.
extern int ogl_tex_upload(const Handle ht, GLenum fmt_ovr = 0, uint q_flags_ovr = 0, GLint int_fmt_ovr = 0);


//
// return information about the texture
//

// retrieve texture dimensions and bits per pixel.
// all params are optional and filled if non-NULL.
extern int ogl_tex_get_size(Handle ht, uint* w, uint* h, uint* bpp);

// retrieve Tex.flags and the corresponding OpenGL format.
// the latter is determined during ogl_tex_upload and is 0 before that.
// all params are optional and filled if non-NULL.
extern int ogl_tex_get_format(Handle ht, uint* flags, GLenum* fmt);

// retrieve pointer to texel data.
//
// note: this memory is freed after a successful ogl_tex_upload for
// this texture. after that, the pointer we retrieve is NULL but
// the function doesn't fail (negative return value) by design.
// if you still need to get at the data, add a reference before
// uploading it or read directly from OpenGL (discouraged).
extern int ogl_tex_get_data(Handle ht, void** p);


//
// misc
//

// apply the specified transforms (as in tex_transform) to the image.
// must be called before uploading (raises a warning if called afterwards).
extern int ogl_tex_transform(Handle ht, uint flags);

#endif	// #ifndef OGL_TEX_H__
