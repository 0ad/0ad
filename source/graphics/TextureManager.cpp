#include "precompiled.h"

#include <algorithm>

#include "TextureManager.h"
#include "lib.h"
#include "ogl.h"
#include "res/tex.h"
#include "CLogger.h"


const char* SupportedTextureFormats[] = { ".png", ".dds", ".tga", ".bmp" };



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
			if (strcmp((const char*) ttype.m_Textures[i]->m_Name,filename)==0) {
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
			if (handle==ttype.m_Textures[i]->m_Handle) {
				return ttype.m_Textures[i];
			}
		}
	}

	return 0;
}

CTextureEntry* CTextureManager::AddTexture(const char* filename,int type)
{
	assert((uint)type<m_TerrainTextures.size());
	
	CStr pathname("art/textures/terrain/types/");
	pathname+=m_TerrainTextures[type].m_Name;
	pathname+='/';
	pathname+=filename;

	Handle h=tex_load((const char*) pathname);
	if (h <= 0) {
		LOG(ERROR, "CTextureManager::AddTexture(): texture %s failed loading\n", pathname.c_str());
		return 0;
	} else {
		int tw;
		int th;

		tex_info(h, &tw, &th, NULL, NULL, NULL);

		tw &= (tw-1);
		th &= (th-1);
		if (tw || th) {
			return 0;
		}
	}
	
	// create new texture entry
	CTextureEntry* texentry=new CTextureEntry;
	texentry->m_Name=filename;
	texentry->m_Handle=h;
	texentry->m_Type=type;

	// upload texture for future GL use
	tex_upload(h,GL_LINEAR_MIPMAP_LINEAR);

	// setup texture to repeat
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// get root color for coloring minimap by querying root level of the texture 
	// (this should decompress any compressed textures for us),
	// then scaling it down to a 1x1 size 
	//	- an alternative approach of just grabbing the top level of the mipmap tree fails 
	//	(or gives an incorrect colour) in some cases: 
	//		- suspect bug on Radeon cards when SGIS_generate_mipmap is used 
	//		- any textures without mipmaps
	// we'll just take the basic approach here:
	int width,height;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&height);

	unsigned char* buf=new unsigned char[width*height*4];
	glGetTexImage(GL_TEXTURE_2D,0,GL_BGRA_EXT,GL_UNSIGNED_BYTE,buf);
	gluScaleImage(GL_BGRA_EXT,width,height,GL_UNSIGNED_BYTE,buf,
			1,1,GL_UNSIGNED_BYTE,&texentry->m_BaseColor);
	delete[] buf;

	// add entry to list ..
	m_TerrainTextures[type].m_Textures.push_back(texentry);

	// .. and return it
	return texentry;
}

void CTextureManager::DeleteTexture(CTextureEntry* entry)
{
	// find entry in list
	std::vector<CTextureEntry*>& textures=m_TerrainTextures[entry->m_Type].m_Textures;
		
	typedef std::vector<CTextureEntry*>::iterator Iter;
	Iter i=std::find(textures.begin(),textures.end(),entry);
	if (i!=textures.end()) {
		textures.erase(i);
	}
	delete entry;
}

void CTextureManager::LoadTerrainTextures(int terraintype,const char* fileext)
{
	CStr pathname("art/textures/terrain/types/");
	pathname+=m_TerrainTextures[terraintype].m_Name;
	pathname+="/";
	
	Handle dir=vfs_open_dir(pathname.c_str());
	vfsDirEnt dent;
	
	if (dir > 0)
	{
		while (vfs_next_dirent(dir, &dent, fileext) == 0)
		{
			LOG(NORMAL, "CTextureManager::LoadTerrainTextures(): texture %s added to type %s\n", dent.name, m_TerrainTextures[terraintype].m_Name.c_str());
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

void CTextureManager::LoadTerrainTextures()
{
	// find all the terrain types by directory name
	BuildTerrainTypes();

	// now iterate through terrain types loading all textures of that type
	for (uint i=0;i<m_TerrainTextures.size();i++) {
		for (uint j=0;j<sizeof(SupportedTextureFormats)/sizeof(const char*);j++) {
			LoadTerrainTextures(i,SupportedTextureFormats[j]);
		}
	}
}
