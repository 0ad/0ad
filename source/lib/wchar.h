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

#ifndef INCLUDED_WCHAR
#define INCLUDED_WCHAR

// note: returning LibError below would be inconvenient; we only use
// these to raise warnings. (codes instead of strings allow suppressing
// the error dialog when running the self-test)
namespace ERR
{
	const LibError WCHAR_SURROGATE     = -100700;
	const LibError WCHAR_OUTSIDE_BMP   = -100701;
	const LibError WCHAR_NONCHARACTER  = -100702;
	const LibError WCHAR_INVALID_UTF8  = -100703;
}

LIB_API std::wstring wstring_from_utf8(const std::string& s);
LIB_API std::string utf8_from_wstring(const std::wstring& s);

#endif	// #ifndef INCLUDED_WCHAR
