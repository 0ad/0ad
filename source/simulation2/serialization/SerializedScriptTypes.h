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

#ifndef INCLUDED_SERIALIZEDSCRIPTTYPES
#define INCLUDED_SERIALIZEDSCRIPTTYPES

enum
{
	SCRIPT_TYPE_VOID = 0,
	SCRIPT_TYPE_NULL = 1,
	SCRIPT_TYPE_ARRAY = 2,
	SCRIPT_TYPE_OBJECT = 3,
	SCRIPT_TYPE_STRING = 4,
	SCRIPT_TYPE_INT = 5,
	SCRIPT_TYPE_DOUBLE = 6,
	SCRIPT_TYPE_BOOLEAN = 7,
	SCRIPT_TYPE_BACKREF = 8
};

#endif // INCLUDED_SERIALIZEDSCRIPTTYPES
