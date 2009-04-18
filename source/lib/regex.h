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
 * File        : regex.h
 * Project     : 0 A.D.
 * Description : minimal regex implementation
 * =========================================================================
 */

#ifndef INCLUDED_REGEX
#define INCLUDED_REGEX

/**
 * see if string matches pattern.
 *
 * @param s input string
 * @param w pseudo-regex to match against. case-insensitive;
 * may contain '?' and/or '*' wildcards. if NULL, matches everything.
 *
 * @return 1 if they match, otherwise 0.
 *
 * algorithmfrom http://www.codeproject.com/string/wildcmp.asp.
 **/
extern int match_wildcard(const char* s, const char* w);
/// unicode version of match_wildcard.
extern int match_wildcardw(const wchar_t* s, const wchar_t* w);

#endif	// #ifndef INCLUDED_REGEX
