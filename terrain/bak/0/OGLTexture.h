//***********************************************************
//
// Name:		OGLTexture.H
// Last Update:	14/2/02
// Author:		Poya Manouchehri
//
// Description: This class represents a OpenGL Texture.
//				When a CTexture gets registered with the
//				renderer, one of these is created and added
//				to the renderer's list of registered textures
//
//***********************************************************

#ifndef OGLTEXTURE_H
#define OGLTEXTURE_H

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>

#include "Texture.H"

#define MAX_REG_TEXTURES (100)

class COGLTexture
{
	public:
		COGLTexture ();
		~COGLTexture ();

		GLuint	m_TextureID;
		
		//a back pointer to the CTexture which owns this
		//D3DTexture.
		CTexture			*m_pTextureBK;
		
		//create the D3DTexture from a normal CTexture
		FRESULT CreateTexture (CTexture *texture);
};

#endif