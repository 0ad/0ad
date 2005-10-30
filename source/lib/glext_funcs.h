#include <GL/glext.h>
#if OS_WIN
# include <GL/wglext.h>
#endif

/*

FUNC is used for functions that are only extensions.
FUNC2 is used for functions that have been promoted to core features.

The FUNC2 call includes the version of OpenGL in which the extension was promoted,
and the pre- and post-promotion names (e.g. "glBindBufferARB" vs "glBindBuffer").

If the GL driver is advertising a sufficiently high version, we load the promoted
name; otherwise we use the *ARB name. (The spec says:
	"GL implementations of such later revisions should continue to export the name
	 strings of promoted extensions in the EXTENSIONS string, and continue to support
	 the ARB-affixed versions of functions and enumerants as a transition aid."
but some drivers might be stupid/buggy and fail to do that, so we don't just use
the ARB names unconditionally.)

The names are made accessible to engine code only via the ARB name, to make it
obvious that care must be taken (i.e. by being certain that the extension is
actually supported).

Function names are in the form that is used in the OpenGL and extensions
specifications, i.e. without the 'gl' prefix. The FUNC/FUNC2 macro add an
appropriate prefix ('pgl' to avoid conflict with the system's GL headers).

*/

// were these defined as real functions in gl.h already?

// GL_EXT_draw_range_elements / GL1.2:
FUNC2(void, DrawRangeElementsEXT, DrawRangeElements, "1.2", (GLenum, GLuint, GLuint, GLsizei, GLenum, GLvoid*))

// GL_ARB_multitexture / GL1.3:
FUNC2(void, MultiTexCoord2fARB, MultiTexCoord2f, "1.3", (int, float, float))
FUNC2(void, ActiveTextureARB, ActiveTexture, "1.3", (int))
FUNC2(void, ClientActiveTextureARB, ClientActiveTexture, "1.3", (int))

// GL_ARB_vertex_buffer_object / GL1.5:
FUNC2(void, BindBufferARB, BindBuffer, "1.5", (int target, GLuint buffer))
FUNC2(void, DeleteBuffersARB, DeleteBuffers, "1.5", (GLsizei n, const GLuint* buffers))
FUNC2(void, GenBuffersARB, GenBuffers, "1.5", (GLsizei n, GLuint* buffers))
FUNC2(bool, IsBufferARB, IsBuffer, "1.5", (GLuint buffer))
FUNC2(void, BufferDataARB, BufferData, "1.5", (int target, GLsizeiptrARB size, const void* data, int usage))
FUNC2(void, BufferSubDataARB, BufferSubData, "1.5", (int target, GLintptrARB offset, GLsizeiptrARB size, const void* data))
FUNC2(void, GetBufferSubDataARB, GetBufferSubData, "1.5", (int target, GLintptrARB offset, GLsizeiptrARB size, void* data))
FUNC2(void*, MapBufferARB, MapBuffer, "1.5", (int target, int access))
FUNC2(bool, UnmapBufferARB, UnmapBuffer, "1.5", (int target))
FUNC2(void, GetBufferParameterivARB, GetBufferParameteriv, "1.5", (int target, int pname, int* params))
FUNC2(void, GetBufferPointervARB, GetBufferPointerv, "1.5", (int target, int pname, void** params))

