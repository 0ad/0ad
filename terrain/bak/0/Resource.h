//***********************************************************
//
// Name:		Resource.H
// Last Update: 7/2/02
// Author:		Poya Manouchehri
//
// Description: A game resource provides an interface for a 
//				game resource type, ie ModelDefs, Bitmap and
//				Textures, Sounds and Music. These can be
//				accessed through a ResourceLibrary.
//				IMPORTANT NOTE: This is an abstract class. It
//				Must ONLY instantiated with a child class. 
//
//***********************************************************

#ifndef RESOURCE_H
#define RESOURCE_H

#include "Types.H"

#define MAX_RSNAME_LENGTH	(64)

enum RESOURCETYPE
{
	RST_BITMAP,
	RST_TEXTURE,
	RST_CUBETEXTURE,
	RST_MODELDEF,
	RST_SOUND,
	RST_VERTEXSHADER,
	RST_PIXELSHADER,
};

class CResource
{
	public:
		virtual ~CResource() {};

		char *GetName() { return m_Name; }
		int GetType() { return m_Type; }

	protected:
		char m_Name[MAX_RSNAME_LENGTH];
		unsigned int m_Type;
};

#endif