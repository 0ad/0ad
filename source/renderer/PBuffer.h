#ifndef _PBUFFER_H
#define _PBUFFER_H


extern void PBufferMakeCurrent();
extern void PBufferInit(int width,int height,int doublebuffer,int colorbits,int depthbits,int stencilbits,
		   int rendertextureformat=0,int rendertexturetarget=0,int havemipmaps=0);
extern void PBufferClose();
extern void PBufferMakeUncurrent();
extern int PBufferWidth();

#endif
