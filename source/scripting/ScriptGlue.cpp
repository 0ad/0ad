#include "precompiled.h"

#include "ScriptGlue.h"
#include "CLogger.h"
#include "CConsole.h"
#include "CStr.h"
#include "EntityHandles.h"
#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "Scheduler.h"

#include "Game.h"
#include "Network/Server.h"
#include "Network/Client.h"

#include "ps/i18n.h"

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
	{"setCursor", setCursor, 1, 0, 0 },
	{"startGame", startGame, 0, 0, 0 },
	{"endGame", endGame, 0, 0, 0 },
	{"joinGame", joinGame, 0, 0, 0 },
	{"startServer", startServer, 0, 0, 0 },
	{"loadLanguage", loadLanguage, 1, 0, 0 },
	{"buildTime", buildTime, 0, 0, 0 },

	{"exit", exitProgram, 0, 0, 0 },
	{"crash", crash, 0, 0, 0 },
	{"_mem", js_mem, 0, 0, 0 }, // Intentionally undocumented
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

	LOG( NORMAL, "script", logMessage );

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

// Some globals from main.cpp
extern void CreateGame();
extern CGameAttributes g_GameAttributes;

JSBool startServer(JSContext* cx, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval)
{
	CSocketAddress listenAddress;
	if (argc == 0)
	{
		CNetServer::GetDefaultListenAddress(listenAddress);
	}
	if (argc == 1)
	{
		listenAddress=CSocketAddress(g_ScriptingHost.ValueToInt(argv[0]), IPv4);
	}

	g_Game=new CGame();
	g_NetServer=new CNetServer(&g_NetServerAttributes, g_Game, &g_GameAttributes);
	PS_RESULT res=g_NetServer->Bind(listenAddress);
	if (res != PS_OK)
	{
		LOG(ERROR, "startServer: Bind error: %s", res);
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return JS_TRUE;
	}

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;
}

// More from main.cpp
extern void StartGame();

JSBool joinGame(JSContext* cx, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval)
{
	CStrW username;
	CStr password;
	CStr connectHostName;
	int connectPort=PS_DEFAULT_PORT;
	if (argc >= 2) // One arg; hostname and default port
	{
		username=g_ScriptingHost.ValueToUCString(argv[0]);
		password="";
		connectHostName=g_ScriptingHost.ValueToString(argv[1]);
	}
	else
		return JS_FALSE;

	if (argc == 3)
	{
		connectPort=g_ScriptingHost.ValueToInt(argv[2]);
	}

	g_Game=new CGame();
	g_NetClient=new CNetClient(g_Game, &g_GameAttributes);
	g_NetClient->SetLoginInfo(username, password);
	PS_RESULT res=g_NetClient->BeginConnect(connectHostName.c_str(), connectPort);
	if (res != PS_OK)
	{
		LOG(ERROR, "joinGame: BeginConnect error: %s", res);
		*rval=BOOLEAN_TO_JSVAL(JS_FALSE);
	}
	else
		*rval=BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;
}

JSBool startGame(JSContext* cx, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval)
{
	if (argc != 0)
	{
		return JS_FALSE;
	}
	if (g_NetServer)
		g_NetServer->StartGame();
	else if (g_NetClient) // startGame is invalid on joined games; do nothing and return an error
		return JS_FALSE;
	else
	{
		g_Game=new CGame();
        g_Game->StartGame(&g_GameAttributes);
	}
	*rval=BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;
}

extern void EndGame();
JSBool endGame(JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* UNUSEDPARAM(rval))
{
	EndGame();
	return JS_TRUE;
}


JSBool loadLanguage(JSContext* cx, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval)
{
	if (argc != 1)
	{
		JS_ReportError(cx, "loadLanguage: needs 1 parameter");
		return JS_FALSE;
	}
	CStr lang = g_ScriptingHost.ValueToString(argv[0]);
	I18n::LoadLanguage(lang);

	return JS_TRUE;
}

JSBool buildTime(JSContext* context, JSObject* UNUSEDPARAM(globalObject), unsigned int argc, jsval* argv, jsval* rval)
{
	if (argc > 1)
	{
		JS_ReportError(context, "buildTime: needs 0 or 1 parameter");
		return JS_FALSE;
	}
	// buildTime( ) = "date time"
	// buildTime(0) = "date"
	// buildTime(1) = "time"
	JSString* s = JS_NewStringCopyZ(context,
		argc && argv[0]==JSVAL_ONE ? __TIME__
	  :	argc ? __DATE__
	  : __DATE__" "__TIME__
	);
	*rval = STRING_TO_JSVAL(s);
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
	uintptr_t ptr = 0xDEADC0DE; // oh dear, might this be an invalid pointer?
	return *(JSBool*) ptr;
}

JSBool js_mem(JSContext* UNUSEDPARAM(context), JSObject* UNUSEDPARAM(globalObject), unsigned int UNUSEDPARAM(argc), jsval* UNUSEDPARAM(argv), jsval* UNUSEDPARAM(rval))
{
#ifdef _WIN32
	int left, total;
	extern int GetVRAMInfo(int&, int&);
	if (GetVRAMInfo(left, total))
		g_Console->InsertMessage(L"VRAM: used %d, total %d, free %d", total-left, total, left);
	else
		g_Console->InsertMessage(L"VRAM: failed to detect");
#else
	g_Console->InsertMessage(L"VRAM: [not available on non-Windows]");
#endif
	return JS_TRUE;
}
