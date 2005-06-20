#ifndef _TEXTUREMANAGER_H
#define _TEXTUREMANAGER_H

#include <vector>
#include <map>

#include "res/handle.h"

#include "CStr.h"
#include "Singleton.h"

// access to sole CTextureManager  object
#define g_TexMan CTextureManager ::GetSingleton()

class XMBElement;
class CXeromyces;
class CTextureEntry;

class CTerrainTypeGroup
{
	// name of this terrain type (as referenced in terrain xmls)
	CStr m_Name;
	// "index".. basically a bogus integer that can be used by ScEd to set texture
	// priorities
	int m_Index;
	// list of textures of this type (found from the texture directory)
	std::vector<CTextureEntry*> m_Terrains;

public:
	CTerrainTypeGroup(CStr name, int index):
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
	typedef std::map<CStr, CTerrainTypeGroup *> TerrainTypeGroupMap;

private:
	// All texture entries created by this class, for easy freeing now that
	// textures may be in several STextureType's
	std::vector<CTextureEntry *> m_TextureEntries;

	TerrainTypeGroupMap m_TerrainTypeGroups;

	uint m_LastGroupIndex;

	// Find all XML's in the directory (with subdirs) and  try to load them as
	// terrain XML's
	void RecurseDirectory(CStr path);

public:
	// constructor, destructor
	CTextureManager();
	~CTextureManager();
	
	int LoadTerrainTextures();
	void LoadTerrainsFromXML(const char *filename);

	CTextureEntry* FindTexture(CStr tag);
	CTextureEntry* FindTexture(Handle handle);
	CTextureEntry* GetRandomTexture();
	// TODO How do Atlas/ScEd want to create new terrain types?
	// CTextureEntry* AddTexture(const char* filename,int type);
	void DeleteTexture(CTextureEntry* entry);
	
	// Find or create a new texture group. All terrain groups are owned by the
	// texturemanager (TerrainTypeManager)
	CTerrainTypeGroup *FindGroup(CStr name);
	// Use the default group for all terrain types that don't have their own
	// ScEd currently relies on every texture having one group (and is happy ignorant of any
	// extra groups that might exist)
	CTerrainTypeGroup *GetDefaultGroup();

	const TerrainTypeGroupMap &GetGroups() const
	{ return m_TerrainTypeGroups; }
};


#endif
