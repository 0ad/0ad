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

#ifndef INCLUDED_FONTMANAGER
#define INCLUDED_FONTMANAGER

#include "ps/CStrIntern.h"

#include <unordered_map>

class CFont;

/**
 * Font manager: loads and caches bitmap fonts.
 */
class CFontManager
{
public:
	shared_ptr<CFont> LoadFont(CStrIntern fontName);

private:
	bool ReadFont(CFont* font, CStrIntern fontName);

	using FontsMap = std::unordered_map<CStrIntern, shared_ptr<CFont> >;
	FontsMap m_Fonts;
};

#endif // INCLUDED_FONTMANAGER
