/**
 * =========================================================================
 * File        : ogl_shader.h
 * Project     : 0 A.D.
 * Description : load and link together shaders; provides hotload support.
 * =========================================================================
 */

#ifndef INCLUDED_OGL_SHADER
#define INCLUDED_OGL_SHADER

#include "lib/res/handle.h"

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
 * @param pathname location of the file containing the shader's source code.
 * @param type e.g. GL_VERTEX_SHADER_ARB.
 **/
Handle ogl_shader_load(const VfsPath& pathname, GLenum type);

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
 * @param pathname XML file describing the program.
 *
 * note: Shader objects are loaded and attached automatically.
 **/
Handle ogl_program_load(const char* pathname);

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
