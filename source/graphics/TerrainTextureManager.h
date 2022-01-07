/* Copyright (C) 2022 Wildfire Games.
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

#include "lib/file/vfs/vfs_path.h"
#include "lib/res/handle.h"
#include "ps/CStr.h"
#include "ps/Singleton.h"
#include "renderer/backend/gl/DeviceCommandContext.h"
#include "renderer/backend/gl/Texture.h"

#include <map>
#include <memory>
#include <vector>

// access to sole CTerrainTextureManager object
#define g_TexMan CTerrainTextureManager::GetSingleton()

#define NUM_ALPHA_MAPS 14

class CTerrainTextureEntry;
class CTerrainProperties;

typedef std::shared_ptr<CTerrainProperties> CTerrainPropertiesPtr;

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
	// Composite alpha map (all the alpha maps packed into one texture).
	std::unique_ptr<Renderer::Backend::GL::CTexture> m_CompositeAlphaMap;
	// Data is used to separate file loading and uploading to GPU.
	std::shared_ptr<u8> m_CompositeDataToUpload;
	// Coordinates of each (untransformed) alpha map within the packed texture.
	struct
	{
		float u0, u1, v0, v1;
	} m_AlphaMapCoords[NUM_ALPHA_MAPS];
};


///////////////////////////////////////////////////////////////////////////////////////////
// CTerrainTextureManager : manager class for all terrain texture objects
class CTerrainTextureManager : public Singleton<CTerrainTextureManager>
{
	friend class CTerrainTextureEntry;

public:
	using TerrainGroupMap = std::map<CStr, CTerrainGroup*>;
	using TerrainAlphaMap = std::map<VfsPath, TerrainAlpha>;

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

	CTerrainTextureManager::TerrainAlphaMap::iterator LoadAlphaMap(const VfsPath& alphaMapType);

	void UploadResourcesIfNeeded(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext);

private:
	// All texture entries created by this class, for easy freeing now that
	// textures may be in several STextureType's
	std::vector<CTerrainTextureEntry*> m_TextureEntries;

	TerrainGroupMap m_TerrainGroups;

	TerrainAlphaMap m_TerrainAlphas;

	size_t m_LastGroupIndex;

	// A way to separate file loading and uploading to GPU to not stall uploading.
	// Once we get a properly threaded loading we might optimize that.
	std::vector<CTerrainTextureManager::TerrainAlphaMap::iterator> m_AlphaMapsToUpload;
};

#endif // INCLUDED_TERRAINTEXTUREMANAGER
