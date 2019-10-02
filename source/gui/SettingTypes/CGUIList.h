/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_CGUILIST
#define INCLUDED_CGUILIST

#include "gui/SettingTypes/CGUIString.h"

#include <vector>

class CGUIList
{
public: // struct:ish (but for consistency I call it _C_GUIList, and
		//  for the same reason it is a class, so that confusion doesn't
		//  appear when forward declaring.

	// Avoid copying the vector.
	NONCOPYABLE(CGUIList);
	MOVABLE(CGUIList);
	CGUIList() = default;

	/**
	 * List of items (as text), the post-processed result is stored in
	 *  the IGUITextOwner structure of this class.
	 */
	std::vector<CGUIString> m_Items;
};

#endif
