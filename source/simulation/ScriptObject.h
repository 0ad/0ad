// A generic type and some helper functions
// for scripts

// Mark Thompson (mark@wildfiregames.com / mot20@cam.ac.uk)

#ifndef SCRIPTOBJECT_INCLUDED
#define SCRIPTOBJECT_INCLUDED

#include "CStr.h"
#include "scripting/ScriptingHost.h"
#include "EntityHandles.h"
#include "scripting/JSInterface_Entity.h"
#include "scripting/DOMEvent.h"

class CScriptObject
{
	JSFunction* Function;
public:

	CScriptObject();
	CScriptObject( JSFunction* _Function );
	CScriptObject( jsval v );

	// Initialize in various ways: from a JS function, a string to be compiled, or a jsval containing either.
	void SetFunction( JSFunction* _Function );
	void SetJSVal( jsval v );
	void Compile( CStrW FileNameTag, CStrW FunctionBody );

	inline bool Defined();

	// JSObject wrapping the function if it's defined, NULL if it isn't.
	JSObject* GetFunctionObject();

	// Executes a script attached to a JS object.
	// Returns false if the script isn't defined, if the script can't be executed,
	// otherwise true. Script return value is in rval.
	bool Run( JSObject* Context, jsval* rval );
	// This variant casts script return value to a boolean, and passes it back.
	bool Run( JSObject* Context );

	// Treat this as an event handler and dispatch an event to it.
	void DispatchEvent( JSObject* Context, CScriptEvent* evt );
};

#endif
