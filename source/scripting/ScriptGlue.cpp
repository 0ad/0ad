#include "precompiled.h"

#include "ScriptGlue.h"
#include "CConsole.h"
#include "CLogger.h"
#include "CStr.h"
#include "EntityHandles.h"
#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "Scheduler.h"
#include "scripting/JSInterface_Entity.h"
#include "scripting/JSInterface_BaseEntity.h"
#include "scripting/JSInterface_Vector3D.h"
#include "gui/scripting/JSInterface_IGUIObject.h"
#include "scripting/JSInterface_Selection.h"
#include "scripting/JSInterface_Camera.h"
#include "scripting/JSInterface_Console.h"

extern CConsole* g_Console;

// Parameters for the table are:

// 0: The name the function will be called as from script
// 1: The function which will be called
// 2: The number of arguments this function expects
// 3: Flags (deprecated, always zero)
// 4: Extra (reserved for future use, always zero)

JSFunctionSpec ScriptFunctionTable[] = 
{
	{"writeLog", WriteLog, 1, 0, 0},
	{"writeConsole", JSI_Console::writeConsole, 1, 0, 0 }, // Keep this variant available for now.
	{"getEntityByHandle", getEntityByHandle, 1, 0, 0 },
	{"getEntityTemplate", getEntityTemplate, 1, 0, 0 },
	{"setTimeout", setTimeout, 2, 0, 0 },
	{"setInterval", setInterval, 2, 0, 0 },
	{"cancelInterval", cancelInterval, 0, 0, 0 },
	{"getGUIObjectByName", JSI_IGUIObject::getByName, 1, 0, 0 },
	{"getGlobal", getGlobal, 0, 0, 0 },
	{"getGUIGlobal", getGUIGlobal, 0, 0, 0 },
	{"setCursor", setCursor, 0, 0, 0 },
	{"exit", exitProgram, 0, 0, 0 },
	{"crash", crash, 1, 0, 0 },
	{0, 0, 0, 0, 0}, 
};

enum ScriptGlobalTinyIDs
{
	GLOBAL_SELECTION,
	GLOBAL_GROUPSARRAY,
	GLOBAL_CAMERA,
	GLOBAL_CONSOLE,
};

JSPropertySpec ScriptGlobalTable[] =
{
	{ "selection", GLOBAL_SELECTION, JSPROP_PERMANENT, JSI_Selection::getSelection, JSI_Selection::setSelection },
	{ "groups", GLOBAL_GROUPSARRAY, JSPROP_PERMANENT, JSI_Selection::getGroups, JSI_Selection::setGroups },
	{ "camera", GLOBAL_CAMERA, JSPROP_PERMANENT, JSI_Camera::getCamera, JSI_Camera::setCamera },
	{ "console", GLOBAL_CONSOLE, JSPROP_PERMANENT | JSPROP_READONLY, JSI_Console::getConsole, NULL },
	{ 0, 0, 0, 0, 0 },
};

// Allow scripts to output to the global log file
JSBool WriteLog(JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* UNUSEDPARAM(rval))
{
	if (argc < 1)
		return JS_FALSE;

	CStr logMessage;

	for (int i = 0; i < (int)argc; i++)
	{
		try
		{
			CStr arg = g_ScriptingHost.ValueToString( argv[i] );
			logMessage += arg;
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			// Do nothing.
		}
	}

	// We should perhaps unicodify (?) the logger at some point.

	LOG( NORMAL, logMessage );

	return JS_TRUE;
}

