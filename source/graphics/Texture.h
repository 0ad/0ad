//-----------------------------------------------------------
// Name:		Texture.h
// Description: Basic texture class
//
//-----------------------------------------------------------

#ifndef INCLUDED_TEXTURE
#define INCLUDED_TEXTURE

#include "lib/res/handle.h"
#include "ps/CStr.h"

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
