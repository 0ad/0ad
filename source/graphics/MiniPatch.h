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

/**
 * =========================================================================
 * File        : MiniPatch.h
 * Project     : 0 A.D.
 * Description : Definition of a single terrain tile
 * =========================================================================
 */

#ifndef INCLUDED_MINIPATCH
#define INCLUDED_MINIPATCH

#include "lib/res/handle.h"

class CPatch;

///////////////////////////////////////////////////////////////////////////////
// CMiniPatch: definition of a single terrain tile
class CMiniPatch
{
public:
	// constructor
	CMiniPatch();
	// destructor
	~CMiniPatch();

	// get the index of this tile in the root terrain object; x,y in [0,MapSize)
	void GetTileIndex(ssize_t& x,ssize_t& z);

public:
	// texture applied to tile
	Handle Tex1;
	// 'priority' of the texture - determines drawing order of terrain textures
	int Tex1Priority;
	// parent patch
	CPatch* m_Parent;
};


#endif
