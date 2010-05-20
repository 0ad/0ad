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

// A generic type and some helper functions
// for scripts

#ifndef INCLUDED_SCRIPTOBJECT
#define INCLUDED_SCRIPTOBJECT

#include "scripting/SpiderMonkey.h"

class CStrW;
class CScriptEvent;

class CScriptObject
{
	JSFunction* Function;
	JSObject* FunctionObject;
	void Root();
	void Uproot();

public:

	CScriptObject();
	CScriptObject( JSFunction* _Function );
	CScriptObject( jsval v );
	CScriptObject( const CScriptObject& copy );

	~CScriptObject();

	// Initialize in various ways: from a JS function, a string to be compiled, or a jsval containing either.
	void SetFunction( JSFunction* _Function );
	void SetJSVal( jsval v );
	void Compile( const CStrW& FileNameTag, const CStrW& FunctionBody );

	inline bool Defined()
	{
		return( Function != NULL );
	}

	inline operator bool() { return( Function != NULL ); }
	inline bool operator!() { return( !Function ); }
	inline bool operator==( const CScriptObject& compare ) { return( Function == compare.Function ); }

	// JSObject wrapping the function if it's defined, NULL if it isn't.
	JSObject* GetFunctionObject();

	// Executes a script attached to a JS object.
	// Returns false if the script isn't defined, if the script can't be executed,
	// otherwise true. Script return value is in rval.
	bool Run( JSObject* Context, jsval* rval, uintN argc = 0, jsval* argv = NULL );
	// This variant casts script return value to a boolean, and passes it back.
	bool Run( JSObject* Context, uintN argc = 0, jsval* argv = NULL );

	// Treat this as an event handler and dispatch an event to it.
	bool DispatchEvent( JSObject* Context, CScriptEvent* evt );
};

#endif
