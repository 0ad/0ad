#include <cassert>
#include <cstring>
#include <cstdio>

#include "wsdl.h"
#include "ogl.h"

#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#endif


// define extension function pointers
extern "C"
{
#define FUNC(ret, name, params) ret (__stdcall *name) params;
#include "glext_funcs.h"
#undef FUNC
}


static const char* exts;


// check if the extension <ext> is supported by the OpenGL implementation
bool oglExtAvail(const char* ext)
{
	assert(exts && "call oglInit before using this function");

	const char *p = exts, *end;

	// make sure ext is valid & doesn't contain spaces
	if(!ext || ext == '\0' || strchr(ext, ' '))
		return false;

	for(;;)
	{
		p = strstr(p, ext);
		if(!p)
			return false; // <ext> string not found - extension not supported
		end = p + strlen(ext); // end of current substring

		// make sure the substring found is an entire extension string,
		// i.e. it starts and ends with ' '
		if(p == exts || *(p-1) == ' ')		// valid start?
			if(*end == ' ' || *end == '\0') // valid end?
				return true;
		p = end;
	}
}


void oglPrintErrors()
{
#define E(e) case e: printf("%s\n", #e); break;

	for(;;)
		switch(glGetError())
		{
		case GL_NO_ERROR:
			return;
E(GL_INVALID_ENUM)
E(GL_INVALID_VALUE)
E(GL_INVALID_OPERATION)
E(GL_STACK_OVERFLOW)
E(GL_STACK_UNDERFLOW)
E(GL_OUT_OF_MEMORY)
		}
}


int max_tex_size;				// [pixels]
int tex_units;
int max_VAR_elements = -1;		// GF2: 64K; GF3: 1M
bool tex_compression_avail;		// S3TC / DXT{1,3,5}
int video_mem;					// [MB]; approximate

#include "time.h"

// call after each video mode change
void oglInit()
{
	exts = (const char*)glGetString(GL_EXTENSIONS);

	// import functions
#define FUNC(ret, name, params) *(void**)&name = SDL_GL_GetProcAddress(#name);
#include "glext_funcs.h"
#undef FUNC

	// detect OpenGL / graphics card caps

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &tex_units);
	// make sure value is -1 if not supported
	if(oglExtAvail("GL_NV_vertex_array_range"))
		glGetIntegerv(GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV, &max_VAR_elements);

	tex_compression_avail = oglExtAvail("GL_ARB_texture_compression") &&
						   (oglExtAvail("GL_EXT_texture_compression_s3tc") || oglExtAvail("GL_S3_s3tc"));

	video_mem = (SDL_GetVideoInfo()->video_mem) / 1048576;	// [MB]
	// TODO: add sizeof(FB)?
}
