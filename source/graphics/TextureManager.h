#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include <vector>
#include "CStr.h"
#include "Singleton.h"
#include "TextureEntry.h"

// access to sole CTextureManager  object
#define g_TexMan CTextureManager ::GetSingleton()

///////////////////////////////////////////////////////////////////////////////////////////
// CTextureManager : manager class for all terrain texture objects
class CTextureManager : public Singleton<CTextureManager>
{
public:
	struct STextureType
	{
		// name of this texture type (derived from directory name)
		CStr m_Name;
		// index in parent array
		int m_Index;
		// list of textures of this type (found from the texture directory)
		std::vector<CTextureEntry*> m_Textures;
	};

public:
	// constructor, destructor
	CTextureManager();
	~CTextureManager();
	
	int LoadTerrainTextures();


	void AddTextureType(const char* name);

	CTextureEntry* FindTexture(const char* filename);
	CTextureEntry* FindTexture(Handle handle);
	CTextureEntry* AddTexture(const char* filename,int type);
	void DeleteTexture(CTextureEntry* entry);

	std::vector<STextureType> m_TerrainTextures;

private:
	void LoadTerrainTexturesImpl(int terraintype,const char* fileext);
	void BuildTerrainTypes();

};


#endif
