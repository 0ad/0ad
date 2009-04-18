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
GUI base
*/

#include "precompiled.h"

#include <string>

#include "GUI.h"

using std::string;

//--------------------------------------------------------
//  Help Classes/Structs for the GUI implementation
//--------------------------------------------------------

CClientArea::CClientArea() : pixel(0.f,0.f,0.f,0.f), percent(0.f,0.f,0.f,0.f)
{
}

CClientArea::CClientArea(const CStr& Value)
{
	SetClientArea(Value);
}

CRect CClientArea::GetClientArea(const CRect &parent) const
{
	// If it's a 0 0 100% 100% we need no calculations
	if (percent == CRect(0.f,0.f,100.f,100.f) && pixel == CRect(0.f,0.f,0.f,0.f))
		return parent;

	CRect client;

	// This should probably be cached and not calculated all the time for every object.
    client.left =	parent.left + (parent.right-parent.left)*percent.left/100.f + pixel.left;
	client.top =	parent.top + (parent.bottom-parent.top)*percent.top/100.f + pixel.top;
	client.right =	parent.left + (parent.right-parent.left)*percent.right/100.f + pixel.right;
	client.bottom =	parent.top + (parent.bottom-parent.top)*percent.bottom/100.f + pixel.bottom;

	return client;
}

bool CClientArea::SetClientArea(const CStr& Value)
{
	// Get value in STL string format
	string _Value = Value;

	// This might lack incredible speed, but since all XML files
	//  are read at startup, reading 100 client areas will be
	//  negligible in the loading time.

	// Setup parser to parse the value

	// One of the four values:
	//  will give outputs like (in argument):
	//  (200) <== no percent, just the first $value
	//  (200) (percent) <== just the percent
	//  (200) (percent) (100) <== percent PLUS pixel
	//  (200) (percent) (-100) <== percent MINUS pixel
	//  (200) (percent) (100) (-100) <== Both PLUS and MINUS are used, INVALID
	/*
	string one_value = "_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]";
	string four_values = one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)" + 
						 one_value + "$arg(delim)_"; // it's easier to just end with another delimiter
	*/
	// Don't use the above strings, because they make this code go very slowly
	const char* four_values =
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_[-_$arg(_minus)]$value[$arg(percent)%_[+_$value]_[-_$arg(_minus)$value]_]" "$arg(delim)"
		"_";
	CParser& parser (CParserCache::Get(four_values));

    CParserLine line;
	line.ParseString(parser, _Value);

	if (!line.m_ParseOK)
		return false;

	int arg_count[4]; // argument counts for the four values
	int arg_start[4] = {0,0,0,0}; // location of first argument, [0] is always 0

	// Divide into the four piles (delimiter is an argument named "delim")
	for (int i=0, valuenr=0; i<(int)line.GetArgCount(); ++i)
	{
		string str;
		line.GetArgString(i, str);
		if (str == "delim")
		{
			if (valuenr==0)
			{
				arg_count[0] = i;
				arg_start[1] = i+1;
			}
			else
			{
				if (valuenr!=3)
				{
					debug_assert(valuenr <= 2);
					arg_start[valuenr+1] = i+1;
					arg_count[valuenr] = arg_start[valuenr+1] - arg_start[valuenr] - 1;
				}
				else
					arg_count[3] = (int)line.GetArgCount() - arg_start[valuenr] - 1;
			}

			++valuenr;
		}
	}

	// Iterate argument
	
	// This is the scheme:
	// 1 argument = Just pixel value
	// 2 arguments = Just percent value
	// 3 arguments = percent and pixel
	// 4 arguments = INVALID

  	// Default to 0
	float values[4][2] = {{0.f,0.f},{0.f,0.f},{0.f,0.f},{0.f,0.f}};
	for (int v=0; v<4; ++v)
	{
		if (arg_count[v] == 1)
		{
			string str;
			line.GetArgString(arg_start[v], str);

			if (!line.GetArgFloat(arg_start[v], values[v][1]))
				return false;
		}
		else
		if (arg_count[v] == 2)
		{
			if (!line.GetArgFloat(arg_start[v], values[v][0]))
				return false;
		}
		else
		if (arg_count[v] == 3)
		{
			if (!line.GetArgFloat(arg_start[v], values[v][0]) ||
				!line.GetArgFloat(arg_start[v]+2, values[v][1]))
				return false;

		}
		else return false;
	}

	// Now store the values[][] in the right place
	pixel.left =		values[0][1];
	pixel.top =			values[1][1];
	pixel.right =		values[2][1];
	pixel.bottom =		values[3][1];
	percent.left =		values[0][0];
	percent.top =		values[1][0];
	percent.right =		values[2][0];
	percent.bottom =	values[3][0];
	return true;
}



//--------------------------------------------------------
//  Error definitions
//--------------------------------------------------------
// TODO Gee: (2004-09-01) Keeper? The PS_NAME_ABIGUITY for instance doesn't let the user know which objects.
DEFINE_ERROR(PS_NAME_TAKEN,			"Reference name is taken");
DEFINE_ERROR(PS_OBJECT_FAIL,		"Object provided is null");
DEFINE_ERROR(PS_SETTING_FAIL,		"Setting does not exist");
DEFINE_ERROR(PS_VALUE_INVALID,		"Value provided is syntactically incorrect");
DEFINE_ERROR(PS_NEEDS_PGUI,			"m_pGUI is NULL when needed for a requested operation");
DEFINE_ERROR(PS_NAME_AMBIGUITY,		"Two or more objects are sharing name");
DEFINE_ERROR(PS_NEEDS_NAME,			"An object are trying to fit into a GUI without a name");

DEFINE_ERROR(PS_LEXICAL_FAIL,		"PS_LEXICAL_FAIL");
DEFINE_ERROR(PS_SYNTACTICAL_FAIL,	"PS_SYNTACTICAL_FAIL");
