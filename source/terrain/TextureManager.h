#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include <vector>
#include "TextureEntry.h"
#include "CStr.h"

class CTextureManager 
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
	CTextureManager();

	void LoadTerrainTextures();


	void AddTextureType(const char* name);

	CTextureEntry* FindTexture(const char* filename);
	CTextureEntry* FindTexture(Handle handle);
	CTextureEntry* AddTexture(const char* filename,int type);
	void DeleteTexture(CTextureEntry* entry);

	std::vector<STextureType> m_TerrainTextures;

private:
	void LoadTerrainTextures(int terraintype,const char* fileext);
	void BuildTerrainTypes();

};

extern CTextureManager g_TexMan;


#endif
