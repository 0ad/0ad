#include "precompiled.h"

#include "pbuffer.h"

#include <string.h>

#ifdef _WIN32

// janwas: wgl-specific crap is in sysdep/win/wgl.h;
// function pointers are imported automagically

/*
#include <windows.h>
#include <gl/gl.h>
#include <gl/glext.h>
*/

#include "ogl.h"

static HDC lastDC;
static HGLRC lastGLRC; 

/*
static bool GotExts=false;
static PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB=0;
static PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB=0;
static PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB=0;
static PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB=0;
static PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB=0;
static PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB=0;
static PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB=0;
static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB=0;
*/

class PBuffer
{
public:
	enum { MAX_ATTRIBS = 32 };
	enum { MAX_PFORMATS = 256 };

    PBuffer();
    ~PBuffer();
    
	void makeCurrent();

	bool init(int width,int height,int doublebuffer,int colorbits,int depthbits,int stencilbits,
		   int rendertextureformat=0,int rendertexturetarget=0,int havemipmaps=0);
	void close();

	// return width,height of this pbuffer
	int GetWidth() const { return _width; }
	int GetHeight() const { return _height; }

private:
    HDC _hDC;		
    HGLRC _hGLRC;	
    HPBUFFERARB _hBuffer; 
    int _width;
    int _height;
	int _doublebuffer;
	int _colorbits;
	int _depthbits;
	int _stencilbits;
    int _rendertextureformat;
	int _rendertexturetarget;
	int _havemipmaps;
};

static PBuffer g_PBuffer;

PBuffer::PBuffer() 
	: _width(0), _height(0), _hDC(0), _hGLRC(0), _hBuffer(0)
{
}

PBuffer::~PBuffer()
{
	close();
}

void PBuffer::close()
{
    if (_hBuffer) {
        wglDeleteContext(_hGLRC);
        wglReleasePbufferDCARB(_hBuffer,_hDC);
        wglDestroyPbufferARB(_hBuffer);
		_hBuffer=0;
      }
}

/*
static void GetExts()
{
	wglCreatePbufferARB=(PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB=(PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB=(PFNWGLRELEASEPBUFFERDCARBPROC) wglGetProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB=(PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress("wglDestroyPbufferARB");
	wglQueryPbufferARB=(PFNWGLQUERYPBUFFERARBPROC) wglGetProcAddress("wglQueryPbufferARB");
	wglGetPixelFormatAttribivARB=(PFNWGLGETPIXELFORMATATTRIBIVARBPROC) wglGetProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribfvARB=(PFNWGLGETPIXELFORMATATTRIBFVARBPROC) wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	wglChoosePixelFormatARB=(PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress("wglChoosePixelFormatARB");
	GotExts=true;
}
*/

