/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_TERRAINTEXTUREENTRY
#define INCLUDED_TERRAINTEXTUREENTRY

#include "graphics/Material.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/Texture.h"
#include "lib/file/vfs/vfs_path.h"
#include "maths/Matrix3D.h"
#include "ps/CStr.h"

#include <vector>

// CTerrainTextureEntry: class wrapping a terrain texture object; contains various other required
// elements - color of minimap, terrain "group" it belongs to, etc
class CTerrainTextureEntry
{
public:
	using GroupVector = std::vector<CTerrainGroup*>;

	// Most of the texture's data is delay-loaded, so after the constructor has
	// been called, the texture entry is ready to be used.
	CTerrainTextureEntry(CTerrainPropertiesPtr props, const VfsPath& path);
	~CTerrainTextureEntry();

	const CStr& GetTag() const { return m_Tag; }

	const CTerrainProperties& GetProperties() const { return *m_pProperties; }

	// Get texture handle, load texture if not loaded.
	const CTexturePtr& GetTexture() const { return m_Material.GetDiffuseTexture(); }

	const CMaterial& GetMaterial() const { return m_Material; }

	// Returns a matrix of the form [c 0 -s 0; -s 0 -c 0; 0 0 0 0; 0 0 0 1]
	// mapping world-space (x,y,z,1) coordinates onto (u,v,0,1) texcoords
	const CMatrix3D& GetTextureMatrix() const { return m_TextureMatrix; }

	// Used in Atlas to retrieve a texture for previews. Can't use textures
	// directly because they're required on CPU side. Another solution is to
	// retrieve path from diffuse texture from material.
	const VfsPath& GetDiffuseTexturePath() const { return m_DiffuseTexturePath; }

	// Get mipmap color in BGRA format
	u32 GetBaseColor()
	{
		if (!m_BaseColorValid) BuildBaseColor();
		return m_BaseColor;
	}

	CTerrainTextureManager::TerrainAlphaMap::iterator m_TerrainAlpha;

private:
	// Tag = file name stripped of path and extension (grass_dark_1)
	CStr m_Tag;

	VfsPath m_DiffuseTexturePath;

	// The property sheet used by this texture
	CTerrainPropertiesPtr m_pProperties;

	CMaterial m_Material;

	CMatrix3D m_TextureMatrix;

	// BGRA color of topmost mipmap level, for coloring minimap, or a color
	// specified by the terrain properties
	u32 m_BaseColor;
	// ..Valid is true if the base color has been cached
	bool m_BaseColorValid;

	// All terrain type groups we're a member of
	GroupVector m_Groups;

	// calculate the root color of the texture, used for coloring minimap
	void BuildBaseColor();
};

#endif
