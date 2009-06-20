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

// Public interface to text-related functions that make use of std::wstring
//
// (This is done in order to avoid forcing inclusion of all the
// STL headers when they're not needed)

//#ifdef new // HACK: to make the STL headers happy with a redefined 'new'
//# undef new
//# include <string>
//# define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
//#else
# include <string>
//#endif

class AtObj;

namespace AtlasObject
{
	// Generate a human-readable string representation of the AtObj,
	// as an easy way of visualising the data (without any horridly ugly
	// XML junk)
	std::wstring ConvertToString(const AtObj& obj);
}
