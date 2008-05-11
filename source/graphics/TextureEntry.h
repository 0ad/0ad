#ifndef INCLUDED_TEXTUREENTRY
#define INCLUDED_TEXTUREENTRY

#include <map>

#include "lib/res/handle.h"
#include "ps/CStr.h"

#include "TextureManager.h"

class XMBElement;
class CXeromyces;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry: class wrapping a terrain texture object; contains various other required
// elements - color of minimap, terrain "group" it belongs to, etc
class CTextureEntry
{
public:
	typedef std::vector<CTerrainGroup *> GroupVector;

private:
	// Tag = file name stripped of path and extension (grass_dark_1)
	CStr m_Tag;
	
	// The property sheet used by this texture
	CTerrainPropertiesPtr m_pProperties;
	
	// Path to the texture file
	CStr m_TexturePath;
	
	void* m_Bitmap; // UI bitmap object (user data for ScEd)
	Handle m_Handle; // handle to GL texture data
	
	// BGRA color of topmost mipmap level, for coloring minimap, or a color
	// specified by the terrain properties
	u32 m_BaseColor;
	// ..Valid is true if the base color has been cached
	bool m_BaseColorValid;
	
	// All terrain type groups we're a member of
	GroupVector m_Groups;

	// A map of all loaded textures and their texture handles for GetByHandle.
	static std::map<Handle, CTextureEntry *> m_LoadedTextures;	

	// load texture from file
	void LoadTexture();
	// calculate the root color of the texture, used for coloring minimap
	void BuildBaseColor();

public:
	// Most of the texture's data is delay-loaded, so after the constructor has
	// been called, the texture entry is ready to be used.
	CTextureEntry(CTerrainPropertiesPtr props, const CStr& path);
	~CTextureEntry();

	CStr GetTag() const
	{ return m_Tag; }
	
	CTerrainPropertiesPtr GetProperties() const
	{ return m_pProperties; }
	
	CStr GetTexturePath() const
	{ return m_TexturePath; }
	
	void* GetBitmap() const { return m_Bitmap; }
	void SetBitmap(void* bmp) { m_Bitmap=bmp; }

	// Get texture handle, load texture if not loaded.
	Handle GetHandle() { 
		if (m_Handle==-1) LoadTexture();
		return m_Handle; 
	}
	// Get mipmap color in BGRA format
	u32 GetBaseColor() {
		if (!m_BaseColorValid) BuildBaseColor();
		return m_BaseColor;
	}
	
	// ScEd wants to sort textures by their group's index. 
	size_t GetType() const
	{ return m_Groups[0]->GetIndex(); }
	const GroupVector &GetGroups() const
	{ return m_Groups; }
	
	// returns whether this texture-entry has loaded any data yet
	bool IsLoaded() { return (m_Handle!=-1); }
	
	// The texture entry class maintains a map of loaded textures and their
	// handles.
	static CTextureEntry *GetByHandle(Handle handle);
};

#endif 
