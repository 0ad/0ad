/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#define LOG_CATEGORY L"graphics"

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

	LOG(CLogger::Warning, LOG_CATEGORY, L"TextureManager: Couldn't find terrain %hs", tag.c_str());
	return 0;
}

CTerrainPropertiesPtr CTextureManager::GetPropertiesFromFile(const CTerrainPropertiesPtr& props, const VfsPath& pathname)
{
	return CTerrainProperties::FromXML(props, pathname);
}

CTextureEntry *CTextureManager::AddTexture(const CTerrainPropertiesPtr& props, const VfsPath& path)
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
void CTextureManager::LoadTextures(const CTerrainPropertiesPtr& props, const VfsPath& path)
{
	VfsPaths pathnames;
	if(fs_util::GetPathnames(g_VFS, path, 0, pathnames) < 0)
		return;
	for(size_t i = 0; i < pathnames.size(); i++)
	{
		// skip files that obviously aren't textures.
		// note: this loop runs for each file in dir, even .xml;
		// we should skip those to avoid spurious "texture load failed".
		// we can't use FindFile's filter param because new texture formats
		// may later be added and that interface doesn't support specifying
		// multiple extensions.
		if(!tex_is_known_extension(pathnames[i]))
			continue;

		VfsPath pathnameXML = fs::change_extension(pathnames[i], L".xml");
		CTerrainPropertiesPtr myprops;
		// Has XML file -> attempt to load properties
		if (FileExists(pathnameXML))
		{
			myprops = GetPropertiesFromFile(props, pathnameXML);
			if (myprops)
				LOG(CLogger::Normal,  LOG_CATEGORY, L"CTextureManager: Successfully loaded override xml %ls for texture %ls", pathnameXML.string().c_str(), pathnames[i].string().c_str());
		}

		// Error or non-existant xml file -> use parent props
		if (!myprops)
			myprops = props;

		AddTexture(myprops, pathnames[i]);
	}
}

void CTextureManager::RecurseDirectory(const CTerrainPropertiesPtr& parentProps, const VfsPath& path)
{
	//LOG(CLogger::Normal,  LOG_CATEGORY, L"CTextureManager::RecurseDirectory(%ls)", path.string().c_str());

	CTerrainPropertiesPtr props;

	// Load terrains.xml first, if it exists
	VfsPath pathname = path/L"terrains.xml"; 
	if (FileExists(pathname))
		props = GetPropertiesFromFile(parentProps, pathname);
	
	// No terrains.xml, or read failures -> use parent props (i.e. 
	if (!props)
	{
		LOG(CLogger::Normal, LOG_CATEGORY, L"CTextureManager::RecurseDirectory(%ls): no terrains.xml (or errors while loading) - using parent properties", path.string().c_str());
		props = parentProps;
	}

	// Recurse once for each subdirectory
	DirectoryNames subdirectoryNames;
	(void)g_VFS->GetDirectoryEntries(path, 0, &subdirectoryNames);
	for (size_t i=0;i<subdirectoryNames.size();i++)
	{
		VfsPath subdirectoryPath = AddSlash(path/subdirectoryNames[i]);
		RecurseDirectory(props, subdirectoryPath);
	}

	LoadTextures(props, path);
}


int CTextureManager::LoadTerrainTextures()
{
	RecurseDirectory(CTerrainPropertiesPtr(), L"art/textures/terrain/types/");
	return 0;
}

CTerrainGroup* CTextureManager::FindGroup(const CStr& name)
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
