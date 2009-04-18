/* Copyright (C) 2009 Wildfire Games.
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
