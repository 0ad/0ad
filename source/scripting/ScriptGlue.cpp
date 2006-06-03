// This module defines the table of all functions callable from JS.
// it's required by the interpreter; we make use of the opportunity to
// document them all in one spot. we thus obviate having to dig through
// all the other headers. most of the functions are implemented here;
// as for the rest, we only link to their docs (duplication is bad).

#include "precompiled.h"

#include "ScriptGlue.h"
#include "JSConversions.h"
#include "GameEvents.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapWriter.h"
#include "graphics/scripting/JSInterface_Camera.h"
#include "graphics/scripting/JSInterface_LightEnv.h"
#include "gui/CGUI.h"
#include "lib/timer.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Interact.h"
#include "ps/Network/Client.h"
#include "ps/Network/Server.h"
#include "ps/i18n.h"
#include "ps/scripting/JSCollection.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Selection.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "simulation/BaseEntityCollection.h"
#include "simulation/Entity.h"
#include "simulation/EntityFormation.h"
#include "simulation/EntityHandles.h"
#include "simulation/EntityManager.h"
#include "simulation/FormationManager.h"
#include "simulation/LOSManager.h"
#include "simulation/Scheduler.h"
#include "simulation/Simulation.h"
#include "simulation/scripting/JSInterface_BaseEntity.h"
#include "simulation/scripting/JSInterface_Entity.h"

#ifndef NO_GUI
# include "gui/scripting/JSInterface_IGUIObject.h"
#endif

extern CConsole* g_Console;
extern bool g_TerrainModified;


// rationale: the function table is now at the end of the source file to
// avoid the need for forward declarations for every function.

// all normal function wrappers have the following signature:
//   JSBool func(JSContext* cx, JSObject* globalObject, uint argc, jsval* argv, jsval* rval);
// all property accessors have the following signature:
//   JSBool accessor(JSContext* cx, JSObject* globalObject, jsval id, jsval* vp);