JSBool getEntityByHandle( JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	i32 handle;
	try
	{
		handle = g_ScriptingHost.ValueToInt( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		*rval = JSVAL_NULL;
		JS_ReportError( context, "Invalid handle" );
		return( JS_TRUE );
	}
	HEntity* v = g_EntityManager.getByHandle( (u16)handle );
	if( !v )
	{
		JS_ReportError( context, "No entity occupying handle: %d", handle );
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	JSObject* entity = JS_NewObject( context, &JSI_Entity::JSI_class, NULL, NULL );
	JS_SetPrivate( context, entity, v );
	*rval = OBJECT_TO_JSVAL( entity );
	return( JS_TRUE );
}

JSBool getEntityTemplate( JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStrW templateName;
	try
	{
		templateName = g_ScriptingHost.ValueToUCString( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		*rval = JSVAL_NULL;
		JS_ReportError( context, "Invalid template identifier" );
		return( JS_TRUE );
	}
	CBaseEntity* v = g_EntityTemplateCollection.getTemplate( templateName );
	if( !v )
	{
		*rval = JSVAL_NULL;
		JS_ReportError( context, "No such template: %ls", (const wchar_t*)templateName );
		return( JS_TRUE );
	}
	JSObject* baseEntity = JS_NewObject( context, &JSI_BaseEntity::JSI_class, NULL, NULL );
	JS_SetPrivate( context, baseEntity, v );
	*rval = OBJECT_TO_JSVAL( baseEntity );
	return( JS_TRUE );
}

JSBool setTimeout( JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* UNUSEDPARAM(rval) )
{
	assert( argc >= 2 );
	size_t delay;
	try
	{
		delay = g_ScriptingHost.ValueToInt( argv[1] );
	}
	catch( ... )
	{
		JS_ReportError( context, "Invalid timer parameters" );
		return( JS_TRUE );
	}

	switch( JS_TypeOfValue( context, argv[0] ) )
	{
	case JSTYPE_STRING:
	{
		CStr16 fragment = g_ScriptingHost.ValueToUCString( argv[0] );
		g_Scheduler.pushTime( delay, fragment, JS_GetScopeChain( context ) );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( context, argv[0] );
		g_Scheduler.pushTime( delay, fn, JS_GetScopeChain( context ) );
		return( JS_TRUE );
	}
	default:
		JS_ReportError( context, "Invalid timer script" );
		return( JS_TRUE );
	}
}

JSBool setInterval( JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* UNUSEDPARAM(rval) )
{
	assert( argc >= 2 );
	size_t first, interval;
	try
	{
		first = g_ScriptingHost.ValueToInt( argv[1] );
		if( argc == 3 )
		{
			// toDo, first, interval
			interval = g_ScriptingHost.ValueToInt( argv[2] );
		}
		else
		{
			// toDo, interval (first = interval)
			interval = first;
		}
	}
	catch( ... )
	{
		JS_ReportError( context, "Invalid timer parameters" );
		return( JS_TRUE );
	}

	switch( JS_TypeOfValue( context, argv[0] ) )
	{
	case JSTYPE_STRING:
	{
		CStr16 fragment = g_ScriptingHost.ValueToUCString( argv[0] );
		g_Scheduler.pushInterval( first, interval, fragment, JS_GetScopeChain( context ) );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( context, argv[0] );
		g_Scheduler.pushInterval( first, interval, fn, JS_GetScopeChain( context ) );
		return( JS_TRUE );
	}
	default:
		JS_ReportError( context, "Invalid timer script" );
		return( JS_TRUE );
	}
}

JSBool cancelInterval( JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* UNUSEDPARAM(rval) )
{
	g_Scheduler.m_abortInterval = true;
	return( JS_TRUE );
}

JSBool getGUIGlobal( JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* rval )
{
	*rval = OBJECT_TO_JSVAL( g_GUI.GetScriptObject() );
	return( JS_TRUE );
}

JSBool getGlobal( JSContext* UNUSEDPARAM(context), JSObject* globalObject, unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* rval )
{
	*rval = OBJECT_TO_JSVAL( globalObject );
	return( JS_TRUE );
}


extern CStr g_CursorName; // from main.cpp
JSBool setCursor(JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* UNUSEDPARAM(rval))
{
	if (argc != 1)
	{
		assert(! "Invalid parameter count to setCursor");
		return JS_FALSE;
	}
	g_CursorName = g_ScriptingHost.ValueToString(argv[0]);
	return JS_TRUE;
}


extern void kill_mainloop(); // from main.cpp

JSBool exitProgram(JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* UNUSEDPARAM(rval))
{
	kill_mainloop();
	return JS_TRUE;
}

JSBool crash(JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* UNUSEDPARAM(rval))
{
	MICROLOG(L"Crashing at user's request.");
	uintptr_t ptr = 0xDEADC0DE;
	return *(JSBool*) ptr;
}
