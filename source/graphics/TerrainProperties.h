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

/*
///////////////////////////////////////////////
	CTerrainProperties

	Basically represents a set of terrain attributes loaded from XML. These
	objects are organized in an inheritance tree, determined at load time.

*/

#ifndef INCLUDED_TERRAINPROPERTIES
#define INCLUDED_TERRAINPROPERTIES

#include <memory>

#include "ps/CStr.h"
#include "lib/file/vfs/vfs_path.h"

class CTerrainGroup;
class XMBElement;
class CXeromyces;
class CTerrainProperties;

typedef shared_ptr<CTerrainProperties> CTerrainPropertiesPtr;

class CTerrainProperties
{
public:
	typedef std::vector<CTerrainGroup *> GroupVector;

private:
	CTerrainPropertiesPtr m_pParent;

	// BGRA color of topmost mipmap level, for coloring minimap, or a color
	// manually specified in the Terrain XML (or by any parent)
	// ..Valid is true if the base color is specified in this terrain XML
	// No caching here, since ideally, a saved XML file of an object should
	// produce be equivalent to the source file
	u32 m_BaseColor;
	bool m_HasBaseColor;

	CStr m_MovementClass;

	// Orientation of texture (in radians) (default pi/4 = 45 degrees)
	float m_TextureAngle;

	// Size of texture in metres (default 32m = 8 tiles)
	float m_TextureSize;

	// All terrain type groups we're a member of
	GroupVector m_Groups;

public:
	CTerrainProperties(CTerrainPropertiesPtr parent);

	// Create a new object and load the XML file specified. Returns NULL upon
	// failure
	// The parent pointer may be NULL, for the "root" terrainproperties object.
	static CTerrainPropertiesPtr FromXML(const CTerrainPropertiesPtr& parent, const VfsPath& pathname);

	void LoadXml(XMBElement node, CXeromyces *pFile, const VfsPath& pathname);

	// Save the object to an XML file. Implement when needed! ;-)
	// bool WriteXML(const CStr& path);

	inline CTerrainPropertiesPtr GetParent() const
	{
		return m_pParent;
	}

	// Return true if this property object or any of its parents has a basecolor
	// override (mmap attribute in the XML file)
	bool HasBaseColor();
	// Return the minimap color specified in this property object or in any of
	// its parents. If no minimap color is specified, return garbage.
	// Use HasBaseColor() to see if the value is valid.
	// The color value is in BGRA format
	u32 GetBaseColor();

	float GetTextureAngle()
	{
		return m_TextureAngle;
	}

	float GetTextureSize()
	{
		return m_TextureSize;
	}

	CStr GetMovementClass() const
	{
		return m_MovementClass;
	}

	const GroupVector &GetGroups() const
	{
		return m_Groups;
	}
};

#endif