// consistent argc checking for normal function wrappers: reports an
// error via JS and returns if number of parameters is incorrect.
// .. require exact number (most common case)
#define REQUIRE_PARAMS(exact_number, func_name)\
	if(argc != exact_number)\
	{\
		JS_ReportError(cx, #func_name ": number of parameters passed doesn't match expected count");\
		return JS_FALSE;\
	}
// .. require 0 params (avoids L4 warning "unused argv param")
#define REQUIRE_NO_PARAMS(func_name)\
	UNUSED2(argv);\
	if(argc != 0)\
	{\
		JS_ReportError(cx, #func_name ": number of parameters passed doesn't match expected count");\
		return JS_FALSE;\
	}
// .. accept at most N params (e.g. buildTime)
#define REQUIRE_MAX_PARAMS(max_number, func_name)\
	if(argc > max_number)\
	{\
		JS_ReportError(cx, #func_name ": too many parameters passed");\
		return JS_FALSE;\
	}
// .. accept at least N params (e.g. issueCommand)
#define REQUIRE_MIN_PARAMS(min_number, func_name)\
	if(argc < min_number)\
	{\
		JS_ReportError(cx, #func_name ": too few parameters passed");\
		return JS_FALSE;\
	}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

// Write values to the log file.
// params: any number of any type.
// returns:
// notes:
// - Each argument is converted to a string and then written to the log.
// - Output is in NORMAL style (see LOG).
JSBool WriteLog(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval))
{
	REQUIRE_MIN_PARAMS(1, WriteLog);

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


//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

// Retrieve the entity currently occupying the specified handle.
// params: handle [int]
// returns: entity
JSBool getEntityByHandle( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS(1, getEntityByHandle);
	*rval = JSVAL_NULL;

	i32 handle;
	try
	{
		handle = ToPrimitive<int>( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid handle" );
		return( JS_TRUE );
	}
	HEntity* v = g_EntityManager.getByHandle( (u16)handle );
	if( !v )
	{
		JS_ReportError( cx, "No entity occupying handle: %d", handle );
		return( JS_TRUE );
	}
	JSObject* entity = (*v)->GetScript();

	*rval = OBJECT_TO_JSVAL( entity );
	return( JS_TRUE );
}


// Look up an EntityTemplate by name.
// params: template name [wstring]
// returns: entity template object
JSBool getEntityTemplate( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(1, getEntityTemplate);
	REQUIRE_MAX_PARAMS(2, getEntityTemplate);
	*rval = JSVAL_NULL;

	CStrW templateName;
	CPlayer* player = 0;

	try
	{
		templateName = g_ScriptingHost.ValueToUCString( argv[0] );
		if( argc == 2 )
		{
			player = ToNative<CPlayer>( argv[1] );
		}
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid template identifier" );
		return( JS_TRUE );
	}

	CBaseEntity* v = g_EntityTemplateCollection.getTemplate( templateName, player );
	if( !v )
	{
		JS_ReportError( cx, "No such template: %s", CStr8(templateName).c_str() );
		return( JS_TRUE );
	}

	*rval = OBJECT_TO_JSVAL( v->GetScript() );
	return( JS_TRUE );
}
//Used to create net messages for formations--msgList.front() is the original message. see issueCommand
void CreateFormationMessage( std::vector<CNetMessage*>& msgList, CNetMessage*& msg, HEntity formationEnt )
{
	if ( formationEnt->GetFormation()->IsDuplication() || msgList.empty() )
		return;
		
	CNetMessage* tmp;
	ENetMessageType type=msg->GetType();
	CEntityList formation = formationEnt->GetFormation()->GetEntityList(); 
	if ( type == NMT_Goto || type == NMT_Run )
	{
		//formationEnt->GetFormation()->BaseToMovement();
		tmp = CNetMessage::CastCommand(msg, formation, NMT_FormationGoto);
	}
	else if ( type == NMT_Generic )
		tmp = CNetMessage::CastCommand(msg, formation, NMT_FormationGeneric);
	else
		return;
	
	msgList.push_back(tmp); 
}

// Issue a command (network message) to an entity or collection.
// params: either an entity- or entity collection object, message ID [int],
//   any further params needed by CNetMessage::CommandFromJSArgs
// returns: command in serialized form [string]
JSBool issueCommand( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(2, issueCommand);
	debug_assert(JSVAL_IS_OBJECT(argv[0]));
	*rval = JSVAL_NULL;
	
	CEntityList entities;

	if (JS_GetClass(JSVAL_TO_OBJECT(argv[0])) == &CEntity::JSI_class)
		entities.push_back( (ToNative<CEntity>(argv[0])) ->me);
	else
		entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));

	//Destroy old notifiers if we're explicitly being reassigned
	for ( size_t i=0; i < entities.size(); i++)
	{
		if ( entities[i]->m_destroyNotifiers )
			entities[i]->DestroyAllNotifiers();
	}
	std::vector<CNetMessage*> messages;
	CNetMessage* msg = CNetMessage::CommandFromJSArgs(entities, cx, argc-1, argv+1);
	if (!msg)
		return JS_TRUE;
	
	messages.push_back(msg);
	//Generate messages for formations
	for (size_t i=0; i < entities.size(); i++ )
	{
		if ( entities[i]->m_formation >= 0)
		{
			CEntityFormation* formation = entities[i]->GetFormation();
			bool duplicate = formation->IsDuplication();
			
			if ( formation->IsLocked() && !duplicate)
			{
				formation->SelectAllUnits();
				CreateFormationMessage(messages, msg, entities[i]);
				formation->SetDuplication(true);
				entities.erase( entities.begin()+i );	//we don't want to be in two orders
			}
			else if ( duplicate )
				entities.erase( entities.begin()+i );
		}
	}
	
	for ( std::vector<CNetMessage*>::iterator it=messages.begin(); it != messages.end(); it++ )
	{
		g_Console->InsertMessage(L"issueCommand: %hs", (*it)->GetString().c_str());
		g_Game->GetSimulation()->QueueLocalCommand(*it);
		*rval = g_ScriptingHost.UCStringToValue((*it)->GetString());
	}

	return JS_TRUE;
}
//Formation stuff
JSBool createEntityFormation( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(2, createEntityFormation);
	
	CEntityList entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));
	CStrW name = ToPrimitive<CStrW>( argv[1] );
	g_FormationManager.CreateFormation( entities, name );
	*rval = JSVAL_VOID;
	return JS_TRUE;

}
JSBool removeFromFormation( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(1, removeFromFormation);
	
	CEntityList entities;
	if (JS_GetClass(JSVAL_TO_OBJECT(argv[0])) == &CEntity::JSI_class)
		entities.push_back( (ToNative<CEntity>(argv[0])) ->me);
	else
		entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));

	*rval = g_FormationManager.RemoveUnitList(entities) ? JS_TRUE : JS_FALSE;
	return JS_TRUE;
}
JSBool lockEntityFormation( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(1, lockEntityFormation);

	CEntity* entity = ToNative<CEntity>( argv[0] );
	entity->GetFormation()->SetLock( ToPrimitive<bool>( argv[1] ) );
	*rval = JSVAL_VOID;
	return JS_TRUE;
}
JSBool isFormationLocked( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(1, isFormationLocked);

	CEntity* entity = ToNative<CEntity>( argv[0] );
	*rval = entity->GetFormation()->IsLocked() ? JS_TRUE : JS_FALSE;
	return JS_TRUE;
}

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

