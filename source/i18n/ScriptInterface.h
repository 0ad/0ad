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

ScriptObject provides an interface to the JavaScript engine.

ScriptValues are used to generate jsvals from 'constants' (strings, ints)
and 'variables' (dependent on the type passed with "translate(...) <<").

*/

#ifndef INCLUDED_I18N_SCRIPTINTERFACE
#define INCLUDED_I18N_SCRIPTINTERFACE

#include "Common.h"
#include "BufferVariable.h"

#include <vector>

ERROR_SUBGROUP(I18n, Script);
ERROR_TYPE(I18n_Script, SetupFailed);

#if OS_WIN // the Windows JS libraries are using '_W64 long' to avoid MSVC warnings
typedef _W64 long jsval;
#else
typedef long jsval;
#endif
typedef unsigned short jschar;
struct JSContext;
struct JSObject;

namespace I18n
{

	class ScriptValue;
	class CLocale;

	class ScriptObject
	{
	public:
		ScriptObject(CLocale* locale, JSContext* cx, JSObject* scope);
		~ScriptObject();

		bool ExecuteCode(const jschar* data, size_t len, const char* filename);

		const StrImW CallFunction(const char* name, const std::vector<BufferVariable*>& vars, const std::vector<ScriptValue*>& params);

		JSContext* Context;
		JSObject* Object;
	};

	// Virtual base class for script values
	class ScriptValue
	{
	public:
		virtual jsval GetJsval(const std::vector<BufferVariable*>& vars) = 0;
		virtual ~ScriptValue() {};

	protected:
		JSContext* Context;
	};

	// Particular types of script value:

	class ScriptValueString : public ScriptValue
	{
	public:
		ScriptValueString(ScriptObject& script, const wchar_t* s);
		~ScriptValueString();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
		jsval Value;
	};

	class ScriptValueInteger : public ScriptValue
	{
	public:
		ScriptValueInteger(ScriptObject& script, const int v);
		~ScriptValueInteger();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
		jsval Value;
	};

	class ScriptValueVariable : public ScriptValue
	{
	public:
		ScriptValueVariable(ScriptObject& script, const unsigned char id);
		~ScriptValueVariable();
		jsval GetJsval(const std::vector<BufferVariable*>& vars);

	private:
		unsigned char ID;
		void* GCVal; // something that's being garbage-collected
	};
}

#endif // INCLUDED_I18N_SCRIPTINTERFACE
