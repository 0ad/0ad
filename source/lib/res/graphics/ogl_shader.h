#ifndef OGL_SHADER_H__
#define OGL_SHADER_H__

#include "../handle.h"

#include "lib/types.h"
#include "lib/ogl.h"

/*
Encapsulate shader objects into handles, which transparently enables sharing
of shader source files between programs as well as reloading shaders at
runtime.

NOTE: Only use functions form this module after verifying that the required
extensions are available, or all bets are off.
*/

// Create, load and compile a shader object of the given type
// (e.g. GL_VERTEX_SHADER_ARB). The given file will be used as
// source code for the shader.
Handle ogl_shader_load(const char* fn, GLenum type);

// Free all resources associated with the given handle (subject
// to refcounting).
void ogl_shader_free(Handle& h);

// Attach a shader to the given OpenGL program.
// Returns 0 on success and a negative error code otherwise.
int ogl_shader_attach(GLhandleARB program, Handle& h);


/*
Encapsulate program objects into handles.
*/

// Load a program object based on the given XML file description.
// Shader objects are loaded and attached automatically.
Handle ogl_program_load(const char* fn);

// Free all resources associated with the given program handle.
void ogl_program_free(Handle& h);

// Activate the program (glUseProgramObjectARB).
// h may be 0, in which case program objects are disabled.
int ogl_program_use(Handle h);

// Query uniform information
GLint ogl_program_get_uniform_location(Handle h, const char* name);

#endif // OGL_SHADER_H__
