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

#ifndef INCLUDED_SGUIICON
#define INCLUDED_SGUIICON

#include "ps/CStr.h"
#include "ps/Shapes.h"

/**
 * Icon, you create them in the XML file with root element <setup>.
 * You use them in text owned by different objects... Such as CText.
 */
struct SGUIIcon
{
	// This struct represents an immutable type, so ensure to avoid copying the strings.
	NONCOPYABLE(SGUIIcon);
	MOVABLE(SGUIIcon);

	SGUIIcon() : m_CellID(0) {}

	// Sprite name of icon
	CStr m_SpriteName;

	// Size
	CSize m_Size;

	// Cell of texture to use; ignored unless the texture has specified cell-size
	int m_CellID;
};

#endif // INCLUDED_SGUIICON
