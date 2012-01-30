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

#ifndef INCLUDED_FONT
#define INCLUDED_FONT

#include "lib/res/handle.h"

class CStrW;

/*

To use CFont:

CFont font("name");
font.Bind();
glwprintf(L"Hello world");

*/

class CFont
{
public:
	CFont(const CStrW& name);
	~CFont();

	void Bind(size_t unit = 0);
	bool HasRGB();
	int GetLineSpacing();
	int GetHeight();
	int GetCharacterWidth(wchar_t c);
	void CalculateStringSize(const CStrW& string, int& w, int& h);

private:
	Handle h;
};


#endif // INCLUDED_FONT
