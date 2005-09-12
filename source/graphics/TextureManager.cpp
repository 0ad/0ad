#include "precompiled.h"

#include <algorithm>
#include <vector>

#include "TextureManager.h"
#include "TextureEntry.h"
#include "TerrainProperties.h"

#include "lib/res/res.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/ogl.h"
#include "lib/timer.h"

#include "ps/CLogger.h"
#include "ps/VFSUtil.h"

#define LOG_CATEGORY "graphics"

using namespace std;

// filter for vfs_next_dirent
static const char* SupportedTextureFormats[] = { "*.png", "*.dds", "*.tga", "*.bmp" };

CTextureManager::CTextureManager():
	m_LastGroupIndex(0)
{}

CTextureManager::~CTextureManager()
{
	for (size_t i=0;i<m_TextureEntries.size();i++) {
		delete m_TextureEntries[i];
	}
	
	TerrainGroupMap::iterator it=m_TerrainGroups.begin();
	while (it != m_TerrainGroups.end())
	{
		delete it->second;
		++it;
	}
}

CTextureEntry* CTextureManager::FindTexture(CStr tag)
{
	// Strip extension off of tag
	long pos=tag.ReverseFind(".");
	if (pos != -1)
	{
		tag = tag.GetSubstring(0, pos);
	}
	for (uint i=0;i<m_TextureEntries.size();i++)
	{
		if (m_TextureEntries[i]->GetTag() == tag)
			return m_TextureEntries[i];
	}

	LOG(WARNING, LOG_CATEGORY, "TextureManager: Couldn't find terrain %s\n", tag.c_str());
	return 0;
}

CTextureEntry* CTextureManager::FindTexture(Handle handle)
{
	return CTextureEntry::GetByHandle(handle);
}

CTerrainProperties *CTextureManager::GetPropertiesFromFile(CTerrainProperties *props, CStr path)
{
	return CTerrainProperties::FromXML(props, path);
}

CTextureEntry *CTextureManager::AddTexture(CTerrainProperties *props, CStr path)
{
	CTextureEntry *entry = new CTextureEntry(props, path);
	m_TextureEntries.push_back(entry);
	return entry;
}

void CTextureManager::DeleteTexture(CTextureEntry* entry)
{
	typedef std::vector<CTextureEntry*>::iterator Iter;
	Iter i=std::find(m_TextureEntries.begin(),m_TextureEntries.end(),entry);
	if (i!=m_TextureEntries.end()) {
		m_TextureEntries.erase(i);
	}
	delete entry;
}

// FIXME This could be effectivized by surveying the xml files in the directory
// instead of trial-and-error checking for existence of the xml file through
// the VFS.
// jw: indeed this is inefficient and RecurseDirectory should be implemented
// via VFSUtil::EnumFiles, but it works fine and "only" takes 25ms for
// typical maps. therefore, we'll leave it for now.
void CTextureManager::LoadTextures(CTerrainProperties *props, CStr path, const char* fileext_filter)
{
	Handle dir=vfs_dir_open(path.c_str());
 	DirEnt dent;
	
	path += '/';
	
 	if (dir > 0)
 	{
		while (vfs_dir_next_ent(dir, &dent, fileext_filter) == 0)
 		{
			// Strip extension off of dent.name, add .xml, check if the file
			// exists
			CStr xmlname=path+dent.name;
			xmlname=xmlname.GetSubstring(0, xmlname.size() - (strlen(fileext_filter) - 1));
			xmlname += ".xml";
			
			CTerrainProperties *myprops = NULL;
			// Has XML file -> attempt to load properties
			if (vfs_exists(xmlname.c_str()))
				myprops=GetPropertiesFromFile(props, xmlname);
			
			if (myprops)
				LOG(NORMAL, LOG_CATEGORY, "CTextureManager: Successfully loaded override xml %s for texture %s\n", xmlname.c_str(), dent.name);
			
			// Error or non-existant xml file -> use parent props
			if (!myprops)
				myprops = props;
			
			AddTexture(myprops, path+dent.name);
 		}

 		vfs_dir_close(dir);
 	}
}

void CTextureManager::RecurseDirectory(CTerrainProperties *parentProps, CStr path)
{
	//LOG(NORMAL, LOG_CATEGORY, "CTextureManager::RecurseDirectory(%s)", path.c_str());
	
	// Load terrains.xml first, if it exists
	CTerrainProperties *props=NULL;
	CStr xmlpath=path+"terrains.xml";
	if (vfs_exists(xmlpath.c_str()))
		props=GetPropertiesFromFile(parentProps, xmlpath);
	
	// No terrains.xml, or read failures -> use parent props (i.e. 
	if (!props)
	{
		LOG(NORMAL, LOG_CATEGORY,
			"CTextureManager::RecurseDirectory(%s): no terrains.xml (or errors while loading) - using parent properties", path.c_str());
		props = parentProps;
	}

	// Recurse once for each subdirectory

	vector<CStr> folders;
	VFSUtil::FindFiles(path.c_str(), "/", folders);

	for (uint i=0;i<folders.size();i++)
	{
		RecurseDirectory(props, folders[i]);
	}

	for (int i=0;i<ARRAY_SIZE(SupportedTextureFormats);i++)
	{
		LoadTextures(props, path, SupportedTextureFormats[i]);
	}
}


int CTextureManager::LoadTerrainTextures()
{
	RecurseDirectory(NULL, "art/textures/terrain/types");
	return 0;
}

CTerrainGroup *CTextureManager::FindGroup(CStr name)
{
	TerrainGroupMap::const_iterator it=m_TerrainGroups.find(name);
	if (it != m_TerrainGroups.end())
		return it->second;
	else
		return m_TerrainGroups[name] = new CTerrainGroup(name, ++m_LastGroupIndex);
}

/* There was a GetRandomTexture in MainFrm.cpp (sced) previously that gave compile errors...
So I thought "better fix it up and put it in CTextureManager instead".. well, it is never used
except for one *comment* in MainFrm.cpp - d'oh */
CTextureEntry* CTextureManager::GetRandomTexture()
{
	if (!m_TextureEntries.size())
		return NULL;

	u32 type=rand()%(u32)m_TextureEntries.size();
	return m_TextureEntries[type];
}

void CTerrainGroup::AddTerrain(CTextureEntry *pTerrain)
{
	m_Terrains.push_back(pTerrain);
}

void CTerrainGroup::RemoveTerrain(CTextureEntry *pTerrain)
{
	vector<CTextureEntry *>::iterator it;
	it=find(m_Terrains.begin(), m_Terrains.end(), pTerrain);
	if (it != m_Terrains.end())
		m_Terrains.erase(it);
}
