//***********************************************************
//
// Name:		GameResource.H
// Last Update: 7/2/02
// Author:		Poya Manouchehri
//
// Description: A game resource provides an interface for a 
//				game resource type, ie ModelDefs, Bitmap and
//				Textures, Sounds and Music. These can be
//				accessed through a ResourceLibrary
//
//***********************************************************

#ifndef GAMERESOURCE_H
#define GAMERESOURCE_H

#include "Types.H"

enum RESOURCETYPE
{
	RST_BITMAP,
	RST_TEXTURE,
	RST_MODELDEF,
	RST_SOUND,
};

class CGameResource
{
	public:
		virtual FRESULT LoadResource (char *filename, RESOURCETYPE type);

		char *GetName() { return m_Name; }
		char *GetPath() { return m_Path; }
		int GetType() { return m_Type; }

	protected:
		char m_Name[MAX_NAME_LENGTH];
		char m_Path[MAX_PATH_LENGTH];
		int m_Type;
};

#endif