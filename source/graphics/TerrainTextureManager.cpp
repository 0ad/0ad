/* Copyright (C) 2015 Wildfire Games.
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

#include "TerrainTextureManager.h"
#include "TerrainTextureEntry.h"
#include "TerrainProperties.h"

#include "lib/res/graphics/ogl_tex.h"
#include "lib/ogl.h"
#include "lib/timer.h"

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

#include <boost/algorithm/string.hpp>


CTerrainTextureManager::CTerrainTextureManager()
	: m_LastGroupIndex(0)
{
	if (!VfsDirectoryExists(L"art/terrains/"))
		return;
	if (!CXeromyces::AddValidator(g_VFS, "terrain", "art/terrains/terrain.rng"))
		LOGERROR("CTerrainTextureManager: failed to load grammar file 'art/terrains/terrain.rng'");
	if (!CXeromyces::AddValidator(g_VFS, "terrain_texture", "art/terrains/terrain_texture.rng"))
		LOGERROR("CTerrainTextureManager: failed to load grammar file 'art/terrains/terrain_texture.rng'");
}

CTerrainTextureManager::~CTerrainTextureManager()
{
	UnloadTerrainTextures();
	
	for (std::pair<const VfsPath, TerrainAlpha>& ta : m_TerrainAlphas)
	{
		ogl_tex_free(ta.second.m_hCompositeAlphaMap);
		ta.second.m_hCompositeAlphaMap = 0;
	}
}

void CTerrainTextureManager::UnloadTerrainTextures()
{
	for (CTerrainTextureEntry* const& te : m_TextureEntries)
		delete te;
	m_TextureEntries.clear();

	for (const std::pair<CStr, CTerrainGroup*>& tg : m_TerrainGroups)
		delete tg.second;
	m_TerrainGroups.clear();

	m_LastGroupIndex = 0;
}

CTerrainTextureEntry* CTerrainTextureManager::FindTexture(const CStr& tag_) const
{
	CStr tag = tag_.BeforeLast("."); // Strip extension

	for (CTerrainTextureEntry* const& te : m_TextureEntries)
		if (te->GetTag() == tag)
			return te;

	LOGWARNING("CTerrainTextureManager: Couldn't find terrain %s", tag.c_str());
	return 0;
}

CTerrainPropertiesPtr CTerrainTextureManager::GetPropertiesFromFile(const CTerrainPropertiesPtr& props, const VfsPath& pathname)
{
	return CTerrainProperties::FromXML(props, pathname);
}

CTerrainTextureEntry* CTerrainTextureManager::AddTexture(const CTerrainPropertiesPtr& props, const VfsPath& path)
{
	CTerrainTextureEntry* entry = new CTerrainTextureEntry(props, path);
	m_TextureEntries.push_back(entry);
	return entry;
}

void CTerrainTextureManager::DeleteTexture(CTerrainTextureEntry* entry)
{
	auto it = std::find(m_TextureEntries.begin(), m_TextureEntries.end(), entry);
	if (it != m_TextureEntries.end())
		m_TextureEntries.erase(it);

	delete entry;
}

// FIXME This could be effectivized by surveying the xml files in the directory
// instead of trial-and-error checking for existence of the xml file through
// the VFS.
// jw: indeed this is inefficient and RecurseDirectory should be implemented
// via VFSUtil::EnumFiles, but it works fine and "only" takes 25ms for
// typical maps. therefore, we'll leave it for now.
void CTerrainTextureManager::LoadTextures(const CTerrainPropertiesPtr& props, const VfsPath& path)
{
	VfsPaths pathnames;
	if(vfs::GetPathnames(g_VFS, path, 0, pathnames) < 0)
		return;

	for(size_t i = 0; i < pathnames.size(); i++)
	{
		if (pathnames[i].Extension() != L".xml")
			continue;
		
		if (pathnames[i].Basename() == L"terrains")
			continue;
		
		AddTexture(props, pathnames[i]);
	}
}

void CTerrainTextureManager::RecurseDirectory(const CTerrainPropertiesPtr& parentProps, const VfsPath& path)
{
	//LOGMESSAGE("CTextureManager::RecurseDirectory(%s)", path.string8());

	CTerrainPropertiesPtr props;

	// Load terrains.xml first, if it exists
	VfsPath pathname = path / "terrains.xml";
	if (VfsFileExists(pathname))
		props = GetPropertiesFromFile(parentProps, pathname);
	
	// No terrains.xml, or read failures -> use parent props (i.e. 
	if (!props)
	{
		LOGMESSAGE("CTerrainTextureManager::RecurseDirectory(%s): no terrains.xml (or errors while loading) - using parent properties", path.string8());
		props = parentProps;
	}

	// Recurse once for each subdirectory
	DirectoryNames subdirectoryNames;
	(void)g_VFS->GetDirectoryEntries(path, 0, &subdirectoryNames);
	for (size_t i=0;i<subdirectoryNames.size();i++)
	{
		VfsPath subdirectoryPath = path / subdirectoryNames[i] / "";
		RecurseDirectory(props, subdirectoryPath);
	}

	LoadTextures(props, path);
}

int CTerrainTextureManager::LoadTerrainTextures()
{
	CTerrainPropertiesPtr rootProps(new CTerrainProperties(CTerrainPropertiesPtr()));
	RecurseDirectory(rootProps, L"art/terrains/");
	return 0;
}

CTerrainGroup* CTerrainTextureManager::FindGroup(const CStr& name)
{
	TerrainGroupMap::const_iterator it = m_TerrainGroups.find(name);
	if (it != m_TerrainGroups.end())
		return it->second;
	else
		return m_TerrainGroups[name] = new CTerrainGroup(name, ++m_LastGroupIndex);
}

void CTerrainGroup::AddTerrain(CTerrainTextureEntry* pTerrain)
{
	m_Terrains.push_back(pTerrain);
}

void CTerrainGroup::RemoveTerrain(CTerrainTextureEntry* pTerrain)
{
	auto it = find(m_Terrains.begin(), m_Terrains.end(), pTerrain);
	if (it != m_Terrains.end())
		m_Terrains.erase(it);
}
