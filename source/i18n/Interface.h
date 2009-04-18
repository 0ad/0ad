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

The only file that external code should need to include.

*/

#ifndef INCLUDED_I18N_INTERFACE
#define INCLUDED_I18N_INTERFACE

#include "StringBuffer.h"
#include "DataTypes.h"

struct JSContext;
struct JSObject;

namespace I18n
{
	// Use an interface class, so minimal headers are required by
	// anybody who only wants to make use of Translate()
	class CLocale_interface
	{
	public:
		virtual StringBuffer Translate(const wchar_t* id) = 0;

		// Load* functions return true for success, false for failure

		// Pass the contents of a UTF-16LE BOMmed .js file.
		// The filename is just used to give more useful error messages from JS.
		virtual bool LoadFunctions(const char* filedata, size_t len, const char* filename) = 0;

		// Desires a .lng file, as produced by convert.pl
		virtual bool LoadStrings(const char* filedata) = 0;

		// Needs .wrd files generated through tables.pl
		virtual bool LoadDictionary(const char* filedata) = 0;

		virtual void UnloadDictionaries() = 0;

		virtual ~CLocale_interface() {}
	};

	// Build a CLocale. Returns NULL on failure.
	CLocale_interface* NewLocale(JSContext* cx, JSObject* scope);
}

#endif // INCLUDED_I18N_INTERFACE
