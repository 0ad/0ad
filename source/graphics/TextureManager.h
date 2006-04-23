#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include <vector>
#include <map>
#include "boost/shared_ptr.hpp"

#include "lib/res/handle.h"

#include "CStr.h"
#include "Singleton.h"

// access to sole CTextureManager  object
#define g_TexMan CTextureManager ::GetSingleton()

class XMBElement;
class CXeromyces;
class CTextureEntry;
class CTerrainProperties;

typedef boost::shared_ptr<CTerrainProperties> CTerrainPropertiesPtr;

class CTerrainGroup
{
	// name of this terrain group (as specified by the terrain XML)
	CStr m_Name;
	// "index".. basically a bogus integer that can be used by ScEd to set texture
	// priorities
	int m_Index;
	// list of textures of this type (found from the texture directory)
	std::vector<CTextureEntry*> m_Terrains;

public:
	CTerrainGroup(CStr name, int index):
		m_Name(name),
		m_Index(index)
	{}
	
	// Add a texture entry to this terrain type
	void AddTerrain(CTextureEntry *);
	// Remove a texture entry
	void RemoveTerrain(CTextureEntry *);

	int GetIndex() const
	{ return m_Index; }
	CStr GetName() const
	{ return m_Name; }

	const std::vector<CTextureEntry*> &GetTerrains() const
	{ return m_Terrains; }
};

///////////////////////////////////////////////////////////////////////////////////////////
// CTextureManager : manager class for all terrain texture objects
class CTextureManager : public Singleton<CTextureManager>
{
public:
	typedef std::map<CStr, CTerrainGroup *> TerrainGroupMap;

private:
	// All texture entries created by this class, for easy freeing now that
	// textures may be in several STextureType's
	std::vector<CTextureEntry *> m_TextureEntries;

	TerrainGroupMap m_TerrainGroups;

	uint m_LastGroupIndex;

	// Find+load all textures in directory; check if
	// there's an override XML with the same basename (if there is, load it)
	void LoadTextures(CTerrainPropertiesPtr props, const char* dir);
	
	// Load all terrains below path, using props as the parent property sheet.
	void RecurseDirectory(CTerrainPropertiesPtr props, const char* dir);
	
	CTerrainPropertiesPtr GetPropertiesFromFile(CTerrainPropertiesPtr props, const char* path);

public:
	// constructor, destructor
	CTextureManager();
	~CTextureManager();

	// Find all XML's in the directory (with subdirs) and try to load them as
	// terrain XML's
	int LoadTerrainTextures();

	void UnloadTerrainTextures();
	
	CTextureEntry* FindTexture(CStr tag);
	CTextureEntry* FindTexture(Handle handle);
	CTextureEntry* GetRandomTexture();
	
	// Create a texture object for a new terrain texture at path, using the
	// property sheet props.
	CTextureEntry *AddTexture(CTerrainPropertiesPtr props, CStr path);
	
	// Remove the texture from all our maps and lists and delete it afterwards.
	void DeleteTexture(CTextureEntry* entry);
	
	// Find or create a new texture group. All terrain groups are owned by the
	// texturemanager (TerrainTypeManager)
	CTerrainGroup *FindGroup(CStr name);
	// Use the default group for all terrain types that don't have their own
	// ScEd currently relies on every texture having one group (and is happy ignorant of any
	// extra groups that might exist)
	CTerrainGroup *GetDefaultGroup();

	const TerrainGroupMap &GetGroups() const
	{ return m_TerrainGroups; }
};


#endif
