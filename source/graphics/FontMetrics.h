/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_FONTMETRICS
#define INCLUDED_FONTMETRICS

class CFont;
class CStrIntern;

/**
 * Helper class for measuring sizes of text.
 * This will load the font when necessary, and will return plausible values
 * if loading fails (since misrendering is better than crashing).
 */
class CFontMetrics
{
public:
	CFontMetrics(CStrIntern font);

	int GetLineSpacing() const;
	int GetHeight() const;
	int GetCharacterWidth(wchar_t c) const;
	void CalculateStringSize(const wchar_t* string, int& w, int& h) const;

private:
	shared_ptr<CFont> m_Font;
};

#endif // INCLUDED_FONTMETRICS
