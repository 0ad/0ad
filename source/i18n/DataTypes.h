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

#ifndef INCLUDED_I18N_DATATYPES
#define INCLUDED_I18N_DATATYPES

#include "StrImmutable.h"

namespace I18n
{
	// Use for names of objects that should be translated, e.g.
	// translate("Construct $obj")<<I18n::Noun(selectedobject.name)
	struct Noun
	{
		template<typename T> Noun(T d) : value(d) {}
		StrImW value;
	};

	// Allow translate("Hello $you")<<I18n::Name(playername), which
	// won't attempt to translate the player's name.
	// Templated to allow char* and wchar_t*
	struct Name
	{
		template<typename T> Name(T d) : value(d) {}
		StrImW value;
	};

	// Also allow I18n::Raw("english message"), which does the same
	// non-translation but makes more sense when writing e.g. error messages
	typedef Name Raw;
}


#endif // INCLUDED_I18N_DATATYPES
