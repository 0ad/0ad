// A generic type and some helper functions
// for scripts

// Mark Thompson (mark@wildfiregames.com / mot20@cam.ac.uk)

#ifndef SCRIPTOBJECT_INCLUDED
#define SCRIPTOBJECT_INCLUDED

#include "CStr.h"
#include "scripting/ScriptingHost.h"
#include "EntityHandles.h"
#include "scripting/JSInterface_Entity.h"

class CScriptObject
{
public:
	enum EScriptType
	{
		UNDEFINED,
		FUNCTION,
		SCRIPT
	} Type;

	JSFunction* Function;
	JSScript* Script;

	CScriptObject();
	CScriptObject( JSFunction* _Function );
	CScriptObject( JSScript* _Script );
	CScriptObject( jsval v );
	void SetFunction( JSFunction* _Function );
	void SetScript( JSScript* _Script );
	void SetJSVal( jsval v );
	inline bool Defined();
	// Executes a script attached to a JS object.
	// Returns false if the script isn't defined, if the script can't be executed,
	// otherwise true. Script return value is in rval.
	bool Run( JSObject* Context, jsval* rval );
	// This variant casts script return value to a boolean, and passes it back.
	bool Run( JSObject* Context );
	
	void CompileScript( CStrW FileNameTag, CStrW Script );
};

#endif