// Register a global handler for the specified DOM event.
// params: event type name [wstring], handler [fragment or function]
// returns: whether it was actually newly registered [bool]
JSBool AddGlobalHandler( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS(2, AddGlobalHandler);

	*rval = BOOLEAN_TO_JSVAL( g_JSGameEvents.AddHandlerJS( cx, argc, argv ) );
	return( JS_TRUE );
}


// Remove a previously registered global handler for the specified DOM event.
// params: event type name [wstring], handler [fragment or function]
// returns: whether it was successfully removed [bool]
JSBool RemoveGlobalHandler( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS(2, RemoveGlobalHandler);

	*rval = BOOLEAN_TO_JSVAL( g_JSGameEvents.RemoveHandlerJS( cx, argc, argv ) );
	return( JS_TRUE );
}


//-----------------------------------------------------------------------------
// Timer
//-----------------------------------------------------------------------------

// Request a callback be executed after the specified delay.
// params: callback [fragment or function], delay in milliseconds [int]
// returns:
// notes:
// - Scripts and functions registered this way are called on the first
//   simulation frame after the specified period has elapsed. If this causes
//   multiple segments of code to be executed in the same frame,
//   relative timing is maintained. Delays of 0 milliseconds cause code to be
//   executed on the following simulation frame. If more than one script or
//   function is scheduled to execute in the same millisecond, the order of
//   execution is undefined. Code is scheduled in simulation time, and is
//   therefore suspended while the game is paused or frozen. Granularity of
//   timing is also limited to 1/(Simulation frame rate); currently 100ms.
//   The called function or script executes in the same scope as the
//   code that called setTimeout (amongst other things, the
//   'this' reference is usually maintained)
JSBool setTimeout( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(2, setTimeout);
	REQUIRE_MAX_PARAMS(3, setTimeout);

	size_t delay;
	try
	{
		delay = ToPrimitive<int>( argv[1] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid timer parameters" );
		return( JS_TRUE );
	}

	JSObject* scope;
	if( argc == 3 )
	{
		if( !JSVAL_IS_OBJECT( argv[2] ) )
		{
			JS_ReportError( cx, "Invalid timer parameters" );
			return( JS_TRUE );
		}
		scope = JSVAL_TO_OBJECT( argv[2] );
	}
	else
	{
		scope = JS_GetScopeChain( cx );
	}

	switch( JS_TypeOfValue( cx, argv[0] ) )
	{
	case JSTYPE_STRING:
	{
		CStrW fragment = g_ScriptingHost.ValueToUCString( argv[0] );
		int id = g_Scheduler.pushTime( delay, fragment, scope );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( cx, argv[0] );
		int id = g_Scheduler.pushTime( delay, fn, scope );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	default:
		JS_ReportError( cx, "Invalid timer script" );
		return( JS_TRUE );
	}
}


// Request a callback be executed periodically.
// params: callback [fragment or function], initial delay in ms [int], period in ms [int]
//   OR callback [fragment or function], period in ms [int] (initial delay = period)
// returns:
// notes:
// - setTimeout's notes apply here as well.
JSBool setInterval( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS(2, setInterval);
	REQUIRE_MAX_PARAMS(3, setInterval);

	size_t first, interval;
	try
	{
		first = ToPrimitive<int>( argv[1] );
		if( argc == 3 )
		{
			// toDo, first, interval
			interval = ToPrimitive<int>( argv[2] );
		}
		else
		{
			// toDo, interval (first = interval)
			interval = first;
		}
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid timer parameters" );
		return( JS_TRUE );
	}

	switch( JS_TypeOfValue( cx, argv[0] ) )
	{
	case JSTYPE_STRING:
	{
		CStrW fragment = g_ScriptingHost.ValueToUCString( argv[0] );
		int id = g_Scheduler.pushInterval( first, interval, fragment, JS_GetScopeChain( cx ) );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( cx, argv[0] );
		int id = g_Scheduler.pushInterval( first, interval, fn, JS_GetScopeChain( cx ) );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	default:
		JS_ReportError( cx, "Invalid timer script" );
		return( JS_TRUE );
	}
}


// Cause all periodic functions registered via setInterval to
//   no longer be called.
// params:
// returns:
// notes:
// - Execution continues until the end of the triggered function or
//   script fragment, but is not triggered again.
JSBool cancelInterval( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_NO_PARAMS(cancelInterval);

	g_Scheduler.m_abortInterval = true;
	return( JS_TRUE );
}


// Cause the scheduled task (timeout or interval) with the given ID to
//   no longer be called.
// params:
// returns:
// notes:
// - Execution continues until the end of the triggered function or
//   script fragment, but is not triggered again.
JSBool cancelTimer( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_MIN_PARAMS(1, cancelTimer);
	REQUIRE_MAX_PARAMS(1, cancelTimer);

	try
	{
		int id = ToPrimitive<int>( argv[0] );
		g_Scheduler.cancelTask( id );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid ID parameter" );
		return( JS_TRUE );
	}

	return( JS_TRUE );
}


//-----------------------------------------------------------------------------
// Game Setup
//-----------------------------------------------------------------------------

// Create a new network server object.
// params:
// returns: net server object
JSBool createServer(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(createServer);

	if( !g_Game )
		g_Game = new CGame();
	if( !g_NetServer )
		g_NetServer = new CNetServer(g_Game, &g_GameAttributes);

	*rval = OBJECT_TO_JSVAL(g_NetServer->GetScript());
	return( JS_TRUE );
}


// Create a new network client object.
// params:
// returns: net client object
JSBool createClient(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(createClient);

	if( !g_Game )
		g_Game = new CGame();
	if( !g_NetClient )
		g_NetClient = new CNetClient(g_Game, &g_GameAttributes);

	*rval = OBJECT_TO_JSVAL(g_NetClient->GetScript());
	return( JS_TRUE );
}


// Begin the process of starting a game.
// params:
// returns: success [bool]
// notes:
// - Performs necessary initialization while calling back into the
//   main loop, so the game remains responsive to display+user input.
// - When complete, the engine calls the reallyStartGame JS function.
// TODO: Replace startGame with create(Game|Server|Client)/game.start() -
//   after merging CGame and CGameAttributes
JSBool startGame(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(startGame);

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	// Hosted MP Game
	if (g_NetServer)
		*rval = BOOLEAN_TO_JSVAL(g_NetServer->StartGame() == 0);
	// Joined MP Game: startGame is invalid - do nothing
	else if (g_NetClient)
	{
	}
	// Start an SP Game Session
	else if (!g_Game)
	{
		g_Game = new CGame();
		PSRETURN ret = g_Game->StartGame(&g_GameAttributes);
		if (ret != PSRETURN_OK)
		{
			// Failed to start the game - destroy it, and return false

			delete g_Game;
			g_Game = NULL;

			*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
			return( JS_TRUE );
		}
	}

	return( JS_TRUE );
}


// Immediately ends the current game (if any).
// params:
// returns:
JSBool endGame(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval))
{
	REQUIRE_NO_PARAMS(endGame);

	EndGame();
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Internationalization
//-----------------------------------------------------------------------------

// these remain here instead of in the i18n tree because they are
// really related to the engine's use of them, as opposed to i18n itself.
// contrariwise, translate() cannot be moved here because that would
// make i18n dependent on this code and therefore harder to reuse.

// Replaces the current language (locale) with a new one.
// params: language id [string] as in I18n::LoadLanguage
// returns:
JSBool loadLanguage(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval))
{
	REQUIRE_PARAMS(1, loadLanguage);

	CStr lang = g_ScriptingHost.ValueToString(argv[0]);
	I18n::LoadLanguage(lang);

	return JS_TRUE;
}


// Return identifier of the current language (locale) in use.
// params:
// returns: language id [string] as in I18n::LoadLanguage
JSBool getLanguageID(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(getLanguageID);
	*rval = JSVAL_NULL;

	JSString* s = JS_NewStringCopyZ(cx, I18n::CurrentLanguageName());
	if (!s)
	{
		JS_ReportError(cx, "Error creating string");
		return JS_FALSE;
	}
	*rval = STRING_TO_JSVAL(s);
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------


// Deliberately cause the game to crash.
// params:
// returns:
// notes:
// - currently implemented via access violation (read of address 0)
// - useful for testing the crashlog/stack trace code.
JSBool crash(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval))
{
	REQUIRE_NO_PARAMS(crash);

	MICROLOG(L"Crashing at user's request.");
	return *(JSBool*)0;
}


// Force a JS GC (garbage collection) cycle to take place immediately.
// params:
// returns: true [bool]
// notes:
// - writes an indication of how long this took to the console.
JSBool forceGC(JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(forceGC);

	double time = get_time();
	JS_GC(cx);
	time = get_time() - time;
	g_Console->InsertMessage(L"Garbage collection completed in: %f", time);
	*rval = JSVAL_TRUE;
	return JS_TRUE ;
}



//-----------------------------------------------------------------------------
// GUI
//-----------------------------------------------------------------------------

// Returns the sort-of-global object associated with the current GUI.
// params:
// returns: global object
// notes:
// - Useful for accessing an object from another scope.
JSBool getGUIGlobal(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval)
{
	REQUIRE_NO_PARAMS(getGUIGlobal);

	*rval = OBJECT_TO_JSVAL(g_GUI.GetScriptObject());
	return JS_TRUE;
}

// Resets the entire GUI state and reloads the XML files.
// params:
// returns:
JSBool resetGUI(JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval))
{
	REQUIRE_NO_PARAMS(resetGUI);

	// Slightly unpleasant code, because CGUI is a Singleton but we don't really
	// want it to be
	g_GUI.Destroy();
	delete &g_GUI;
	new CGUI;
	GUI_Init();
	g_GUI.SendEventToAll("load");

	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Misc. Engine Interface
//-----------------------------------------------------------------------------

// Return the global frames-per-second value.
// params:
// returns: FPS [int]
// notes:
// - This value is recalculated once a frame. We take special care to
//   filter it, so it is both accurate and free of jitter.
JSBool getFPS( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS(getFPS);

	*rval = INT_TO_JSVAL(fps);
	return JS_TRUE;
}


// Cause the game to exit gracefully.
// params:
// returns:
// notes:
// - Exit happens after the current main loop iteration ends
//   (since this only sets a flag telling it to end)
JSBool exitProgram( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_NO_PARAMS(exitProgram);

	kill_mainloop();
	return JS_TRUE;
}


// Write an indication of total/available video RAM to console.
// params:
// returns:
// notes:
// - Not supported on all platforms.
// - Only a rough approximation; do not base low-level decisions
//   ("should I allocate one more texture?") on this.
JSBool vmem( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_NO_PARAMS(vmem);

#if OS_WIN
	int left, total;
	if (GetVRAMInfo(left, total))
		g_Console->InsertMessage(L"VRAM: used %d, total %d, free %d", total-left, total, left);
	else
		g_Console->InsertMessage(L"VRAM: failed to detect");
#else
	g_Console->InsertMessage(L"VRAM: [not available on non-Windows]");
#endif
	return JS_TRUE;
}


// Change the mouse cursor.
// params: cursor name [string] (i.e. basename of definition file and texture)
// returns:
// notes:
// - Cursors are stored in "art\textures\cursors"
JSBool setCursor( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_PARAMS(1, setCursor);

	g_CursorName = g_ScriptingHost.ValueToString(argv[0]);
	return JS_TRUE;
}


// Trigger a rewrite of all maps.
// params:
// returns:
// notes:
// - Usefulness is unclear. If you need it, consider renaming this and updating the docs.
JSBool _rewriteMaps( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_NO_PARAMS(_rewriteMaps);

	g_Game->GetWorld()->RewriteMap();
	return JS_TRUE;
}


// Change the LOD bias.
// params: LOD bias [float]
// returns:
// notes:
// - value is as required by GL_TEXTURE_LOD_BIAS.
// - useful for adjusting image "sharpness" (since it affects which mipmap level is chosen)
JSBool _lodbias( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_PARAMS(1, _lodbias);

	g_Renderer.SetOptionFloat(CRenderer::OPT_LODBIAS, ToPrimitive<float>(argv[0]));
	return JS_TRUE;
}


// Focus the game camera on a given position.
// params: target position vector [CVector3D]
// returns: success [bool]
JSBool setCameraTarget( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS(1, setCameraTarget);
	*rval = JSVAL_NULL;

	CVector3D* target = ToNative<CVector3D>( argv[0] );
	if(!target)
	{
		JS_ReportError( cx, "Invalid camera target" );
		return( JS_TRUE );
	}
	g_Game->GetView()->SetCameraTarget( *target );

	*rval = JSVAL_TRUE;
	return( JS_TRUE );
}


//-----------------------------------------------------------------------------
// Miscellany
//-----------------------------------------------------------------------------

// Return the date/time at which the current executable was compiled.
// params: none (-> "date time") OR
//   what to display [int]: 0 (-> "date"); 1 (-> "time")
// returns: date and/or time [string]
// notes:
// - Displayed on main menu screen; tells non-programmers which auto-build
//   they are running. Could also be determined via .EXE file properties,
//   but that's a bit more trouble.
// - To be exact, the date/time returned is when scriptglue.cpp was
//   last compiled; since the auto-build does full rebuilds, that is moot.
JSBool buildTime( JSContext* cx, JSObject*, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MAX_PARAMS(1, buildTime);

	// buildTime( ) = "date time"
	// buildTime(0) = "date"
	// buildTime(1) = "time"
	JSString* s = JS_NewStringCopyZ(cx,
		argc && argv[0]==JSVAL_ONE ? __TIME__
		:	argc ? __DATE__
		: __DATE__" "__TIME__
		);
	*rval = STRING_TO_JSVAL(s);
	return JS_TRUE;
}


// Return distance between 2 points.
// params: 2 position vectors [CVector3D]
// returns: Euclidean distance [float]
JSBool v3dist( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS(2, v3dist);

	CVector3D* a = ToNative<CVector3D>( argv[0] );
	CVector3D* b = ToNative<CVector3D>( argv[1] );
	float dist = ( *a - *b ).GetLength();
	*rval = ToJSVal( dist );
	return( JS_TRUE );
}


// Returns the global object.
// params:
// returns: global object
// notes:
// - Useful for accessing an object from another scope.
JSBool getGlobal( JSContext* cx, JSObject* globalObject, uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS(getGlobal);

	*rval = OBJECT_TO_JSVAL( globalObject );
	return( JS_TRUE );
}

// Activates the building placement cursor for placing a building. The currently selected units
// are then ordered to construct the building if it is placed.
// params: templateName - the name of the entity to place.
// returns: true if cursor was activated, false if cursor was already active.
JSBool startPlacing( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	CStrW name;
	if(argc == 0) {
		name = L"hele_ho";			// save some typing during testing
	}
	else {
		if(!ToPrimitive<CStrW>( g_ScriptingHost.GetContext(), argv[0], name ))
		{
			JS_ReportError( cx, "Invalid template name argument" );
			*rval = JSVAL_NULL;
			return( JS_FALSE );
		}
	}

	*rval = g_BuildingPlacer.activate(name) ? JS_TRUE : JS_FALSE;

	return( JS_TRUE );
}

// Toggles drawing the sky
JSBool toggleSky( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS( toggleSky );
	g_Renderer.GetSkyManager()->m_RenderSky = !g_Renderer.GetSkyManager()->m_RenderSky;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Toggles drawing the water plane
JSBool toggleWater( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS( toggleWater );
	g_Renderer.GetWaterManager()->m_RenderWater = !g_Renderer.GetWaterManager()->m_RenderWater;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the water plane height
JSBool setWaterHeight( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS( 1, setWaterHeight );
	float newHeight;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], newHeight ))
	{
		JS_ReportError( cx, "Invalid water height argument" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterHeight = newHeight;
	g_TerrainModified = true;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Gets the water plane height
JSBool getWaterHeight( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS( getWaterHeight );
	*rval = ToJSVal(g_Renderer.GetWaterManager()->m_WaterHeight);
	return( JS_TRUE );
}

// Sets the water color
JSBool setWaterColor( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS( 3, setWaterColor );
	float r,g,b;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], r )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[1], g )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[2], b ))
	{
		JS_ReportError( cx, "Invalid arguments" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterColor = CColor(r, g, b, 1.0f);
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the max water alpha (achieved when it is at WaterFullDepth or deeper)
JSBool setWaterMaxAlpha( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS( 1, setWaterMaxAlpha );
	float val;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], val ))
	{
		JS_ReportError( cx, "Invalid argument" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterMaxAlpha = val;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the water full depth (when it is colored WaterMaxAlpha)
JSBool setWaterFullDepth( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS( 1, setWaterFullDepth );
	float val;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], val ))
	{
		JS_ReportError( cx, "Invalid argument" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterFullDepth = val;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the water alpha offset (added to tweak water alpha near the shore)
JSBool setWaterAlphaOffset( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_PARAMS( 1, setWaterAlphaOffset );
	float val;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], val ))
	{
		JS_ReportError( cx, "Invalid argument" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterAlphaOffset = val;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Is the game paused?
JSBool isPaused( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS( isPaused );

	if( !g_Game )
	{
		JS_ReportError( cx, "Game is not started" );
		return JS_FALSE;
	}

	*rval = g_Game->m_Paused ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE ;
}

// Pause/unpause the game
JSBool setPaused( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* UNUSED(rval) )
{
	REQUIRE_PARAMS( 1, setPaused );

	if( !g_Game )
	{
		JS_ReportError( cx, "Game is not started" );
		return JS_FALSE;
	}

	try
	{
		g_Game->m_Paused = ToPrimitive<bool>( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid parameter to setPaused" );
	}

	return  JS_TRUE;
}

// Get game time
JSBool getGameTime( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_NO_PARAMS( getGameTime );

	if( !g_Game )
	{
		JS_ReportError( cx, "Game is not started" );
		return JS_FALSE;
	}

	*rval = ToJSVal(g_Game->GetTime());
	return JS_TRUE;
}

// Reveal map
JSBool revealMap( JSContext* cx, JSObject* UNUSED(globalObject), uint argc, jsval* argv, jsval* rval )
{
	REQUIRE_MIN_PARAMS( 0, revealMap );
	REQUIRE_MAX_PARAMS( 1, revealMap );

	uint newValue;
	if(argc == 0)
	{
		newValue = 2;
	}
	else if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], newValue ) || newValue > 2 || newValue < 0)
	{
		JS_ReportError( cx, "Invalid argument (should be 0, 1 or 2)" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}

	g_Game->GetWorld()->GetLOSManager()->m_LOSSetting = newValue;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

//-----------------------------------------------------------------------------
// function table
//-----------------------------------------------------------------------------

// the JS interpreter expects the table to contain 5-tuples as follows:
// - name the function will be called as from script;
// - function which will be called;
// - number of arguments this function expects
// - Flags (deprecated, always zero)
// - Extra (reserved for future use, always zero)
//
// we simplify this a bit with a macro:
#define JS_FUNC(script_name, cpp_function, min_params) { #script_name, cpp_function, min_params, 0, 0 },

JSFunctionSpec ScriptFunctionTable[] =
{
	// Console
	JS_FUNC(writeConsole, JSI_Console::writeConsole, 1)	// external

	// Entity
	JS_FUNC(getEntityByHandle, getEntityByHandle, 1)
	JS_FUNC(getEntityTemplate, getEntityTemplate, 1)
	JS_FUNC(issueCommand, issueCommand, 2)
	JS_FUNC(startPlacing, startPlacing, 1)

	JS_FUNC(createEntityFormation, createEntityFormation, 2)
	JS_FUNC(removeFromFormation, removeFromFormation, 1)
	JS_FUNC(lockEntityFormation, lockEntityFormation, 1)
	JS_FUNC(isFormationLocked, isFormationLocked, 1)

	// Camera
	JS_FUNC(setCameraTarget, setCameraTarget, 1)

	// Sky
	JS_FUNC(toggleSky, toggleSky, 0)

	// Water
	JS_FUNC(toggleWater, toggleWater, 0)
	JS_FUNC(setWaterHeight, setWaterHeight, 1)
	JS_FUNC(getWaterHeight, getWaterHeight, 0)
	JS_FUNC(setWaterColor, setWaterColor, 0)
	JS_FUNC(setWaterMaxAlpha, setWaterMaxAlpha, 0)
	JS_FUNC(setWaterFullDepth, setWaterFullDepth, 0)
	JS_FUNC(setWaterAlphaOffset, setWaterAlphaOffset, 0)

	// GUI
#ifndef NO_GUI
	JS_FUNC(getGUIObjectByName, JSI_IGUIObject::getByName, 1)	// external
	JS_FUNC(getGUIGlobal, getGUIGlobal, 0)
	JS_FUNC(resetGUI, resetGUI, 0)
#endif

	// Events
	JS_FUNC(addGlobalHandler, AddGlobalHandler, 2)
	JS_FUNC(removeGlobalHandler, RemoveGlobalHandler, 2)

	// Timer
	JS_FUNC(setTimeout, setTimeout, 2)
	JS_FUNC(setInterval, setInterval, 2)
	JS_FUNC(cancelInterval, cancelInterval, 0)
	JS_FUNC(cancelTimer, cancelTimer, 0)

	// Game Setup
	JS_FUNC(startGame, startGame, 0)
	JS_FUNC(endGame, endGame, 0)
	JS_FUNC(createClient, createClient, 0)
	JS_FUNC(createServer, createServer, 0)

	// VFS (external)
	JS_FUNC(buildFileList, JSI_VFS::BuildFileList, 1)
	JS_FUNC(getFileMTime, JSI_VFS::GetFileMTime, 1)
	JS_FUNC(getFileSize, JSI_VFS::GetFileSize, 1)
	JS_FUNC(readFile, JSI_VFS::ReadFile, 1)
	JS_FUNC(readFileLines, JSI_VFS::ReadFileLines, 1)
	JS_FUNC(archiveBuilderCancel, JSI_VFS::ArchiveBuilderCancel, 1)

	// Internationalization
	JS_FUNC(loadLanguage, loadLanguage, 1)
	JS_FUNC(getLanguageID, getLanguageID, 0)
	// note: i18n/ScriptInterface.cpp registers translate() itself.
	// rationale: see implementation section above.

	// Debug
	JS_FUNC(crash, crash, 0)
	JS_FUNC(forceGC, forceGC, 0)
	JS_FUNC(revealMap, revealMap, 1)

	// Misc. Engine Interface
	JS_FUNC(writeLog, WriteLog, 1)
	JS_FUNC(exit, exitProgram, 0)
	JS_FUNC(isPaused, isPaused, 0)
	JS_FUNC(setPaused, setPaused, 1)
	JS_FUNC(getGameTime, getGameTime, 0)
	JS_FUNC(vmem, vmem, 0)
	JS_FUNC(_rewriteMaps, _rewriteMaps, 0)
	JS_FUNC(_lodbias, _lodbias, 0)
	JS_FUNC(setCursor, setCursor, 1)
	JS_FUNC(getFPS, getFPS, 0)

	// Miscellany
	JS_FUNC(v3dist, v3dist, 2)
	JS_FUNC(buildTime, buildTime, 0)
	JS_FUNC(getGlobal, getGlobal, 0)

	// end of table marker
	{0, 0, 0, 0, 0}
};
#undef JS_FUNC


//-----------------------------------------------------------------------------
// property accessors
//-----------------------------------------------------------------------------

JSBool GetEntitySet( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(argv), jsval* vp )
{
	std::vector<HEntity>* extant = g_EntityManager.getExtant();

	*vp = OBJECT_TO_JSVAL( EntityCollection::Create( *extant ) );
	delete( extant );

	return( JS_TRUE );
}


JSBool GetPlayerSet( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	std::vector<CPlayer*>* players = g_Game->GetPlayers();

	*vp = OBJECT_TO_JSVAL( PlayerCollection::Create( *players ) );

	return( JS_TRUE );
}


JSBool GetLocalPlayer( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	*vp = OBJECT_TO_JSVAL( g_Game->GetLocalPlayer()->GetScript() );
	return( JS_TRUE );
}


JSBool GetGaiaPlayer( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	*vp = OBJECT_TO_JSVAL( g_Game->GetPlayer( 0 )->GetScript() );
	return( JS_TRUE );
}


JSBool SetLocalPlayer( JSContext* cx, JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	CPlayer* newLocalPlayer = ToNative<CPlayer>( *vp );

	if( !newLocalPlayer )
	{
		JS_ReportError( cx, "Not a valid Player." );
		return( JS_TRUE );
	}

	g_Game->SetLocalPlayer( newLocalPlayer );
	return( JS_TRUE );
}


JSBool GetGameView( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	if (g_Game)
		*vp = OBJECT_TO_JSVAL( g_Game->GetView()->GetScript() );
	else
		*vp = JSVAL_NULL;
	return( JS_TRUE );
}


JSBool GetRenderer( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	if (CRenderer::IsInitialised())
		*vp = OBJECT_TO_JSVAL( g_Renderer.GetScript() );
	else
		*vp = JSVAL_NULL;
	return( JS_TRUE );
}


enum ScriptGlobalTinyIDs
{
	GLOBAL_SELECTION,
	GLOBAL_GROUPSARRAY,
	GLOBAL_CAMERA,
	GLOBAL_CONSOLE,
	GLOBAL_LIGHTENV
};

// shorthand
#define PERM  JSPROP_PERMANENT
#define CONST JSPROP_READONLY

JSPropertySpec ScriptGlobalTable[] =
{
	{ "selection"  , GLOBAL_SELECTION,   PERM,         JSI_Selection::getSelection, JSI_Selection::setSelection },
	{ "groups"     , GLOBAL_GROUPSARRAY, PERM,         JSI_Selection::getGroups,    JSI_Selection::setGroups },
	{ "camera"     , GLOBAL_CAMERA,      PERM,         JSI_Camera::getCamera,       JSI_Camera::setCamera },
	{ "console"    , GLOBAL_CONSOLE,     PERM | CONST, JSI_Console::getConsole,     0 },
	{ "lightenv"   , GLOBAL_LIGHTENV,    PERM,         JSI_LightEnv::getLightEnv,   JSI_LightEnv::setLightEnv },
	{ "entities"   , 0,                  PERM | CONST, GetEntitySet,                0 },
	{ "players"    , 0,                  PERM | CONST, GetPlayerSet,                0 },
	{ "localPlayer", 0,                  PERM        , GetLocalPlayer,              SetLocalPlayer },
	{ "gaiaPlayer" , 0,                  PERM | CONST, GetGaiaPlayer,               0 },
	{ "gameView"   , 0,                  PERM | CONST, GetGameView,                 0 },
	{ "renderer"   , 0,                  PERM | CONST, GetRenderer,                 0 },

	// end of table marker
	{ 0, 0, 0, 0, 0 },
};
