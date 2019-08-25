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

/*
This file is used by all bits of GUI code that need to repeat some code
for a variety of types (to avoid repeating the list of types in half a dozen
places, and to make it much easier to add a new type). Just do
		#define TYPE(T) your_code_involving_T;
		#include "GUItypes.h"
		#undef TYPE
to handle every possible type.
*/

#ifndef GUITYPE_IGNORE_COPYABLE
TYPE(bool)
TYPE(i32)
TYPE(u32)
TYPE(float)
TYPE(EAlign)
TYPE(EVAlign)
TYPE(CPos)
#endif

#ifndef GUITYPE_IGNORE_NONCOPYABLE
TYPE(CClientArea)
TYPE(CGUIColor)
TYPE(CGUIList)
TYPE(CGUISeries)
TYPE(CGUISpriteInstance)
TYPE(CGUIString)
TYPE(CStr)
TYPE(CStrW)
#endif
