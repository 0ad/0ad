#include "ScriptObject.h"
#include "Entity.h"

CScriptObject::CScriptObject()
{
	Type = UNDEFINED;
	Function = NULL;
	Script = NULL;
}

CScriptObject::CScriptObject( JSFunction* _Function )
{
	SetFunction( _Function );
}

CScriptObject::CScriptObject( JSScript* _Script )
{
	SetScript( _Script );
}

CScriptObject::CScriptObject( jsval v )
{
	SetJSVal( v );
}

void CScriptObject::SetFunction( JSFunction* _Function )
{
	Type = FUNCTION;
	Function = _Function;
	Script = NULL;
}

void CScriptObject::SetScript( JSScript* _Script )
{
	Type = SCRIPT;
	Function = NULL;
	Script = _Script;
}

void CScriptObject::SetJSVal( jsval v )
{
	switch( JS_TypeOfValue( g_ScriptingHost.GetContext(), v ) )
	{
	case JSTYPE_STRING:
	{
		CStrW Source = g_ScriptingHost.ValueToUCString( v );
		JSScript* Script = JS_CompileUCScript( g_ScriptingHost.GetContext(), JS_GetGlobalObject( g_ScriptingHost.GetContext() ), Source.c_str(), Source.Length(), "subset query script", 0 );
		SetScript( Script );
		break;
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( g_ScriptingHost.GetContext(), v );
		SetFunction( fn );
		break;
	}
	default:
		Type = UNDEFINED;
		Function = NULL;
		Script = NULL;
	}
}

bool CScriptObject::Defined()
{
	return( Type != UNDEFINED );
}

// Executes a script attached to a JS object.
// Returns false if the script isn't defined, if the script can't be executed,
// otherwise true. Script return value is in rval.
bool CScriptObject::Run( JSObject* Context, jsval* rval )
{
	switch( Type )
	{
	case UNDEFINED:
		return( false );
	case FUNCTION:
		return( JS_TRUE == JS_CallFunction( g_ScriptingHost.GetContext(), Context, Function, 0, NULL, rval ) );
	case SCRIPT:
		return( JS_TRUE == JS_ExecuteScript( g_ScriptingHost.GetContext(), Context, Script, rval ) );
	default:
		return( false );
	}
}

// This variant casts script return value to a boolean, and passes it back.
bool CScriptObject::Run( JSObject* Context )
{
	jsval Temp;
	if( !Run( Context, &Temp ) )
		return( false );
	return( g_ScriptingHost.ValueToBool( Temp ) );
}
	
void CScriptObject::CompileScript( CStrW FileNameTag, CStrW Script )
{
	Type = FUNCTION;
	Function = JS_CompileUCFunction( g_ScriptingHost.GetContext(), NULL, NULL, 0, NULL, Script, Script.Length(), (CStr)FileNameTag, 0 );
}
