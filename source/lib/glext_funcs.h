#include <GL/glext.h>
#ifdef _WIN32
# include <GL/wglext.h>
#endif
/*
typedef void* HDC;
typedef void* HGLRC;
typedef void* HPBUFFERARB;
*/


// were these defined as real functions in gl.h already?

#ifndef REAL_GL_1_2
FUNC(void, glMultiTexCoord2f, (int, float, float))
FUNC(void, glDrawRangeElements,(GLenum,GLuint,GLuint,GLsizei,GLenum,GLvoid*))
#endif

#ifndef REAL_GL_1_3
FUNC(void, glActiveTexture, (int))
FUNC(void, glClientActiveTexture, (int))
#endif

// EXT_swap_control
FUNC(int, wglSwapIntervalEXT, (int))

// ARB_vertex_array_object
FUNC(void, glBindBufferARB, (int target, GLuint buffer))
FUNC(void, glDeleteBuffersARB, (GLsizei n, const GLuint* buffers))
FUNC(void, glGenBuffersARB, (GLsizei n, GLuint* buffers))
FUNC(bool, glIsBufferARB, (GLuint buffer))
FUNC(void, glBufferDataARB, (int target, GLsizeiptrARB size, const void* data, int usage))
FUNC(void, glBufferSubDataARB, (int target, GLintptrARB offset, GLsizeiptrARB size, const void* data))
FUNC(void, glGetBufferSubDataARB, (int target, GLintptrARB offset, GLsizeiptrARB size, void* data))
FUNC(void*, glMapBufferARB, (int target, int access))
FUNC(bool, glUnmapBufferARB, (int target))
FUNC(void, glGetBufferParameterivARB, (int target, int pname, int* params))
FUNC(void, glGetBufferPointervARB, (int target, int pname, void** params))

// ARB_pbuffer
#ifdef _WIN32
FUNC(HPBUFFERARB, wglCreatePbufferARB, (HDC, int, int, int, const int*))
FUNC(HDC, wglGetPbufferDCARB, (HPBUFFERARB))
FUNC(int, wglReleasePbufferDCARB, (HPBUFFERARB, HDC))
FUNC(int, wglDestroyPbufferARB, (HPBUFFERARB))
FUNC(int, wglQueryPbufferARB, (HPBUFFERARB, int, int*))

// ARB_pixel_format
FUNC(int, wglGetPixelFormatAttribivARB, (HDC, int, int, unsigned int, const int*, int*))
FUNC(int, wglGetPixelFormatAttribfvARB, (HDC, int, int, unsigned int, const int*, float*))
FUNC(int, wglChoosePixelFormatARB, (HDC, const int *, const float*, unsigned int, int*, unsigned int*))
#endif // _WIN32

// ARB_texture_compression
FUNC(void, glCompressedTexImage3DARB, (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC(void, glCompressedTexImage2DARB, (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC(void, glCompressedTexImage1DARB, (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC(void, glCompressedTexSubImage3DARB, (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC(void, glCompressedTexSubImage2DARB, (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC(void, glCompressedTexSubImage1DARB, (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC(void, glGetCompressedTexImageARB, (GLenum, GLint, GLvoid*))