// This function actually does the creation of the p-buffer.
// It can only be called once a window has already been created.
bool PBuffer::init(int width,int height,int doublebuffer,int colorbits,int depthbits,int stencilbits,
				   int rendertextureformat,int rendertexturetarget,int havemipmaps)
{
/*
	if (!GotExts) {
		GetExts();
	}
*/

	// store requested parameters
	_width=width;
	_height=height;
	_doublebuffer=doublebuffer;
	_colorbits=colorbits;
	_depthbits=depthbits;
	_stencilbits=stencilbits;
    _rendertextureformat=rendertextureformat;
	_rendertexturetarget=rendertexturetarget;
	_havemipmaps=havemipmaps;
    
	HDC hdc = wglGetCurrentDC();
	HGLRC hglrc = wglGetCurrentContext();

	// build attributes
	int iattributes[2*PBuffer::MAX_ATTRIBS];
	memset(iattributes,0,sizeof(int)*2*MAX_ATTRIBS);
	float fattributes[2*PBuffer::MAX_ATTRIBS];
	memset(fattributes,0,sizeof(float)*2*MAX_ATTRIBS);

	int niattribs=0;
	
    // pixel format must be "p-buffer capable" ..
    iattributes[2*niattribs] = WGL_DRAW_TO_PBUFFER_ARB;
    iattributes[2*niattribs+1] = 1;
    niattribs++;

	// .. use RGBA rather than index mode ..
	iattributes[2*niattribs] = WGL_PIXEL_TYPE_ARB;
    iattributes[2*niattribs+1] = WGL_TYPE_RGBA_ARB;
    niattribs++;

	// .. setup double buffer ..
    iattributes[2*niattribs] = WGL_DOUBLE_BUFFER_ARB;
	iattributes[2*niattribs+1] = doublebuffer;
    niattribs++;

	// .. setup color and alpha bits ..
    iattributes[2*niattribs] = WGL_COLOR_BITS_ARB;
	iattributes[2*niattribs+1] = colorbits;
    niattribs++;
    iattributes[2*niattribs] = WGL_ALPHA_BITS_ARB;
    iattributes[2*niattribs+1] = (colorbits==32) ? 8 : 0;
    niattribs++;

	 // .. setup depth/stencil buffer ..
    iattributes[2*niattribs] = WGL_DEPTH_BITS_ARB;
    iattributes[2*niattribs+1] = depthbits;
    niattribs++;
    iattributes[2*niattribs] = WGL_STENCIL_BITS_ARB;
    iattributes[2*niattribs+1] = stencilbits;
    niattribs++;


	// .. must support opengl (doh ..)
    iattributes[2*niattribs] = WGL_SUPPORT_OPENGL_ARB;
    iattributes[2*niattribs+1] = 1;
    niattribs++;

	if (rendertextureformat!=0) {
		// pbuffer meant to be bound as an RGBA texture, so need a color plane
		iattributes[2*niattribs] = WGL_BIND_TO_TEXTURE_RGBA_ARB;
		iattributes[2*niattribs+1] = 1;
		niattribs++;
	}

    int pformat[MAX_PFORMATS];
    unsigned int nformats;
    if (!wglChoosePixelFormatARB(hdc,iattributes,fattributes,MAX_PFORMATS,pformat,&nformats)) {
        // LOG << "PBuffer creation failed:  wglChoosePixelFormatARB() returned 0\n";
        return false;
	}
	if (nformats<=0) {
        // LOG << "PBuffer creation failed:  couldn't find a suitable pixel format\n";
        return false;
	}

    // Create the p-buffer.
    niattribs=0;
	memset(iattributes,0,sizeof(int)*2*MAX_ATTRIBS);
	if (rendertextureformat!=0) {

		// set format
		iattributes[2*niattribs]=WGL_TEXTURE_FORMAT_ARB;
		iattributes[2*niattribs+1]=rendertextureformat;
		niattribs++;

		// set target
	    iattributes[2*niattribs]=WGL_TEXTURE_TARGET_ARB;
		iattributes[2*niattribs+1]=rendertexturetarget;
		niattribs++;

		// mipmaps ..
		iattributes[2*niattribs] = WGL_MIPMAP_TEXTURE_ARB;
		iattributes[2*niattribs+1] = havemipmaps;
		niattribs++;

		// ask to allocate the largest pbuffer it can, if it is
		// unable to allocate for the width and height
		iattributes[2*niattribs]=WGL_PBUFFER_LARGEST_ARB;
		iattributes[2*niattribs+1]=0;
		niattribs++;

	}

    _hBuffer=wglCreatePbufferARB(hdc,pformat[0],_width,_height,iattributes );
    if (!_hBuffer) {
		// LOG << "PBuffer creation failed: wglCreatePbufferARB returned 0\n";
		return false;
	}
     
    // Get the device context.
    _hDC=wglGetPbufferDCARB(_hBuffer);
    if (!_hDC) {
		// LOG << "PBuffer creation failed: wglGetPbufferDCARB returned 0\n";
        return false;
	}

    // Create a gl context for the p-buffer.
    _hGLRC=wglCreateContext(_hDC);
    if (!_hGLRC) {
		// LOG << "PBuffer creation failed: wglCreateContext returned 0\n";
        return false;
	}
	
	if(!wglShareLists(hglrc,_hGLRC)) {
		// LOG << "PBuffer creation failed: wglShareLists returned 0\n";
		return false;
	}
	
    // Determine the actual width and height we were able to create.
    wglQueryPbufferARB(_hBuffer,WGL_PBUFFER_WIDTH_ARB,&_width);
    wglQueryPbufferARB(_hBuffer,WGL_PBUFFER_HEIGHT_ARB,&_height);

	// success ..
	// LOG << "PBuffer created: " << _width << " x " << _height << "\n";

	// log out attributes of pbuffer's pixel format 
	int values[MAX_ATTRIBS];
	int iatr[MAX_ATTRIBS] = { WGL_PIXEL_TYPE_ARB, WGL_COLOR_BITS_ARB,
		WGL_RED_BITS_ARB, WGL_GREEN_BITS_ARB, WGL_BLUE_BITS_ARB,
		WGL_ALPHA_BITS_ARB, WGL_DEPTH_BITS_ARB, WGL_STENCIL_BITS_ARB, WGL_ACCUM_BITS_ARB,
		WGL_DOUBLE_BUFFER_ARB, WGL_SUPPORT_OPENGL_ARB, WGL_ACCELERATION_ARB, 
		WGL_BIND_TO_TEXTURE_RGBA_ARB, WGL_SAMPLE_BUFFERS_ARB, WGL_SAMPLES_ARB
	};
    
	if (wglGetPixelFormatAttribivARB(hdc,pformat[0],0,15,iatr,values)) {
		// LOG << "\nPBuffer pixelformat (" << pformat[0] << "):\n";

		if (values[0]==WGL_TYPE_COLORINDEX_ARB) {
			// LOG << "    pixel type = WGL_TYPE_COLORINDEX_ARB\n";
		} else if (values[0]==WGL_TYPE_RGBA_ARB) {
			// LOG << "    pixel type = WGL_TYPE_RGBA_ARB\n";
		} else {
			// LOG << "    pixel type = UNKNOWN\n";
		}

		// LOG << "    color bits = " << values[1] << "\n";
		// LOG << "        red    = " << values[2] << "\n";
		// LOG << "        green  = " << values[3] << "\n";
		// LOG << "        blue   = " << values[4] << "\n";
		// LOG << "        alpha  = " << values[5] << "\n";
		// LOG << "    depth bits = " << values[6] << "\n";
		// LOG << "    stencil bits = " << values[7] << "\n";
		// LOG << "    accum bits = " << values[8] << "\n";
		// LOG << "    doublebuffer = " << values[9] << "\n";
		// LOG << "    support opengl = " << values[10] << "\n";

		if (values[11]==WGL_FULL_ACCELERATION_ARB) {
			// LOG << "    acceleration = WGL_FULL_ACCELERATION_ARB\n";
		} else if (values[11]== WGL_GENERIC_ACCELERATION_ARB) {
			// LOG << "    acceleration = WGL_GENERIC_ACCELERATION_ARB\n";
		} else {
			// LOG << "    acceleration = UNKNOWN\n";
		}

		// LOG << "    bind to texture = " << values[12] << "\n\n";
		// LOG << "    multisample = " << values[13] << "\n";
		if (values[13]) {
			// LOG << "        sample buffers = " << values[14] << "\n\n";
		}
	}

	return true;
}

