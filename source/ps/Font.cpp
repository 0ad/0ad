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

#include "Font.h"

#include "ps/ConfigDB.h"
#include "lib/res/graphics/unifont.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "graphics"

#include <map>
#include <string>

const char* DefaultFont = "verdana16";

CFont::CFont(const char* name)
{
	// TODO perhaps: cache the resultant filename (or Handle) for each
	// font name; but it's nice to allow run-time alteration of the fonts

	std::string fontFilename;

	CStr fontName = "font."; fontName += name;

	// See if the config value can be loaded
	CConfigValue* fontFilenameVar = g_ConfigDB.GetValue(CFG_USER, fontName);
	if (fontFilenameVar && fontFilenameVar->GetString(fontFilename))
	{
		h = unifont_load(fontFilename.c_str());
	}
	else
	{
		// Not found in the config file -- try it as a simple filename
		h = unifont_load(name);

		// Found it
		if (h > 0)
			return;

		// Not found as a font -- give up and use the default.
		LOG_ONCE(CLogger::Error, LOG_CATEGORY, "Failed to find font '%s'", name);
		h = unifont_load(DefaultFont);
		// Assume this worked
	}
}

CFont::~CFont()
{
	unifont_unload(h);
}

void CFont::Bind()
{
	unifont_bind(h);
}

int CFont::GetLineSpacing()
{
	return unifont_linespacing(h);
}

int CFont::GetHeight()
{
	return unifont_height(h);
}

int CFont::GetCharacterWidth(wchar_t c)
{
	return unifont_character_width(h, c);
}

void CFont::CalculateStringSize(const CStrW& string, int& width, int& height)
{
	unifont_stringsize(h, (const wchar_t*)string, width, height);
}
