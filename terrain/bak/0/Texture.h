//***********************************************************
//
// Name:		Texture.H
// Author:		Poya Manouchehri
//
// Description: The texture class holds data about a texture,
//				and certain flags descibing the filtering used
//				for the texture. It must be registered with the
//				renderer before being used. The flags must be set
//				before registering for them to have an effect
//
//***********************************************************

#ifndef TEXTURE_H
#define TEXTURE_H

#include "Types.H"
#include "Bitmap.H"

class CTexture : public CBitmap
{
	public:
		CTexture ();
		virtual ~CTexture ();

		virtual FRESULT LoadBitmap (char *path, RESOURCETYPE type);
		virtual FRESULT CreateBitmap (RESOURCETYPE type, char *name, int width, int height);

		//set number of mip mapping flag
		void SetMipMapFlag (bool flag) { m_MipMap = flag; }
		int GetMipMapFlag () { return m_MipMap; }

		//registry stuff
		void SetRegisterID (int id) { m_RegisterID = id; }
		int GetRegisterID () { return m_RegisterID; }

	private:
		//An id which is given to the texture when registered.
		//It is equal to -1 if not registered
		int				m_RegisterID;

		//A full mipmap table is built when creating
		bool			m_MipMap;
};

#endif