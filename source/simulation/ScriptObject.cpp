#include "precompiled.h"

#include "ScriptObject.h"
#include "Entity.h"

CScriptObject::CScriptObject()
{
	Function = NULL;
}

CScriptObject::CScriptObject( JSFunction* _Function )
{
	SetFunction( _Function );
}

CScriptObject::CScriptObject( jsval v )
{
	SetJSVal( v );
}

void CScriptObject::SetFunction( JSFunction* _Function )
{
	Function = _Function;
}

void CScriptObject::SetJSVal( jsval v )
{
	CStrW Source;
	switch( JS_TypeOfValue( g_ScriptingHost.GetContext(), v ) )
	{
	case JSTYPE_STRING:
		Source = g_ScriptingHost.ValueToUCString( v );
		Compile( L"unknown", Source );
		break;
	case JSTYPE_FUNCTION:
		SetFunction( JS_ValueToFunction( g_ScriptingHost.GetContext(), v ) );
		break;
	default:
		Function = NULL;
	}
}

bool CScriptObject::Defined()
{
	return( Function != NULL );
}

JSObject* CScriptObject::GetFunctionObject()
{
	if( Function )
		return( JS_GetFunctionObject( Function ) );
	return( NULL );
}

// Executes a script attached to a JS object.
// Returns false if the script isn't defined, if the script can't be executed,
// otherwise true. Script return value is in rval.
bool CScriptObject::Run( JSObject* Context, jsval* rval )
{
	if( !Function )
		return( false );
	return( JS_TRUE == JS_CallFunction( g_ScriptingHost.GetContext(), Context, Function, 0, NULL, rval ) );
}

// This variant casts script return value to a boolean, and passes it back.
bool CScriptObject::Run( JSObject* Context )
{
	jsval Temp;
	if( !Run( Context, &Temp ) )
		return( false );
	return( g_ScriptingHost.ValueToBool( Temp ) );
}

// Treat this as an event handler and dispatch an event to it. Return !evt->m_cancelled, as a convenience.
bool CScriptObject::DispatchEvent( JSObject* Context, CScriptEvent* evt )
{
	jsval Temp;
	jsval EventObject = OBJECT_TO_JSVAL( evt->GetScript() );
	if( Function )
		JS_CallFunction( g_ScriptingHost.GetContext(), Context, Function, 1, &EventObject, &Temp );
	return( !evt->m_Cancelled );
}

void CScriptObject::Compile( CStrW FileNameTag, CStrW FunctionBody )
{
	const char* argnames[] = { "evt" };
	utf16string str16=FunctionBody.utf16();
	Function = JS_CompileUCFunction( g_ScriptingHost.GetContext(), NULL, NULL, 1, argnames, str16.c_str(), str16.size(), (CStr)FileNameTag, 0 );
}
