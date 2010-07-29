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
 * load and link together shaders; provides hotload support.
 */

#ifndef INCLUDED_OGL_SHADER
#define INCLUDED_OGL_SHADER

#include "lib/res/handle.h"
#include "lib/file/vfs/vfs.h"

#include "lib/ogl.h"


namespace ERR
{
	const LibError SHDR_CREATE     = -120200;
	const LibError SHDR_COMPILE    = -120201;
	const LibError SHDR_NO_SHADER  = -120202;
	const LibError SHDR_LINK       = -120203;
	const LibError SHDR_NO_PROGRAM = -120204;
}

/*
Encapsulate shader objects into handles, which transparently enables sharing
of shader source files between programs as well as reloading shaders at
runtime.

NOTE: Only use functions form this module after verifying that the required
extensions are available, or all bets are off.
*/

/**
 * Create, load and compile a shader object.
 *
 * @param vfs
 * @param pathname Location of the file containing the shader's source code.
 * @param type Type e.g. GL_VERTEX_SHADER_ARB.
 **/
Handle ogl_shader_load(const PIVFS& vfs, const VfsPath& pathname, GLenum type);

/**
 * Free all resources associated with the given handle
 * (subject to reference counting).
 **/
void ogl_shader_free(Handle& h);

/**
 * Attach a shader to the given OpenGL program.
 **/
LibError ogl_shader_attach(GLhandleARB program, Handle& h);


/*
Encapsulate program objects into handles.
*/

/**
 * Load a program object.
 *
 * @param vfs
 * @param pathname XML file describing the program.
 *
 * note: Shader objects are loaded and attached automatically.
 **/
Handle ogl_program_load(const PIVFS& vfs, const VfsPath& pathname);

/**
 * Free all resources associated with the given program handle.
 * (subject to reference counting).
 **/
void ogl_program_free(Handle& h);

/**
 * Activate the program (glUseProgramObjectARB).
 *
 * @param h may be 0, in which case program objects are disabled.
 **/
LibError ogl_program_use(Handle h);

/**
 * Query uniform information
 **/
GLint ogl_program_get_uniform_location(Handle h, const char* name);

/**
 * Query vertex attribute information
 **/
GLint ogl_program_get_attrib_location(Handle h, const char* name);

#endif // INCLUDED_OGL_SHADER
