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

CTerrainPropertiesPtr CTextureManager::GetPropertiesFromFile(CTerrainPropertiesPtr props, const char* path)
{
	return CTerrainProperties::FromXML(props, path);
}

CTextureEntry *CTextureManager::AddTexture(CTerrainPropertiesPtr props, CStr path)
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
void CTextureManager::LoadTextures(CTerrainPropertiesPtr props, const char* dir)
{
	VFSUtil::FileList files;
	if(!VFSUtil::FindFiles(dir, 0, files))
		// FindFiles has already logged the failure
		return;

	for(VFSUtil::FileList::iterator it = files.begin(); it != files.end(); ++it)
	{
		const char* texture_name = it->c_str();

		// skip files that obviously aren't textures.
		// note: this loop runs for each file in dir, even .xml;
		// we should skip those to avoid spurious "texture load failed".
		// we can't use FindFile's filter param because new texture formats
		// may later be added and that interface doesn't support specifying
		// multiple extensions.
		const char* ext = strrchr(texture_name, '.');
		if(!ext || !stricmp(ext, ".xml") || !stricmp(ext, ".xmb") || !stricmp(ext, ".dtd"))
			continue;
		// Also allow storage of temporary files in the texture directories
		if(ext[strlen(ext)-1] == '~')
			continue;

		// build name of associated xml file (i.e. replace extension)
		char xml_name[PATH_MAX+5];	// add room for .XML
		strcpy_s(xml_name, PATH_MAX, texture_name);
		strcpy(xml_name + (ext-texture_name), ".xml");	// safe

		CTerrainPropertiesPtr myprops;
		// Has XML file -> attempt to load properties
		if (vfs_exists(xml_name))
		{
			myprops = GetPropertiesFromFile(props, xml_name);
			if (myprops)
				LOG(NORMAL, LOG_CATEGORY, "CTextureManager: Successfully loaded override xml %s for texture %s\n", xml_name, texture_name);
		}

		// Error or non-existant xml file -> use parent props
		if (!myprops)
			myprops = props;

		AddTexture(myprops, texture_name);
	}
}

void CTextureManager::RecurseDirectory(CTerrainPropertiesPtr parentProps, const char* cur_dir_path)
{
	//LOG(NORMAL, LOG_CATEGORY, "CTextureManager::RecurseDirectory(%s)", path.c_str());

	CTerrainPropertiesPtr props;

	// Load terrains.xml first, if it exists
	char fn[PATH_MAX];
	snprintf(fn, PATH_MAX, "%s/%s", cur_dir_path, "terrains.xml");
	if (vfs_exists(fn))
		props = GetPropertiesFromFile(parentProps, fn);
	
	// No terrains.xml, or read failures -> use parent props (i.e. 
	if (!props)
	{
		LOG(NORMAL, LOG_CATEGORY,
			"CTextureManager::RecurseDirectory(%s): no terrains.xml (or errors while loading) - using parent properties", cur_dir_path);
		props = parentProps;
	}

	// Recurse once for each subdirectory
	VFSUtil::FileList subdirs;
	VFSUtil::FindFiles(cur_dir_path, "/", subdirs);
	for (uint i=0;i<subdirs.size();i++)
	{
		RecurseDirectory(props, subdirs[i].c_str());
	}

	LoadTextures(props, cur_dir_path);
}


int CTextureManager::LoadTerrainTextures()
{
	RecurseDirectory(CTerrainPropertiesPtr(), "art/textures/terrain/types");
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
