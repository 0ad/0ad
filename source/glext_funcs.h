/* GL 1.2 */
FUNC(void, glActiveTexture, (int))

/* EXT_swap_control */
FUNC(int, wglSwapIntervalEXT, (int))

/* NV_vertex_array */
FUNC(void, glVertexArrayRangeNV, (int, void*))
FUNC(void, glFlushVertexArrayRangeNV, ())
FUNC(void*, wglAllocateMemoryNV, (int, float, float, float))
FUNC(void, wglFreeMemoryNV, (void*))

/* NV_fence */
FUNC(void, glGenFencesNV, (int, unsigned int*))
FUNC(void, glDeleteFencesNV, (int, const unsigned int*))
FUNC(void, glSetFenceNV, (unsigned int, int))
FUNC(int, glTestFenceNV, (unsigned int))
FUNC(void, glFinishFenceNV, (unsigned int))
FUNC(int, glIsFenceNV, (unsigned int))
FUNC(void, glGetFenceivNV, (unsigned int, int, int*))



FUNC(void, glCompressedTexImage2DARB, (int, int, int, unsigned int, unsigned int, int, unsigned int, const void*))
FUNC(void, glCompressedTexSubImage2DARB, (int, int, int, int, unsigned int, int, int, unsigned int, const void*))
