#ifndef _TEXTUREENTRY_H
#define _TEXTUREENTRY_H

#include "res/res.h"
#include "CStr.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTextureEntry: class wrapping a terrain texture object; contains various other required
// elements - color of minimap, terrain "group" it belongs to, etc
class CTextureEntry
{
public:
	CTextureEntry(const char* name,int type);

	// accessor - return texture name
	const char* GetName() const { return (const char*) m_Name; }
	
	// accessor - get UI bitmap object
	void* GetBitmap() const { return m_Bitmap; }
	// accessor - set UI bitmap object
	void SetBitmap(void* bmp) { m_Bitmap=bmp; }

	// accessor - get texture handle
	Handle GetHandle() { 
		if (m_Handle==0xffffffff) LoadTexture();
		return m_Handle; 
	}
	// accessor - get mipmap color
	u32 GetBaseColor() {
		if (!m_BaseColorValid) BuildBaseColor();
		return m_BaseColor;
	}
	// accessor - return texture type
	int GetType() const { return m_Type; }

private:
	// load texture from file
	void LoadTexture();
	// calculate the root color of the texture, used for coloring minimap
	void BuildBaseColor();

	// filename
	CStr m_Name;
	// UI bitmap object
	void* m_Bitmap;
	// handle to GL texture data
	Handle m_Handle;
	// BGRA color of topmost mipmap level, for coloring minimap
	u32 m_BaseColor;
	// above color valid?
	bool m_BaseColorValid;
	// "type" of texture - index into TextureManager texturetypes array
	int m_Type;
};

#endif