void PBuffer::makeCurrent()
{
	lastGLRC=wglGetCurrentContext();
	lastDC=wglGetCurrentDC();

	// check first for lost pbuffer
    int lost;
    wglQueryPbufferARB(_hBuffer,WGL_PBUFFER_LOST_ARB,&lost);

    if (lost) {
		// ack .. have to recreate it ..
        close();
        if (!init(_width,_height,_doublebuffer,_colorbits,_depthbits,_stencilbits,
			  _rendertextureformat,_rendertexturetarget,_havemipmaps)) {
			// oops .. couldn't rebuild it ..
			// LOG << "PBuffer lost; couldn't rebuild";
			return;
		}
	}

	if (!wglMakeCurrent(_hDC,_hGLRC)) {
		// LOG << "PBuffer::makeCurrent - failed to make pbuffer current";
	}
}

void PBufferMakeCurrent()
{
	g_PBuffer.makeCurrent();
}


bool isWGLExtensionSupported(const char *extension)
{
	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB=(PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");
	if (!wglGetExtensionsStringARB) {
		// no function to retrieve WGL extension : maybe worth looking in the regular extensions ..
		return false;
	}

	static const char *extensions = NULL;
	const char *start;
	char *where, *terminator;
   
	where = (char *) strchr(extension, ' ');
	if ((where) || (*extension == '\0'))
		return false;
   
	if (!extensions)
		extensions = (const char *) wglGetExtensionsStringARB(wglGetCurrentDC());
   
	start = extensions;
	for (;;) {
		where = (char *) strstr((const char *) start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return true;
			}
		}

		start = terminator;
	}

	return false;
}

void PBufferInit(int width,int height,int doublebuffer,int colorbits,int depthbits,int stencilbits,
		   int rendertextureformat,int rendertexturetarget,int havemipmaps)
{
	g_PBuffer.init(width,height,doublebuffer,colorbits,depthbits,stencilbits,
		rendertextureformat,rendertexturetarget,havemipmaps);
}

void PBufferClose()
{
	g_PBuffer.close();

}

void PBufferMakeUncurrent()
{
	wglMakeCurrent(lastDC,lastGLRC);
}

int PBufferWidth()
{
	return g_PBuffer.GetWidth();
}

#endif

bool PBufferQuery()
{
	if (!isWGLExtensionSupported("WGL_ARB_pbuffer")) return false;
	if (!isWGLExtensionSupported("WGL_ARB_pixel_format")) return false;
	return true;
}
