#ifndef _TEXTUREENTRY_H
#define _TEXTUREENTRY_H

#include "res/handle.h"
#include "CStr.h"

#include "TextureManager.h"

class XMBElement;
class CXeromyces;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry: class wrapping a terrain texture object; contains various other required
// elements - color of minimap, terrain "group" it belongs to, etc
class CTextureEntry
{
public:
	typedef std::vector<CTerrainTypeGroup *> GroupVector;
	
	~CTextureEntry();

	CStr GetTag() const
	{ return m_Tag; }
	
	CStr GetTexturePath() const
	{ return m_TexturePath; }
	
	// accessor - get UI bitmap object
	void* GetBitmap() const { return m_Bitmap; }
	// accessor - set UI bitmap object
	void SetBitmap(void* bmp) { m_Bitmap=bmp; }

	// accessor - get texture handle
	Handle GetHandle() { 
		if (m_Handle==-1) LoadTexture();
		return m_Handle; 
	}
	// accessor - get mipmap color
	u32 GetBaseColor() {
		if (!m_BaseColorValid) BuildBaseColor();
		return m_BaseColor;
	}
	
	const GroupVector &GetGroups() const
	{ return m_Groups; }
	
	// returns whether this texture-entry has loaded any data yet
	bool IsLoaded() { return (m_Handle!=-1); }
	
	static CTextureEntry *FromXML(XMBElement el, CXeromyces *pFile);
	
	// Load all properties from the parent (run on all terrains after loading
	// all the xml's or when changes has been made on the parent)
	// This will only actually work once per instance (noop on subsequent calls)
	void LoadParent();

private:
	CTextureEntry();

	// load texture from file
	void LoadTexture();
	// calculate the root color of the texture, used for coloring minimap
	void BuildBaseColor();

	CStr m_Tag;
	CStr m_ParentName;
	CTextureEntry *m_pParent;
	
	CStr m_TexturePath;
	void* m_Bitmap; // UI bitmap object (user data for ScEd)
	Handle m_Handle; // handle to GL texture data
	
	// BGRA color of topmost mipmap level, for coloring minimap, or a color
	// manually specified in the Terrain XML (or by any parent)
	// ..Valid is true if the base color is a cached value or an XML override
	u32 m_BaseColor;
	bool m_BaseColorValid;
	
	// All terrain type groups we're a member of
	GroupVector m_Groups;
};

#endif 
