//***********************************************************
//
// Name:		OGLTexture.Cpp
// Last Update:	14/2/02
// Author:		Poya Manouchehri
//
// Description: This class represents a OpenGL Texture.
//				When a CTexture gets registered with the
//				renderer, one of these is created and added
//				to the renderer's list of registered textures
//
//***********************************************************

#include "OGLTexture.H"

COGLTexture::COGLTexture ()
{
	m_TextureID = 0;
	m_pTextureBK = NULL;
}

COGLTexture::~COGLTexture ()
{
	glDeleteTextures (1, &m_TextureID);
	m_pTextureBK = NULL;
}

FRESULT COGLTexture::CreateTexture (CTexture *texture)
{
//	unsigned int *NewBits;

	//create and set the texture name
	glGenTextures (1, &m_TextureID);
	glBindTexture (GL_TEXTURE_2D, m_TextureID);

	if (!texture->GetMipMapFlag ())
	{
//		NewBits = new unsigned int [texture->GetWidth()*texture->GetHeight()];
		
		//resize the image for the next mip level
//		if (gluScaleImage (GL_RGBA,
//						   texture->GetWidth (),
//						   texture->GetHeight (),
//						   GL_UNSIGNED_INT,
//						   texture->GetBits (),
//						   texture->GetWidth (),
//						   texture->GetHeight (),
//						   GL_UNSIGNED_INT,
//						   NewBits) != 0)
//			return R_FAIL;

		//create the texture
		glTexImage2D (GL_TEXTURE_2D,
					  0,
					  4,
					  texture->GetWidth (),
					  texture->GetHeight (),
					  0,
					  GL_RGBA,
					  GL_UNSIGNED_BYTE,
					  texture->GetBits());
		
//		if (NewBits)
//			delete [] NewBits;
	}
	else
	{
		//build full mipmap table
		if (gluBuild2DMipmaps (GL_TEXTURE_2D,
							   4,
							   texture->GetWidth (),
							   texture->GetHeight (),
							   GL_RGBA,
							   GL_UNSIGNED_BYTE,
							   texture->GetBits()) != 0)
			return R_FAIL;
	}

	m_pTextureBK = texture;

	return R_OK;
}