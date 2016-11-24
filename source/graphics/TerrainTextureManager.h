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

#ifndef INCLUDED_TERRAINTEXTUREMANAGER
#define INCLUDED_TERRAINTEXTUREMANAGER

#include <map>
#include <memory>
#include <vector>

#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/handle.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/Singleton.h"

// access to sole CTerrainTextureManager object
#define g_TexMan CTerrainTextureManager::GetSingleton()

#define NUM_ALPHA_MAPS 14

class XMBElement;
class CXeromyces;
class CTerrainTextureEntry;
class CTerrainProperties;

typedef shared_ptr<CTerrainProperties> CTerrainPropertiesPtr;

class CTerrainGroup
{
	// name of this terrain group (as specified by the terrain XML)
	CStr m_Name;
	// "index".. basically a bogus integer that can be used by ScEd to set texture
	// priorities
	size_t m_Index;
	// list of textures of this type (found from the texture directory)
	std::vector<CTerrainTextureEntry*> m_Terrains;

public:
	CTerrainGroup(CStr name, size_t index):
		m_Name(name),
		m_Index(index)
	{}

	// Add a texture entry to this terrain type
	void AddTerrain(CTerrainTextureEntry*);
	// Remove a texture entry
	void RemoveTerrain(CTerrainTextureEntry*);

	size_t GetIndex() const
	{ return m_Index; }
	CStr GetName() const
	{ return m_Name; }

	const std::vector<CTerrainTextureEntry*> &GetTerrains() const
	{ return m_Terrains; }
};


struct TerrainAlpha
{
	// ogl_tex handle of composite alpha map (all the alpha maps packed into one texture)
	Handle m_hCompositeAlphaMap;
	// coordinates of each (untransformed) alpha map within the packed texture
	struct {
		float u0,u1,v0,v1;
	} m_AlphaMapCoords[NUM_ALPHA_MAPS];
};


///////////////////////////////////////////////////////////////////////////////////////////
// CTerrainTextureManager : manager class for all terrain texture objects
class CTerrainTextureManager : public Singleton<CTerrainTextureManager>
{
	friend class CTerrainTextureEntry;

public:
	typedef std::map<CStr, CTerrainGroup*> TerrainGroupMap;
	typedef std::map<VfsPath, TerrainAlpha> TerrainAlphaMap;

private:
	// All texture entries created by this class, for easy freeing now that
	// textures may be in several STextureType's
	std::vector<CTerrainTextureEntry*> m_TextureEntries;

	TerrainGroupMap m_TerrainGroups;

	TerrainAlphaMap m_TerrainAlphas;

	size_t m_LastGroupIndex;

public:
	// constructor, destructor
	CTerrainTextureManager();
	~CTerrainTextureManager();

	// Find all XML's in the directory (with subdirs) and try to load them as
	// terrain XML's
	int LoadTerrainTextures();

	void UnloadTerrainTextures();

	CTerrainTextureEntry* FindTexture(const CStr& tag) const;

	// Create a texture object for a new terrain texture at path, using the
	// property sheet props.
	CTerrainTextureEntry* AddTexture(const CTerrainPropertiesPtr& props, const VfsPath& path);

	// Remove the texture from all our maps and lists and delete it afterwards.
	void DeleteTexture(CTerrainTextureEntry* entry);

	// Find or create a new texture group. All terrain groups are owned by the
	// texturemanager (TerrainTypeManager)
	CTerrainGroup* FindGroup(const CStr& name);

	const TerrainGroupMap& GetGroups() const
	{ return m_TerrainGroups; }
};


#endif
