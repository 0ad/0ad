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

#ifndef INCLUDED_SCOLOR
#define INCLUDED_SCOLOR

// SColor3ub: structure for packed RGB colors
struct SColor3ub
{
	u8 R;
	u8 G;
	u8 B;
};

// SColor4ub: structure for packed RGBA colors
struct SColor4ub
{
	u8 R;
	u8 G;
	u8 B;
	u8 A;

	SColor4ub() { }
	SColor4ub(u8 _r, u8 _g, u8 _b, u8 _a) : R(_r), G(_g), B(_b), A(_a) { }
};

#endif