// GL_ARB_texture_compression / GL1.3
FUNC2(void, CompressedTexImage3DARB, CompressedTexImage3D, "1.3", (GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC2(void, CompressedTexImage2DARB, CompressedTexImage2D, "1.3", (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC2(void, CompressedTexImage1DARB, CompressedTexImage1D, "1.3", (GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid*))
FUNC2(void, CompressedTexSubImage3DARB, CompressedTexSubImage3D, "1.3", (GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC2(void, CompressedTexSubImage2DARB, CompressedTexSubImage2D, "1.3", (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC2(void, CompressedTexSubImage1DARB, CompressedTexSubImage1D, "1.3", (GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid*))
FUNC2(void, GetCompressedTexImageARB, GetCompressedTexImage, "1.3", (GLenum, GLint, GLvoid*))

// GL_ARB_shader_objects
FUNC2(void, DeleteObjectARB, DeleteObject, "2.0", (GLhandleARB obj))
FUNC2(GLhandleARB, GetHandleARB, GetHandle, "2.0", (GLenum pname))
FUNC2(void, DetachObjectARB, DetachObject, "2.0", (GLhandleARB containerObj, GLhandleARB attachedObj))
FUNC2(GLhandleARB, CreateShaderObjectARB, CreateShaderObject, "2.0", (GLenum shaderType))
FUNC2(void, ShaderSourceARB, ShaderSource, "2.0", (GLhandleARB shaderObj, GLsizei count, const char **string, const GLint *length))
FUNC2(void, CompileShaderARB, CompileShader, "2.0", (GLhandleARB shaderObj))
FUNC2(GLhandleARB, CreateProgramObjectARB, CreateProgramObject, "2.0", (void))
FUNC2(void, AttachObjectARB, AttachObject, "2.0", (GLhandleARB containerObj, GLhandleARB obj))
FUNC2(void, LinkProgramARB, LinkProgram, "2.0", (GLhandleARB programObj))
FUNC2(void, UseProgramObjectARB, UseProgramObject, "2.0", (GLhandleARB programObj))
FUNC2(void, ValidateProgramARB, ValidateProgram, "2.0", (GLhandleARB programObj))
FUNC2(void, Uniform1fARB, Uniform1f, "2.0", (GLint location, GLfloat v0))
FUNC2(void, Uniform2fARB, Uniform2f, "2.0", (GLint location, GLfloat v0, GLfloat v1))
FUNC2(void, Uniform3fARB, Uniform3f, "2.0", (GLint location, GLfloat v0, GLfloat v1, GLfloat v2))
FUNC2(void, Uniform4fARB, Uniform4f, "2.0", (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))
FUNC2(void, Uniform1iARB, Uniform1i, "2.0", (GLint location, GLint v0))
FUNC2(void, Uniform2iARB, Uniform2i, "2.0", (GLint location, GLint v0, GLint v1))
FUNC2(void, Uniform3iARB, Uniform3i, "2.0", (GLint location, GLint v0, GLint v1, GLint v2))
FUNC2(void, Uniform4iARB, Uniform4i, "2.0", (GLint location, GLint v0, GLint v1, GLint v2, GLint v3))
FUNC2(void, Uniform1fvARB, Uniform1fv, "2.0", (GLint location, GLsizei count, const GLfloat *value))
FUNC2(void, Uniform2fvARB, Uniform2fv, "2.0", (GLint location, GLsizei count, const GLfloat *value))
FUNC2(void, Uniform3fvARB, Uniform3fv, "2.0", (GLint location, GLsizei count, const GLfloat *value))
FUNC2(void, Uniform4fvARB, Uniform4fv, "2.0", (GLint location, GLsizei count, const GLfloat *value))
FUNC2(void, Uniform1ivARB, Uniform1iv, "2.0", (GLint location, GLsizei count, const GLint *value))
FUNC2(void, Uniform2ivARB, Uniform2iv, "2.0", (GLint location, GLsizei count, const GLint *value))
FUNC2(void, Uniform3ivARB, Uniform3iv, "2.0", (GLint location, GLsizei count, const GLint *value))
FUNC2(void, Uniform4ivARB, Uniform4iv, "2.0", (GLint location, GLsizei count, const GLint *value))
FUNC2(void, UniformMatrix2fvARB, UniformMatrix2fv, "2.0", (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
FUNC2(void, UniformMatrix3fvARB, UniformMatrix3fv, "2.0", (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
FUNC2(void, UniformMatrix4fvARB, UniformMatrix4fv, "2.0", (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value))
FUNC2(void, GetObjectParameterfvARB, GetObjectParameterfv, "2.0", (GLhandleARB obj, GLenum pname, GLfloat *params))
FUNC2(void, GetObjectParameterivARB, GetObjectParameteriv, "2.0", (GLhandleARB obj, GLenum pname, GLint *params))
FUNC2(void, GetInfoLogARB, GetInfoLog, "2.0", (GLhandleARB obj, GLsizei maxLength, GLsizei *length, char *infoLog))
FUNC2(void, GetAttachedObjectsARB, GetAttachedObjects, "2.0", (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj))
FUNC2(GLint, GetUniformLocationARB, GetUniformLocation, "2.0", (GLhandleARB programObj, const char *name))
FUNC2(void, GetActiveUniformARB, GetActiveUniform, "2.0", (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, char *name))
FUNC2(void, GetUniformfvARB, GetUniformfv, "2.0", (GLhandleARB programObj, GLint location, GLfloat *params))
FUNC2(void, GetUniformivARB, GetUniformiv, "2.0", (GLhandleARB programObj, GLint location, GLint *params))
FUNC2(void, GetShaderSourceARB, GetShaderSource, "2.0", (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source))

// GL_ARB_vertex_shader
FUNC2(void, VertexAttrib1fARB, VertexAttrib1f, "2.0", (GLuint index, GLfloat v0))
FUNC2(void, VertexAttrib1sARB, VertexAttrib1s, "2.0", (GLuint index, GLshort v0))
FUNC2(void, VertexAttrib1dARB, VertexAttrib1d, "2.0", (GLuint index, GLdouble v0))
FUNC2(void, VertexAttrib2fARB, VertexAttrib2f, "2.0", (GLuint index, GLfloat v0, GLfloat v1))
FUNC2(void, VertexAttrib2sARB, VertexAttrib2s, "2.0", (GLuint index, GLshort v0, GLshort v1))
FUNC2(void, VertexAttrib2dARB, VertexAttrib2d, "2.0", (GLuint index, GLdouble v0, GLdouble v1))
FUNC2(void, VertexAttrib3fARB, VertexAttrib3f, "2.0", (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2))
FUNC2(void, VertexAttrib3sARB, VertexAttrib3s, "2.0", (GLuint index, GLshort v0, GLshort v1, GLshort v2))
FUNC2(void, VertexAttrib3dARB, VertexAttrib3d, "2.0", (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2))
FUNC2(void, VertexAttrib4fARB, VertexAttrib4f, "2.0", (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3))
FUNC2(void, VertexAttrib4sARB, VertexAttrib4s, "2.0", (GLuint index, GLshort v0, GLshort v1, GLshort v2, GLshort v3))
FUNC2(void, VertexAttrib4dARB, VertexAttrib4d, "2.0", (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3))
FUNC2(void, VertexAttrib4NubARB, VertexAttrib4Nub, "2.0", (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w))
FUNC2(void, VertexAttrib1fvARB, VertexAttrib1fv, "2.0", (GLuint index, const GLfloat *v))
FUNC2(void, VertexAttrib1svARB, VertexAttrib1sv, "2.0", (GLuint index, const GLshort *v))
FUNC2(void, VertexAttrib1dvARB, VertexAttrib1dv, "2.0", (GLuint index, const GLdouble *v))
FUNC2(void, VertexAttrib2fvARB, VertexAttrib2fv, "2.0", (GLuint index, const GLfloat *v))
FUNC2(void, VertexAttrib2svARB, VertexAttrib2sv, "2.0", (GLuint index, const GLshort *v))
FUNC2(void, VertexAttrib2dvARB, VertexAttrib2dv, "2.0", (GLuint index, const GLdouble *v))
FUNC2(void, VertexAttrib3fvARB, VertexAttrib3fv, "2.0", (GLuint index, const GLfloat *v))
FUNC2(void, VertexAttrib3svARB, VertexAttrib3sv, "2.0", (GLuint index, const GLshort *v))
FUNC2(void, VertexAttrib3dvARB, VertexAttrib3dv, "2.0", (GLuint index, const GLdouble *v))
FUNC2(void, VertexAttrib4fvARB, VertexAttrib4fv, "2.0", (GLuint index, const GLfloat *v))
FUNC2(void, VertexAttrib4svARB, VertexAttrib4sv, "2.0", (GLuint index, const GLshort *v))
FUNC2(void, VertexAttrib4dvARB, VertexAttrib4dv, "2.0", (GLuint index, const GLdouble *v))
FUNC2(void, VertexAttrib4ivARB, VertexAttrib4iv, "2.0", (GLuint index, const GLint *v))
FUNC2(void, VertexAttrib4bvARB, VertexAttrib4bv, "2.0", (GLuint index, const GLbyte *v))
FUNC2(void, VertexAttrib4ubvARB, VertexAttrib4ubv, "2.0", (GLuint index, const GLubyte *v))
FUNC2(void, VertexAttrib4usvARB, VertexAttrib4usv, "2.0", (GLuint index, const GLushort *v))
FUNC2(void, VertexAttrib4uivARB, VertexAttrib4uiv, "2.0", (GLuint index, const GLuint *v))
FUNC2(void, VertexAttrib4NbvARB, VertexAttrib4Nbv, "2.0", (GLuint index, const GLbyte *v))
FUNC2(void, VertexAttrib4NsvARB, VertexAttrib4Nsv, "2.0", (GLuint index, const GLshort *v))
FUNC2(void, VertexAttrib4NivARB, VertexAttrib4Niv, "2.0", (GLuint index, const GLint *v))
FUNC2(void, VertexAttrib4NubvARB, VertexAttrib4Nubv, "2.0", (GLuint index, const GLubyte *v))
FUNC2(void, VertexAttrib4NusvARB, VertexAttrib4Nusv, "2.0", (GLuint index, const GLushort *v))
FUNC2(void, VertexAttrib4NuivARB, VertexAttrib4Nuiv, "2.0", (GLuint index, const GLuint *v))
FUNC2(void, VertexAttribPointerARB, VertexAttribPointer, "2.0", (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer))
FUNC2(void, EnableVertexAttribArrayARB, EnableVertexAttribArray, "2.0", (GLuint index))
FUNC2(void, DisableVertexAttribArrayARB, DisableVertexAttribArray, "2.0", (GLuint index))
FUNC2(void, BindAttribLocationARB, BindAttribLocation, "2.0", (GLhandleARB programObj, GLuint index, const char *name))
FUNC2(void, GetActiveAttribARB, GetActiveAttrib, "2.0", (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, int *size, GLenum *type, char *name))
FUNC2(GLint, GetAttribLocationARB, GetAttribLocation, "2.0", (GLhandleARB programObj, const char *name))
FUNC2(void, GetVertexAttribdvARB, GetVertexAttribdv, "2.0", (GLuint index, GLenum pname, GLdouble *params))
FUNC2(void, GetVertexAttribfvARB, GetVertexAttribfv, "2.0", (GLuint index, GLenum pname, GLfloat *params))
FUNC2(void, GetVertexAttribivARB, GetVertexAttribiv, "2.0", (GLuint index, GLenum pname, GLint *params))
FUNC2(void, GetVertexAttribPointervARB, GetVertexAttribPointerv, "2.0", (GLuint index, GLenum pname, void **pointer))

#if OS_WIN
// WGL_EXT_swap_control
FUNC(int, wglSwapIntervalEXT, (int))

// WGL_ARB_pbuffer
FUNC(HPBUFFERARB, wglCreatePbufferARB, (HDC, int, int, int, const int*))
FUNC(HDC, wglGetPbufferDCARB, (HPBUFFERARB))
FUNC(int, wglReleasePbufferDCARB, (HPBUFFERARB, HDC))
FUNC(int, wglDestroyPbufferARB, (HPBUFFERARB))
FUNC(int, wglQueryPbufferARB, (HPBUFFERARB, int, int*))

// GL_ARB_pixel_format
FUNC(int, wglGetPixelFormatAttribivARB, (HDC, int, int, unsigned int, const int*, int*))
FUNC(int, wglGetPixelFormatAttribfvARB, (HDC, int, int, unsigned int, const int*, float*))
FUNC(int, wglChoosePixelFormatARB, (HDC, const int *, const float*, unsigned int, int*, unsigned int*))
#endif // OS_WIN
