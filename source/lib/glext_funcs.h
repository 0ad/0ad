// were these defined as real functions in gl.h already?

#ifndef REAL_GL_1_2
FUNC(void, glMultiTexCoord2f, (int, float, float))
#endif

#ifndef REAL_GL_1_3
FUNC(void, glActiveTexture, (int))
#endif

// EXT_swap_control
FUNC(int, wglSwapIntervalEXT, (int))

// NV_vertex_array
FUNC(void, glVertexArrayRangeNV, (int, void*))
FUNC(void, glFlushVertexArrayRangeNV, ())
FUNC(void*, wglAllocateMemoryNV, (int, float, float, float))
FUNC(void, wglFreeMemoryNV, (void*))

// NV_fence
FUNC(void, glGenFencesNV, (int, unsigned int*))
FUNC(void, glDeleteFencesNV, (int, const unsigned int*))
FUNC(void, glSetFenceNV, (unsigned int, int))
FUNC(int, glTestFenceNV, (unsigned int))
FUNC(void, glFinishFenceNV, (unsigned int))
FUNC(int, glIsFenceNV, (unsigned int))
FUNC(void, glGetFenceivNV, (unsigned int, int, int*))

// ARB_vertex_array_object
FUNC(void, BindBufferARB, (int target, GLuint buffer))
FUNC(void, DeleteBuffersARB, (GLsizei n, const GLuint* buffers))
FUNC(void, GenBuffersARB, (GLsizei n, GLuint* buffers))
FUNC(bool, IsBufferARB, (GLuint buffer))
FUNC(void, BufferDataARB, (int target, GLsizeiptrARB size, const void* data, int usage))
FUNC(void, BufferSubDataARB, (int target, GLintptrARB offset, GLsizeiptrARB size, const void* data))
FUNC(void, GetBufferSubDataARB, (int target, GLintptrARB offset, GLsizeiptrARB size, void* data))
FUNC(void*, MapBufferARB, (int target, int access))
FUNC(bool, UnmapBufferARB, (int target))
FUNC(void, GetBufferParameterivARB, (int target, int pname, int* params))
FUNC(void, GetBufferPointervARB, (int target, int pname, void** params))



FUNC(void, glCompressedTexImage2DARB, (int, int, int, unsigned int, unsigned int, int, unsigned int, const void*))
FUNC(void, glCompressedTexSubImage2DARB, (int, int, int, int, unsigned int, int, int, unsigned int, const void*))
