/*

	CTerrainProperties
	
	Basically represents a set of terrain attributes loaded from XML. These
	objects are organized in an inheritance tree, determined at load time.

*/

#ifndef graphics_TerrainProperties_H
#define graphics_TerrainProperties_H

#include "CStr.h"

class CTerrainGroup;
class XMBElement;
class CXeromyces;

class CTerrainProperties
{
public:
	typedef std::vector<CTerrainGroup *> GroupVector;
	
private:
	CTerrainProperties *m_pParent;

	// BGRA color of topmost mipmap level, for coloring minimap, or a color
	// manually specified in the Terrain XML (or by any parent)
	// ..Valid is true if the base color is specified in this terrain XML
	// No caching here, since ideally, a saved XML file of an object should
	// produce be equivalent to the source file
	u32 m_BaseColor;
	bool m_HasBaseColor;
	
	// All terrain type groups we're a member of
	GroupVector m_Groups;

	void LoadXML(XMBElement node, CXeromyces *pFile);

public:
	CTerrainProperties(CTerrainProperties *parent);

	// Create a new object and load the XML file specified. Returns NULL upon
	// failure
	// The parent pointer may be NULL, for the "root" terrainproperties object.
	static CTerrainProperties *FromXML(CTerrainProperties *parent, const char* path);
	
	// Save the object to an XML file. Implement when needed! ;-)
	// bool WriteXML(CStr path);
	
	inline CTerrainProperties *GetParent() const
	{	return m_pParent; }
	
	// Return true if this property object or any of its parents has a basecolor
	// override (mmap attribute in the XML file)
	bool HasBaseColor();
	// Return the minimap color specified in this property object or in any of
	// its parents. If no minimap color is specified, return garbage.
	// Use HasBaseColor() to see if the value is valid.
	// The color value is in BGRA format
	u32 GetBaseColor();

	const GroupVector &GetGroups() const
	{ return m_Groups; }
};

#endif
