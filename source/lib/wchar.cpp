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

#include "precompiled.h"
#include "wchar.h"


std::wstring wstring_from_string(const std::string& s)
{
	wchar_t buf[1000];
	const size_t numConverted = mbstowcs(buf, s.c_str(), ARRAY_SIZE(buf)-1);
	debug_assert(numConverted < ARRAY_SIZE(buf));
	return buf;
}


std::string string_from_wstring(const std::wstring& s)
{
	char buf[1000];
	const size_t numConverted = wcstombs(buf, s.c_str(), ARRAY_SIZE(buf)-1);
	debug_assert(numConverted < ARRAY_SIZE(buf));
	return buf;
}
