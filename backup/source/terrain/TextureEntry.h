#ifndef _TEXTUREENTRY_H
#define _TEXTUREENTRY_H

#include "res/res.h"
#include "CStr.h"

class CTextureEntry
{
public:
	CTextureEntry() : m_Bitmap(0) {}

	// filename
	CStr m_Name;
	// UI bitmap object
	void* m_Bitmap;
	// handle to GL texture data
	Handle m_Handle;
	// BGRA color of topmost mipmap level, for coloring minimap
	unsigned int m_BaseColor;
	// "type" of texture - index into TextureManager texturetypes array
	int m_Type;
};

#endif
