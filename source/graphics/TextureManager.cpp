#include "precompiled.h"

#include <algorithm>
#include <vector>

#include "TextureManager.h"
#include "TextureEntry.h"
#include "TerrainProperties.h"

#include "lib/res/graphics/ogl_tex.h"
#include "lib/ogl.h"
#include "lib/timer.h"

#include "ps/CLogger.h"
#include "ps/Filesystem.h"

#define LOG_CATEGORY "graphics"

CTextureManager::CTextureManager():
	m_LastGroupIndex(0)
{}

CTextureManager::~CTextureManager()
{
	UnloadTerrainTextures();
}

void CTextureManager::UnloadTerrainTextures()
{
	for (size_t i=0; i < m_TextureEntries.size(); i++)
		delete m_TextureEntries[i];
	m_TextureEntries.clear();

	TerrainGroupMap::iterator it = m_TerrainGroups.begin();
	while (it != m_TerrainGroups.end())
	{
		delete it->second;
		++it;
	}
	m_TerrainGroups.clear();

	m_LastGroupIndex = 0;
}

CTextureEntry* CTextureManager::FindTexture(const CStr& tag_)
{
	CStr tag(tag_);
	// Strip extension off of tag
	long pos=tag.ReverseFind(".");
	if (pos != -1)
	{
		tag = tag.substr(0, pos);
	}
	for (size_t i=0;i<m_TextureEntries.size();i++)
	{
		if (m_TextureEntries[i]->GetTag() == tag)
			return m_TextureEntries[i];
	}

	LOG(CLogger::Warning, LOG_CATEGORY, "TextureManager: Couldn't find terrain %s", tag.c_str());
	return 0;
}

CTextureEntry* CTextureManager::FindTexture(Handle handle)
{
	return CTextureEntry::GetByHandle(handle);
}

CTerrainPropertiesPtr CTextureManager::GetPropertiesFromFile(const CTerrainPropertiesPtr& props, const char* path)
{
	return CTerrainProperties::FromXML(props, path);
}

CTextureEntry *CTextureManager::AddTexture(const CTerrainPropertiesPtr& props, const CStr& path)
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
void CTextureManager::LoadTextures(const CTerrainPropertiesPtr& props, const char* dir)
{
	VfsPaths pathnames;
	if(fs_GetPathnames(g_VFS, dir, 0, pathnames) < 0)
		return;
	for(size_t i = 0; i < pathnames.size(); i++)
	{
		const char* texture_name = pathnames[i].string().c_str();

		// skip files that obviously aren't textures.
		// note: this loop runs for each file in dir, even .xml;
		// we should skip those to avoid spurious "texture load failed".
		// we can't use FindFile's filter param because new texture formats
		// may later be added and that interface doesn't support specifying
		// multiple extensions.
		if(!tex_is_known_extension(texture_name))
			continue;

		// build name of associated xml file (i.e. replace extension)
		char xml_name[PATH_MAX+5];	// add room for .XML
		strcpy_s(xml_name, PATH_MAX, texture_name);
		const char* ext = path_extension(texture_name);
		SAFE_STRCPY(xml_name + (ext-texture_name), "xml");

		CTerrainPropertiesPtr myprops;
		// Has XML file -> attempt to load properties
		if (FileExists(xml_name))
		{
			myprops = GetPropertiesFromFile(props, xml_name);
			if (myprops)
				LOG(CLogger::Normal,  LOG_CATEGORY, "CTextureManager: Successfully loaded override xml %s for texture %s", xml_name, texture_name);
		}

		// Error or non-existant xml file -> use parent props
		if (!myprops)
			myprops = props;

		AddTexture(myprops, texture_name);
	}
}

void CTextureManager::RecurseDirectory(const CTerrainPropertiesPtr& parentProps, const char* cur_dir_path)
{
	//LOG(CLogger::Normal,  LOG_CATEGORY, "CTextureManager::RecurseDirectory(%s)", path.c_str());

	CTerrainPropertiesPtr props;

	// Load terrains.xml first, if it exists
	char fn[PATH_MAX];
	snprintf(fn, PATH_MAX, "%s%s", cur_dir_path, "terrains.xml");
	fn[PATH_MAX-1] = '\0';
	if (FileExists(fn))
		props = GetPropertiesFromFile(parentProps, fn);
	
	// No terrains.xml, or read failures -> use parent props (i.e. 
	if (!props)
	{
		LOG(CLogger::Normal,  LOG_CATEGORY,
			"CTextureManager::RecurseDirectory(%s): no terrains.xml (or errors while loading) - using parent properties", cur_dir_path);
		props = parentProps;
	}

	// Recurse once for each subdirectory
	DirectoryNames subdirectoryNames;
	(void)g_VFS->GetDirectoryEntries(cur_dir_path, 0, &subdirectoryNames);
	for (size_t i=0;i<subdirectoryNames.size();i++)
	{
		char subdirectoryPath[PATH_MAX];
		path_append(subdirectoryPath, cur_dir_path, subdirectoryNames[i].c_str(), PATH_APPEND_SLASH);
		RecurseDirectory(props, subdirectoryPath);
	}

	LoadTextures(props, cur_dir_path);
}


int CTextureManager::LoadTerrainTextures()
{
	RecurseDirectory(CTerrainPropertiesPtr(), "art/textures/terrain/types/");
	return 0;
}

CTerrainGroup *CTextureManager::FindGroup(const CStr& name)
{
	TerrainGroupMap::const_iterator it=m_TerrainGroups.find(name);
	if (it != m_TerrainGroups.end())
		return it->second;
	else
		return m_TerrainGroups[name] = new CTerrainGroup(name, ++m_LastGroupIndex);
}

void CTerrainGroup::AddTerrain(CTextureEntry *pTerrain)
{
	m_Terrains.push_back(pTerrain);
}

void CTerrainGroup::RemoveTerrain(CTextureEntry *pTerrain)
{
	std::vector<CTextureEntry *>::iterator it;
	it=find(m_Terrains.begin(), m_Terrains.end(), pTerrain);
	if (it != m_Terrains.end())
		m_Terrains.erase(it);
}
