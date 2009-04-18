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

Stores sections of a translated string: constant strings, simple variables,
and JS function calls, allowing conversion to strings.

*/

#ifndef INCLUDED_I18N_TSCOMPONENT
#define INCLUDED_I18N_TSCOMPONENT

#include "StrImmutable.h"
#include "BufferVariable.h"
#include "ScriptInterface.h"

#include <algorithm>
#include <vector>

namespace I18n
{
	class CLocale;

	class TSComponent
	{
	public:
		virtual const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const = 0;
		virtual ~TSComponent() {}
	};

	
	class TSComponentString : public TSComponent
	{
		NONCOPYABLE(TSComponentString);
	public:
		TSComponentString(const wchar_t* s) : String(s) {}

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

	private:
		const StrImW String;
	};


	class TSComponentVariable : public TSComponent
	{
		NONCOPYABLE(TSComponentVariable);
	public:
		TSComponentVariable(unsigned char id) : ID(id) {}

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

	private:
		unsigned char ID;
	};


	class TSComponentFunction : public TSComponent
	{
		NONCOPYABLE(TSComponentFunction);
	public:
		TSComponentFunction(const char* name) : Name(name) {}
		~TSComponentFunction();

		const StrImW ToString(CLocale* locale, std::vector<BufferVariable*>& vars) const;

		// AddParam is called when loading the function, building up the
		// internal list of parameters (strings / ints / variables)
		void AddParam(ScriptValue* p);

	private:
		const std::string Name;
		std::vector<ScriptValue*> Params;
	};


}

#endif // INCLUDED_I18N_TSCOMPONENT
