//***********************************************************
//
// Name:		Texture.Cpp
// Author:		Poya Manouchehri
//
// Description: The texture class holds data about a texture,
//				and certain flags descibing the filtering used
//				for the texture. It must be registered with the
//				renderer before being used.
//
//***********************************************************

#include "Texture.H"

CTexture::CTexture ()
{
	m_MipMap = true;
	m_RegisterID = -1;
}

CTexture::~CTexture ()
{
	CBitmap::~CBitmap ();
}

FRESULT CTexture::LoadBitmap (char *path, RESOURCETYPE type)
{
	FRESULT result;
	//load the image
	result = CBitmap::LoadBitmap (path, type);

	if (result != R_OK)
		return result;

	int w = m_Width;
	int h = m_Height;

	//width must be a power of 2
	while (w > 1)
	{
		if (w%2 != 0)
		{
			DestroyData ();
			return R_FAIL;
		}

		w /= 2;
	}

	//height must be a power of 2
	while (h > 1)
	{
		if (h%2 != 0)
		{
			DestroyData ();
			return R_FAIL;
		}

		h /= 2;
	}

	return R_OK;
}

FRESULT CTexture::CreateBitmap (RESOURCETYPE type, char *name, int width, int height)
{
	//must be square
	if (width != height)
		return R_BADPARAMS;

	int w = width;
	int h = height;

	//width must be a power of 2
	while (w > 1)
	{
		if (w%2 != 0)
		{
			DestroyData ();
			return R_FAIL;
		}

		w /= 2;
	}

	//height must be a power of 2
	while (h > 1)
	{
		if (h%2 != 0)
		{
			DestroyData ();
			return R_FAIL;
		}

		h /= 2;
	}
	return CBitmap::CreateBitmap (type, name, width, height);
}