//-----------------------------------------------------------
//
// Name:		Texture.h
// Last Update: 25/11/03
// Author:		Rich Cross
// Contact:		rich@0ad.wildfiregames.com
//
// Description: Basic texture class
//
//-----------------------------------------------------------

#ifndef _TEXTURE_H
#define _TEXTURE_H

#include "res/res.h"
#include "CStr.h"

class CTexture
{
public:
	CTexture() : m_Handle(0) {}
	CTexture(const char* name) : m_Name(name), m_Handle(0) {}

	void SetName(const char* name) { m_Name=name; }
	const char* GetName() const { return (const char*) m_Name; }

	Handle GetHandle() const { return m_Handle; }
	void SetHandle(Handle handle) { m_Handle=handle; }

private:
	CStr m_Name;
	Handle m_Handle;
};

#endif
