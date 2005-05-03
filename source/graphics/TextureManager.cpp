#include "precompiled.h"

#include <algorithm>

#include "TextureManager.h"
#include "res/res.h"
#include "ogl.h"
#include "res/ogl_tex.h"
#include "CLogger.h"
#include "timer.h"

#define LOG_CATEGORY "graphics"

// filter for vfs_next_dirent
static const char* SupportedTextureFormats[] = { "*.png", "*.dds", "*.tga", "*.bmp" };



CTextureManager::CTextureManager()
{
	m_TerrainTextures.reserve(32);
}

CTextureManager::~CTextureManager()
{
	for (size_t i=0;i<m_TerrainTextures.size();i++) {
		for (size_t j=0;j<m_TerrainTextures[i].m_Textures.size();j++) {
			delete m_TerrainTextures[i].m_Textures[j];
		}
	}
}

void CTextureManager::AddTextureType(const char* name)
{
	m_TerrainTextures.resize(m_TerrainTextures.size()+1);
	STextureType& ttype=m_TerrainTextures.back();
	ttype.m_Name=name;
	ttype.m_Index=(int)m_TerrainTextures.size()-1;
}

CTextureEntry* CTextureManager::FindTexture(const char* filename)
{
	// check if file already loaded
	for (uint k=0;k<m_TerrainTextures.size();k++) {
		STextureType& ttype=m_TerrainTextures[k];
		for (uint i=0;i<ttype.m_Textures.size();i++) {
			if (strcmp((const char*) ttype.m_Textures[i]->GetName(),filename)==0) {
				return ttype.m_Textures[i];
			}
		}
	}

	return 0;
}

CTextureEntry* CTextureManager::FindTexture(Handle handle)
{
	for (uint k=0;k<m_TerrainTextures.size();k++) {
		STextureType& ttype=m_TerrainTextures[k];
		for (uint i=0;i<ttype.m_Textures.size();i++) {
			// Don't bother looking at textures that haven't been loaded yet - since
			// the caller has given us a Handle to the texture, it must be loaded.
			// (This matters because GetHandle would load the texture, even though
			// there's no need to.)
			if (ttype.m_Textures[i]->IsLoaded() && handle==ttype.m_Textures[i]->GetHandle()) {
				return ttype.m_Textures[i];
			}
		}
	}

	return 0;
}

CTextureEntry* CTextureManager::AddTexture(const char* filename,int type)
{
	assert((uint)type<m_TerrainTextures.size());

	// create new texture entry
	CTextureEntry* texentry=new CTextureEntry(filename,type);

	// add entry to list ..
	m_TerrainTextures[type].m_Textures.push_back(texentry);

	return texentry;
}

void CTextureManager::DeleteTexture(CTextureEntry* entry)
{
	// find entry in list
	std::vector<CTextureEntry*>& textures=m_TerrainTextures[entry->GetType()].m_Textures;

	typedef std::vector<CTextureEntry*>::iterator Iter;
	Iter i=std::find(textures.begin(),textures.end(),entry);
	if (i!=textures.end()) {
		textures.erase(i);
	}
	delete entry;
}

void CTextureManager::LoadTerrainTexturesImpl(int terraintype,const char* fileext_filter)
{
	CStr pathname("art/textures/terrain/types/");
	pathname+=m_TerrainTextures[terraintype].m_Name;
	pathname+="/";

	Handle dir=vfs_open_dir(pathname.c_str());
	vfsDirEnt dent;

	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, fileext_filter) == 0)
		{
			LOG(NORMAL, LOG_CATEGORY, "CTextureManager::LoadTerrainTextures(): texture %s added to type %s", dent.name, m_TerrainTextures[terraintype].m_Name.c_str());
			AddTexture(dent.name, terraintype);
		}

		vfs_close_dir(dir);
	}
}

void CTextureManager::BuildTerrainTypes()
{
	Handle dir=vfs_open_dir("art/textures/terrain/types/");
	vfsDirEnt dent;

	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, "/") == 0)
		{
			AddTextureType(dent.name);
		}

		vfs_close_dir(dir);
	}

}

int CTextureManager::LoadTerrainTextures()
{
	// find all the terrain types by directory name
	BuildTerrainTypes();

	// now iterate through terrain types loading all textures of that type
	for (uint i=0;i<m_TerrainTextures.size();i++) {
		for (uint j=0;j<ARRAY_SIZE(SupportedTextureFormats);j++) {
			LoadTerrainTexturesImpl(i,SupportedTextureFormats[j]);
		}
	}

	return 0;
}
