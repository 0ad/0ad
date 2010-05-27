/* Copyright (C) 2010 Wildfire Games.
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

/*
 * Definition of a single terrain tile
 */

#ifndef INCLUDED_MINIPATCH
#define INCLUDED_MINIPATCH

#include "lib/res/handle.h"

#include "graphics/TextureEntry.h"

///////////////////////////////////////////////////////////////////////////////
// CMiniPatch: definition of a single terrain tile
class CMiniPatch
{
public:
	// constructor
	CMiniPatch();

	// texture applied to tile
	CTextureEntry* Tex;
	// 'priority' of the texture - determines drawing order of terrain textures
	int Priority;

	CTextureEntry* GetTextureEntry() { return Tex; }
	Handle GetHandle() { return Tex ? Tex->GetHandle() : 0; }
	int GetPriority() { return Priority; }
};


#endif
