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

#include "precompiled.h"

#include "GUIutil.h"

#include "gui/CGUIString.h"
#include "ps/CLogger.h"

class CGUIList;
class CGUISeries;

template <>
bool GUI<bool>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, bool& Output)
{
	if (Value == L"true")
		Output = true;
	else if (Value == L"false")
		Output = false;
	else
		return false;

	return true;
}

template <>
bool GUI<i32>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, int& Output)
{
	Output = Value.ToInt();
	return true;
}

template <>
bool GUI<u32>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, u32& Output)
{
	Output = Value.ToUInt();
	return true;
}

template <>
bool GUI<float>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, float& Output)
{
	Output = Value.ToFloat();
	return true;
}

template <>
bool GUI<CRect>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CRect& Output)
{
	const unsigned int NUM_COORDS = 4;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CRect parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CRect parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	if (!stream.eof())
	{
		LOGWARNING("Too many CRect parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	// Finally the rectangle values
	Output = CRect(coords[0], coords[1], coords[2], coords[3]);

	return true;
}

template <>
bool GUI<CClientArea>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CClientArea& Output)
{
	return Output.SetClientArea(Value.ToUTF8());
}

template <>
bool GUI<CGUIColor>::ParseString(const CGUI* pGUI, const CStrW& Value, CGUIColor& Output)
{
	return Output.ParseString(pGUI, Value.ToUTF8());
}

template <>
bool GUI<CSize>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CSize& Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CSize parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CSize parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.cx = coords[0];
	Output.cy = coords[1];

	if (!stream.eof())
	{
		LOGWARNING("Too many CSize parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool GUI<CPos>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CPos& Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CPos parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CPos parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.x = coords[0];
	Output.y = coords[1];

	if (!stream.eof())
	{
		LOGWARNING("Too many CPos parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool GUI<EAlign>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, EAlign& Output)
{
	if (Value == L"left")
		Output = EAlign_Left;
	else if (Value == L"center")
		Output = EAlign_Center;
	else if (Value == L"right")
		Output = EAlign_Right;
	else
		return false;

	return true;
}

template <>
bool GUI<EVAlign>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, EVAlign& Output)
{
	if (Value == L"top")
		Output = EVAlign_Top;
	else if (Value == L"center")
		Output = EVAlign_Center;
	else if (Value == L"bottom")
		Output = EVAlign_Bottom;
	else
		return false;

	return true;
}

template <>
bool GUI<CGUIString>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CGUIString& Output)
{
	Output.SetValue(Value);
	return true;
}

template <>
bool GUI<CStr>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CStr& Output)
{
	Output = Value.ToUTF8();
	return true;
}

template <>
bool GUI<CStrW>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CStrW& Output)
{
	Output = Value;
	return true;
}

template <>
bool GUI<CGUISpriteInstance>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& Value, CGUISpriteInstance& Output)
{
	Output = CGUISpriteInstance(Value.ToUTF8());
	return true;
}

template <>
bool GUI<CGUISeries>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& UNUSED(Value), CGUISeries& UNUSED(Output))
{
	return false;
}

template <>
bool GUI<CGUIList>::ParseString(const CGUI* UNUSED(pGUI), const CStrW& UNUSED(Value), CGUIList& UNUSED(Output))
{
	return false;
}
