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

/*
 * return DLL version information
 */

#ifndef INCLUDED_WDLL_VER
#define INCLUDED_WDLL_VER

/**
 * read DLL version information and append it to a string.
 *
 * @param pathname of DLL (preferably the complete path, so that we don't
 * inadvertently load another one on the library search path.)
 * if no extension is given, .dll will be appended.
 *
 * the text output includes the module name.
 * on failure, the version is given as "unknown".
 **/
extern void wdll_ver_Append(const fs::wpath& pathname, std::wstring& list);

#endif	// #ifndef INCLUDED_WDLL_VER
