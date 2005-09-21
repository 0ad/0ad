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


Caching and Texture Instances
-----------------------------

Caching is both an advantage and drawback. When opening the same
texture twice without previously freeing it, a reference to the
first instance is returned. Therefore, be advised that concurrent use of the
same texture but with differing parameters (e.g. upload quality) followed by 
a reload of the first instance will result in using the wrong parameters.
For background and rationale why this is acceptable, see struct OglTex.


Uploading to OpenGL
-------------------

.. deserves some clarification. This entails calling glTexImage2D
(or its variants for mipmaps/compressed textures) and transfers
texture parameters and data from system memory to OpenGL
(and thereby usually video memory).

In so doing, choices are made as to filtering algorithm and the
texture's internal representation (how it is stored in vmem) -
in particular, the bit depth. This can trade performance
(more/less data to copy) for quality (fidelity to original).

We provide a mechanism that applies defaults to all uploads unless
overrides are specified (for individual ogl_tex_upload calls).
This allows a global "quality" setting that can boost performance on
older graphics cards without requiring anything else to be changed.


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


2) Advanced usage: wrap existing texture data and specify
   filter/internal_format overrides.
Tex t;
const uint flags = 0;	// image is plain RGB, default orientation
void* data = [pre-existing image]
(void)tex_wrap(w, h, 24, flags, data, &t);
Handle hCompositeAlphaMap = ogl_tex_wrap(&t, "(alpha map composite)");
(void)ogl_tex_upload(hCompositeAlphaMap, GL_LINEAR, GL_INTENSITY);
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

// change default upload settings - these affect performance vs. quality.
// may be overridden for individual textures via ogl_tex_upload parameters.
// pass 0 to keep the current setting; defaults and legal values are:
// - filter: GL_LINEAR; any valid OpenGL minification filter
// - bpp   : 32; 16 or 32 (this toggles between RGBA4 and RGBA8)
extern void ogl_tex_set_default_upload(GLint filter, uint bpp);


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


// upload the texture to OpenGL. texture filter and [internal] format
// may be specified to override the global defaults (see below).
// side effects:
// - enables texturing on TMU 0 and binds the texture to it;
// - frees the texel data! see ogl_tex_get_data.
extern int ogl_tex_upload(Handle ht, GLint filter_override = 0, GLint internal_fmt_override = 0, GLenum format_override = 0);

// bind the texture to the specified unit [number] in preparation for
// using it in rendering. assumes multitexturing is available.
// side effects:
// - enables (or disables, if <ht> == 0) texturing on the given unit.
extern int ogl_tex_bind(Handle ht, GLenum unit = 0);


// retrieve texture dimensions and bits per pixel.
// all params are optional and filled if non-NULL.
extern int ogl_tex_get_size(Handle ht, int* w, int* h, int* bpp);

// retrieve Tex.flags and the corresponding OpenGL format.
// all params are optional and filled if non-NULL.
extern int ogl_tex_get_format(Handle ht, int* flags, GLenum* fmt);

// retrieve pointer to texel data.
//
// note: this memory is freed after a successful ogl_tex_upload for
// this texture. after that, the pointer we retrieve is NULL but
// the function doesn't fail (negative return value) by design.
// if you still need to get at the data, add a reference before
// uploading it or read directly from OpenGL (discouraged).
extern int ogl_tex_get_data(Handle ht, void** p);

#endif	// #ifndef OGL_TEX_H__
