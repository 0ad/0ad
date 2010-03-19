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

// This module defines the table of all functions callable from JS.
// it's required by the interpreter; we make use of the opportunity to
// document them all in one spot. we thus obviate having to dig through
// all the other headers. most of the functions are implemented here;
// as for the rest, we only link to their docs (duplication is bad).

#include "precompiled.h"

#include "ScriptGlue.h"
#include "JSConversions.h"
#include "GameEvents.h"

#include "ScriptableComplex.inl"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapWriter.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "graphics/scripting/JSInterface_Camera.h"
#include "graphics/scripting/JSInterface_LightEnv.h"
#include "lib/timer.h"
#include "lib/svn_revision.h"
#include "lib/frequency_filter.h"
#include "maths/scripting/JSInterface_Vector3D.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Game.h"
#include "ps/Globals.h"	// g_frequencyFilter
#include "ps/GameSetup/GameSetup.h"
#include "ps/Hotkey.h"
#include "ps/Interact.h"
#include "ps/ProfileViewer.h"
#include "ps/i18n.h"
#include "ps/scripting/JSCollection.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Selection.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation/Entity.h"
#include "simulation/EntityFormation.h"
#include "simulation/EntityHandles.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityTemplate.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/FormationManager.h"
#include "simulation/LOSManager.h"
#include "simulation/Scheduler.h"
#include "simulation/Simulation.h"
#include "simulation/TechnologyCollection.h"
#include "simulation/TriggerManager.h"

#define LOG_CATEGORY L"script"
extern bool g_TerrainModified;


// rationale: the function table is now at the end of the source file to
// avoid the need for forward declarations for every function.

// all normal function wrappers have the following signature:
//   JSBool func(JSContext* cx, JSObject* globalObject, uintN argc, jsval* argv, jsval* rval);
// all property accessors have the following signature:
//   JSBool accessor(JSContext* cx, JSObject* globalObject, jsval id, jsval* vp);


//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

// Write values to the log file.
// params: any number of any type.
// returns:
// notes:
// - Each argument is converted to a string and then written to the log.
// - Output is in NORMAL style (see LOG).
JSBool WriteLog(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_PARAMS(1);

	CStrW logMessage;

	for (int i = 0; i < (int)argc; i++)
	{
		try
		{
			CStrW arg = g_ScriptingHost.ValueToUCString( argv[i] );
			logMessage += arg;
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			// Do nothing.
		}
	}

	LOG(CLogger::Normal, LOG_CATEGORY, L"%ls", logMessage.c_str());

	*rval = JSVAL_TRUE;
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

// Retrieve the entity currently occupying the specified handle.
// params: handle [int]
// returns: entity
JSBool GetEntityByUnitID( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);
	*rval = JSVAL_NULL;

	int uid;
	try
	{
		uid = ToPrimitive<int>( argv[0] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid parameter" );
		return( JS_TRUE );
	}

	CUnit* unit = g_Game->GetWorld()->GetUnitManager().FindByID( uid );
	if( !unit || !unit->GetEntity() )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}

	*rval = OBJECT_TO_JSVAL( unit->GetEntity()->GetScript() );
	return( JS_TRUE );
}


// Look up an EntityTemplate by name.
// params: template name [wstring]
// returns: entity template object
JSBool GetEntityTemplate( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAM_RANGE(1, 2);
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

	CEntityTemplate* v = g_EntityTemplateCollection.GetTemplate( templateName, player );
	if( !v )
	{
		JS_ReportError( cx, "No such template: %s", CStr(templateName).c_str() );
		return( JS_TRUE );
	}

	*rval = OBJECT_TO_JSVAL( v->GetScript() );
	return( JS_TRUE );
}

JSBool GetPlayerUnitCount( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);
	const size_t playerNum = ToPrimitive<size_t>( argv[0] );
	const CStrW unitName = ToPrimitive<CStrW>( argv[1] );

	const int unitCount = g_EntityManager.GetPlayerUnitCount(playerNum, unitName);
	*rval = ToJSVal( unitCount );
	return JS_TRUE;
}


//Used to create net messages for formations--msgList.front() is the original message. see IssueCommand
void CreateFormationMessage( std::vector<CNetMessage*>& msgList, CNetMessage* msg, CEntityList& formation )
{
	CNetMessage* retMsg;
	const int type = msg->GetType();

	if ( type == NMT_GOTO )
	{
		//formationEnt->GetFormation()->BaseToMovement();
		CGotoMessage* tmp = static_cast<CGotoMessage*>(msg);
		retMsg = CNetMessage::CreatePositionMessage( formation, NMT_FORMATION_GOTO, 
						CVector2D(tmp->m_TargetX, tmp->m_TargetY) );
	}
	else if( type == NMT_RUN )
	{
		CGotoMessage* tmp = static_cast<CGotoMessage*>(msg);
		retMsg = CNetMessage::CreatePositionMessage( formation, NMT_FORMATION_GOTO, 
						CVector2D(tmp->m_TargetX, tmp->m_TargetY) );
	}
	else if ( type == NMT_CONTACT_ACTION )
	{
		CContactActionMessage* tmp = static_cast<CContactActionMessage*>(msg);
		retMsg = CNetMessage::CreateEntityIntMessage(formation, NMT_FORMATION_CONTACT_ACTION, 
					tmp->m_Target, tmp->m_Action);
	}
	else
		return;
	
	msgList.push_back(retMsg); 
}

// Issue a command (network message) to an entity or collection.
// params: either an entity- or entity collection object, message ID [int],
//   any further params needed by CNetMessage::CommandFromJSArgs
// returns: command in serialized form [string]
JSBool IssueCommand( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	// at least one for target object, one for isQueued, and then 1 or more for the CommandFromJSArgs
	JSU_REQUIRE_MIN_PARAMS(3);

	JSU_ASSERT(JSVAL_IS_OBJECT(argv[0]), "Argument 0 must be an entity collection.");
	*rval = JSVAL_NULL;
	
	CEntityList entities, msgEntities;

	if (JS_InstanceOf(cx, JSVAL_TO_OBJECT(argv[0]), &CEntity::JSI_class, NULL))
		entities.push_back( (ToNative<CEntity>(argv[0])) ->me);
	else
		entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));

	typedef std::map<size_t, CEntityList> EntityStore;
	EntityStore entityStore;

	bool isQueued = ToPrimitive<bool>(argv[1]);

	//Destroy old notifiers if we're explicitly being reassigned
	for ( size_t i=0; i < entities.size(); i++)
	{
		if ( entities[i]->entf_get(ENTF_DESTROY_NOTIFIERS))
			entities[i]->DestroyAllNotifiers();
	}

	std::vector<CNetMessage*> messages;
	
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
				entityStore[entities[i]->m_formation] = formation->GetEntityList();
				formation->SetDuplication(true);
			}
		}
		else
			msgEntities.push_back( entities[i] );
	}
	CNetMessage* msg = CNetMessage::CommandFromJSArgs(msgEntities, cx, argc-2, argv+2, isQueued);
	if (!msg)
	{
		delete msg;
		return JS_TRUE;
	}
	messages.push_back(msg);
	
	for ( EntityStore::iterator it=entityStore.begin(); it!=entityStore.end(); it++)
		CreateFormationMessage(messages, msg, it->second);
	
	for ( std::vector<CNetMessage*>::iterator it=messages.begin(); it != messages.end(); it++ )
	{
		g_Console->InsertMessage(L"IssueCommand: %hs", (*it)->ToString().c_str());
		*rval = g_ScriptingHost.UCStringToValue((*it)->ToString());
		g_Game->GetSimulation()->QueueLocalCommand(*it);
	}

	return JS_TRUE;
}


// Get the state of a given hotkey (from the hotkeys file)
JSBool isOrderQueued( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	*rval = ToJSVal(hotkeys[HOTKEY_ORDER_QUEUE]);
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// formations
//-----------------------------------------------------------------------------

JSBool CreateEntityFormation( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);
	
	CEntityList entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));
	CStrW name = ToPrimitive<CStrW>( argv[1] );
	g_FormationManager.CreateFormation( entities, name );
	*rval = JSVAL_VOID;
	return JS_TRUE;

}

JSBool RemoveFromFormation( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);
	
	CEntityList entities;
	if (JS_InstanceOf(cx, JSVAL_TO_OBJECT(argv[0]), &CEntity::JSI_class, NULL))
		entities.push_back( (ToNative<CEntity>(argv[0])) ->me);
	else
		entities = *EntityCollection::RetrieveSet(cx, JSVAL_TO_OBJECT(argv[0]));

	*rval = g_FormationManager.RemoveUnitList(entities) ? JS_TRUE : JS_FALSE;
	return JS_TRUE;
}

JSBool LockEntityFormation( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);

	CEntity* entity = ToNative<CEntity>( argv[0] );
	entity->GetFormation()->SetLock( ToPrimitive<bool>( argv[1] ) );
	*rval = JSVAL_VOID;
	return JS_TRUE;
}

JSBool IsFormationLocked( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);

	CEntity* entity = ToNative<CEntity>( argv[0] );
	*rval = entity->GetFormation()->IsLocked() ? JS_TRUE : JS_FALSE;
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Techs
//-----------------------------------------------------------------------------

JSBool GetTechnology( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);
	
	CStrW name;
	CPlayer* player;
	
	try
	{
		name = g_ScriptingHost.ValueToUCString( argv[0] );
		player = ToNative<CPlayer>( argv[1] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid parameters for GetTechnology (expected name and player)" );
		return( JS_TRUE );
	}

	*rval = JSVAL_NULL;

	CTechnology* tech = g_TechnologyCollection.GetTechnology( name, player );	
	if ( tech )
		*rval = ToJSVal( tech );
	else
		g_Console->InsertMessage( L"Warning: Invalid tech template name \"%ls\" passed for GetTechnology()", name.c_str() );

	return JS_TRUE;
}

//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

// Register a global handler for the specified DOM event.
// params: event type name [wstring], handler [fragment or function]
// returns: whether it was actually newly registered [bool]
JSBool AddGlobalHandler( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);

	*rval = BOOLEAN_TO_JSVAL( g_JSGameEvents.AddHandlerJS( cx, argc, argv ) );
	return( JS_TRUE );
}


// Remove a previously registered global handler for the specified DOM event.
// params: event type name [wstring], handler [fragment or function]
// returns: whether it was successfully removed [bool]
JSBool RemoveGlobalHandler( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);

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
//   code that called SetTimeout (amongst other things, the
//   'this' reference is usually maintained)
JSBool SetTimeout( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAM_RANGE(2, 3);

	int delay;
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
		int id = g_Scheduler.PushTime( delay, fragment, scope );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( cx, argv[0] );
		int id = g_Scheduler.PushTime( delay, fn, scope );
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
// - SetTimeout's notes apply here as well.
JSBool SetInterval( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAM_RANGE(2, 3);

	int first, interval;
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
		int id = g_Scheduler.PushInterval( first, interval, fragment, JS_GetScopeChain( cx ) );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	case JSTYPE_FUNCTION:
	{
		JSFunction* fn = JS_ValueToFunction( cx, argv[0] );
		int id = g_Scheduler.PushInterval( first, interval, fn, JS_GetScopeChain( cx ) );
		*rval = INT_TO_JSVAL( id );
		return( JS_TRUE );
	}
	default:
		JS_ReportError( cx, "Invalid timer script" );
		return( JS_TRUE );
	}
}

// Cause all periodic functions registered via SetInterval to
//   no longer be called.
// params:
// returns:
// notes:
// - Execution continues until the end of the triggered function or
//   script fragment, but is not triggered again.
JSBool CancelInterval( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

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
JSBool CancelTimer( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);

	try
	{
		int id = ToPrimitive<int>( argv[0] );
		g_Scheduler.CancelTask( id );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Invalid ID parameter" );
		return( JS_TRUE );
	}

	return( JS_TRUE );
}

//Set the simulation rate scalar-time becomes time * SimRate.
//Params: rate [float] : sets SimRate
JSBool SetSimRate(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_PARAMS(1);

	g_Game->SetSimRate( ToPrimitive<float>(argv[0]) );
	return JS_TRUE;
}

//Generate a random float in [0, 1) using the simulation's random generator
JSBool SimRand(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	*rval = ToJSVal( g_Game->GetSimulation()->RandFloat() );
	return JS_TRUE;
}

//Generate a random float int between 0 and the given number - 1 using the simulation's RNG
JSBool SimRandInt(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_PARAMS(1);
	JSU_ASSERT(JSVAL_IS_INT(argv[0]), "SimRandInt(): first parameter must be an int");

	*rval = ToJSVal( g_Game->GetSimulation()->RandInt(ToPrimitive<int>(argv[0])) );
	return JS_TRUE;
}

// Script profiling functions: Begin timing a piece of code with StartJsTimer(num)
// and stop timing with StopJsTimer(num). The results will be printed to stdout
// when the game exits.

static const size_t MAX_JS_TIMERS = 20;
static TimerUnit js_start_times[MAX_JS_TIMERS];
static TimerUnit js_timer_overhead;
static TimerClient js_timer_clients[MAX_JS_TIMERS];
static wchar_t js_timer_descriptions_buf[MAX_JS_TIMERS * 12];	// depends on MAX_JS_TIMERS and format string below

static void InitJsTimers()
{
	wchar_t* pos = js_timer_descriptions_buf;
	for(size_t i = 0; i < MAX_JS_TIMERS; i++)
	{
		const wchar_t* description = pos;
		pos += swprintf_s(pos, 12, L"js_timer %d", (int)i)+1;
		timer_AddClient(&js_timer_clients[i], description);
	}

	// call several times to get a good approximation of 'hot' performance.
	// note: don't use a separate timer slot to warm up and then judge
	// overhead from another: that causes worse results (probably some
	// caching effects inside JS, but I don't entirely understand why).
	static const char* calibration_script =
		"startXTimer(0);\n"
		"stopXTimer (0);\n"
		"\n";
	g_ScriptingHost.RunMemScript(calibration_script, strlen(calibration_script));
	// slight hack: call RunMemScript twice because we can't average several
	// TimerUnit values because there's no operator/. this way is better anyway
	// because it hopefully avoids the one-time JS init overhead.
	g_ScriptingHost.RunMemScript(calibration_script, strlen(calibration_script));
	js_timer_overhead = js_timer_clients[0].sum;
	js_timer_clients[0].sum.SetToZero();
}

JSBool StartJsTimer(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	ONCE(InitJsTimers());

	JSU_REQUIRE_PARAMS(1);
	size_t slot = ToPrimitive<size_t>(argv[0]);
	if (slot >= MAX_JS_TIMERS)
		return JS_FALSE;

	js_start_times[slot].SetFromTimer();
	return JS_TRUE;
}


JSBool StopJsTimer(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_PARAMS(1);
	size_t slot = ToPrimitive<size_t>(argv[0]);
	if (slot >= MAX_JS_TIMERS)
		return JS_FALSE;

	TimerUnit now;
	now.SetFromTimer();
	now.Subtract(js_timer_overhead);
	timer_BillClient(&js_timer_clients[slot], js_start_times[slot], now);
	js_start_times[slot].SetToZero();
	return JS_TRUE;
}


//-----------------------------------------------------------------------------
// Game Setup
//-----------------------------------------------------------------------------

// Create a new network server object.
// params:
// returns: net server object
JSBool CreateServer(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

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
JSBool CreateClient(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

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
// TODO: Replace StartGame with Create(Game|Server|Client)/game.start() -
//   after merging CGame and CGameAttributes
JSBool StartGame(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);

	// Hosted MP Game
	if (g_NetServer) 
	{
		*rval = BOOLEAN_TO_JSVAL(g_NetServer->StartGame() == 0);
	}
	// Joined MP Game
	else if (g_NetClient)
	{
		*rval = BOOLEAN_TO_JSVAL(g_NetClient->StartGame() == 0);
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
	else
	{
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	}

	return( JS_TRUE );
}


// Immediately ends the current game (if any).
// params:
// returns:
JSBool EndGame(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	EndGame();
	return JS_TRUE;
}

JSBool GetGameMode(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	*rval = ToJSVal( g_GameAttributes.GetGameMode() );
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
JSBool LoadLanguage(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_PARAMS(1);

	CStr lang = g_ScriptingHost.ValueToString(argv[0]);
	I18n::LoadLanguage(lang);

	return JS_TRUE;
}


// Return identifier of the current language (locale) in use.
// params:
// returns: language id [string] as in I18n::LoadLanguage
JSBool GetLanguageID(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();
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
JSBool ProvokeCrash(JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	MICROLOG(L"Crashing at user's request.");
	return *(JSBool*)0;
}


// Force a JS garbage collection cycle to take place immediately.
// params:
// returns: true [bool]
// notes:
// - writes an indication of how long this took to the console.
JSBool ForceGarbageCollection(JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval)
{
	JSU_REQUIRE_NO_PARAMS();

	double time = timer_Time();
	JS_GC(cx);
	time = timer_Time() - time;
	g_Console->InsertMessage(L"Garbage collection completed in: %f", time);
	*rval = JSVAL_TRUE;
	return JS_TRUE ;
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
JSBool GetFps( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	*rval = INT_TO_JSVAL(g_frequencyFilter->StableFrequency());
	return JS_TRUE;
}


// Cause the game to exit gracefully.
// params:
// returns:
// notes:
// - Exit happens after the current main loop iteration ends
//   (since this only sets a flag telling it to end)
JSBool ExitProgram( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	kill_mainloop();
	return JS_TRUE;
}


// Write an indication of total video RAM to console.
// params:
// returns:
// notes:
// - May not be supported on all platforms.
// - Only a rough approximation; do not base low-level decisions
//   ("should I allocate one more texture?") on this.
JSBool WriteVideoMemToConsole( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo();
	g_Console->InsertMessage(L"VRAM: total %d", videoInfo->video_mem);
	return JS_TRUE;
}


// Change the mouse cursor.
// params: cursor name [string] (i.e. basename of definition file and texture)
// returns:
// notes:
// - Cursors are stored in "art\textures\cursors"
JSBool SetCursor( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);
	g_CursorName = g_ScriptingHost.ValueToUCString(argv[0]);
	return JS_TRUE;
}

JSBool GetCursorName( JSContext* UNUSED(cx), JSObject*, uintN UNUSED(argc), jsval* UNUSED(argv), jsval* rval )
{
	*rval = ToJSVal(g_CursorName);
	return JS_TRUE;
}

// Trigger a rewrite of all maps.
// params:
// returns:
// notes:
// - Usefulness is unclear. If you need it, consider renaming this and updating the docs.
JSBool _RewriteMaps( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	g_Game->GetWorld()->RewriteMap();
	return JS_TRUE;
}


// Change the LOD bias.
// params: LOD bias [float]
// returns:
// notes:
// - value is as required by GL_TEXTURE_LOD_BIAS.
// - useful for adjusting image "sharpness" (since it affects which mipmap level is chosen)
JSBool _LodBias( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);

	g_Renderer.SetOptionFloat(CRenderer::OPT_LODBIAS, ToPrimitive<float>(argv[0]));
	return JS_TRUE;
}


// Focus the game camera on a given position.
// params: target position vector [CVector3D]
// returns: success [bool]
JSBool SetCameraTarget( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(1);
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
// params: none (-> "date time (svn revision)") OR an integer specifying
//   what to display: 0 for date, 1 for time, 2 for svn revision
// returns: string with the requested timestamp info
// notes:
// - Displayed on main menu screen; tells non-programmers which auto-build
//   they are running. Could also be determined via .EXE file properties,
//   but that's a bit more trouble.
// - To be exact, the date/time returned is when scriptglue.cpp was
//   last compiled, but the auto-build does full rebuilds.
// - svn revision is generated by calling svnversion and cached in
//   lib/svn_revision.cpp. it is useful to know when attempting to
//   reproduce bugs (the main EXE and PDB should be temporarily reverted to
//   that revision so that they match user-submitted crashdumps).
JSBool GetBuildTimestamp( JSContext* cx, JSObject*, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_MAX_PARAMS(1);

	char buf[200];

	// see function documentation
	const int mode = argc? JSVAL_TO_INT(argv[0]) : -1;
	switch(mode)
	{
	case -1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s %s (%ls)", __DATE__, __TIME__, svn_revision);
		break;
	case 0:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s", __DATE__);
		break;
	case 1:
		sprintf_s(buf, ARRAY_SIZE(buf), "%s", __TIME__);
		break;
	case 2:
		sprintf_s(buf, ARRAY_SIZE(buf), "%ls", svn_revision);
		break;
	}

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buf));
	return JS_TRUE;
}


// Return distance between 2 points.
// params: 2 position vectors [CVector3D]
// returns: Euclidean distance [float]
JSBool ComputeDistanceBetweenTwoPoints( JSContext* cx, JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS(2);

	CVector3D* a = ToNative<CVector3D>( argv[0] );
	CVector3D* b = ToNative<CVector3D>( argv[1] );
	float dist = ( *a - *b ).Length();
	*rval = ToJSVal( dist );
	return( JS_TRUE );
}


// Returns the global object.
// params:
// returns: global object
// notes:
// - Useful for accessing an object from another scope.
JSBool GetGlobal( JSContext* cx, JSObject* globalObject, uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	*rval = OBJECT_TO_JSVAL( globalObject );
	return( JS_TRUE );
}

// Saves the current profiling data to the logs/profile.txt file
JSBool SaveProfileData( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	g_ProfileViewer.SaveToFile();
	return( JS_TRUE );
}

// Activates the building placement cursor for placing a building. The currently selected units
// are then ordered to construct the building if it is placed.
// params: templateName - the name of the entity to place.
// returns: true if cursor was activated, false if cursor was already active.
JSBool StartPlacing( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
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

	*rval = g_BuildingPlacer.Activate(name) ? JS_TRUE : JS_FALSE;

	return( JS_TRUE );
}

// Toggles drawing the sky
JSBool ToggleSky( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	g_Renderer.GetSkyManager()->m_RenderSky = !g_Renderer.GetSkyManager()->m_RenderSky;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Toggles drawing territory outlines
JSBool ToggleTerritoryRendering( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	g_Renderer.m_RenderTerritories = !g_Renderer.m_RenderTerritories;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}


//-----------------------------------------------------------------------------
// water

// Toggles drawing the water plane
JSBool ToggleWater( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	g_Renderer.GetWaterManager()->m_RenderWater = !g_Renderer.GetWaterManager()->m_RenderWater;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the water plane height
JSBool SetWaterHeight( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 1 );
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
JSBool GetWaterHeight( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	*rval = ToJSVal(g_Renderer.GetWaterManager()->m_WaterHeight);
	return( JS_TRUE );
}

// Sets the water color
JSBool SetWaterColor( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 3 );
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

// Sets the water tint (used to tint reflections in fancy water)
JSBool SetWaterTint( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 3 );
	float r,g,b;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], r )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[1], g )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[2], b ))
	{
		JS_ReportError( cx, "Invalid arguments" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_WaterTint = CColor(r, g, b, 1.0f);
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the water tint (used to tint reflections in fancy water)
JSBool SetReflectionTint( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 3 );
	float r,g,b;
	if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], r )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[1], g )
		|| !ToPrimitive( g_ScriptingHost.GetContext(), argv[2], b ))
	{
		JS_ReportError( cx, "Invalid arguments" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}
	g_Renderer.GetWaterManager()->m_ReflectionTint = CColor(r, g, b, 1.0f);
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

// Sets the max water alpha (achieved when it is at WaterFullDepth or deeper)
JSBool SetWaterMaxAlpha( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 1 );
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
JSBool SetWaterFullDepth( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 1 );
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
JSBool SetWaterAlphaOffset( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 1 );
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

//-----------------------------------------------------------------------------

// Is the game paused?
JSBool IsPaused( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	if( !g_Game )
	{
		JS_ReportError( cx, "Game is not started" );
		return JS_FALSE;
	}

	*rval = g_Game->m_Paused ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE ;
}

// Pause/unpause the game
JSBool SetPaused( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS( 1 );

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
		JS_ReportError( cx, "Invalid parameter to SetPaused" );
	}

	return  JS_TRUE;
}

// Get game time
JSBool GetGameTime( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();

	if( !g_Game )
	{
		JS_ReportError( cx, "Game is not started" );
		return JS_FALSE;
	}

	*rval = ToJSVal(g_Game->GetSimulation()->GetTime());
	return JS_TRUE;
}

JSBool RegisterTrigger( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CTrigger* trigger = ToNative<CTrigger>( argv[0] );
	debug_assert( trigger );

	g_TriggerManager.AddTrigger( trigger );
	*rval = JSVAL_NULL;
	return JS_TRUE;
}

JSBool GetTrigger( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_PARAMS_CPP(1);
	CStrW name = ToPrimitive<CStrW>( argv[0] );
	
	if ( g_TriggerManager.m_TriggerMap.find(name) != g_TriggerManager.m_TriggerMap.end() )
		*rval = ToJSVal( g_TriggerManager.m_TriggerMap[name] );
	else
	{
		debug_printf(L"Invalid trigger name %ls", name.c_str());
		*rval = JSVAL_NULL;
	}
	return JS_TRUE;
}

// Reveal map
JSBool RevealMap( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_MAX_PARAMS(1);

	int newValue;
	if(argc == 0)
		newValue = LOS_SETTING_ALL_VISIBLE;
	else if(!ToPrimitive( g_ScriptingHost.GetContext(), argv[0], newValue ) || newValue > 2)
	{
		JS_ReportError( cx, "Invalid argument (should be 0, 1 or 2)" );
		*rval = JSVAL_VOID;
		return( JS_FALSE );
	}

	g_Game->GetWorld()->GetLOSManager()->m_LOSSetting = (ELOSSetting)newValue;
	*rval = JSVAL_VOID;
	return( JS_TRUE );
}

/**
 * isGameRunning
 * @return bool
 */
JSBool isGameRunning( JSContext* cx, JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* rval )
{
	JSU_REQUIRE_NO_PARAMS();
	
	if (g_Game && g_Game->IsGameStarted())
	{
		*rval = JSVAL_TRUE;
	}
	else
	{
		*rval = JSVAL_FALSE;
	}
	
	return JS_TRUE;
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
#define JS_FUNC(script_name, cpp_function, min_params) { script_name, cpp_function, min_params, 0, 0 },

JSFunctionSpec ScriptFunctionTable[] =
{
	// Console
	JS_FUNC("writeConsole", JSI_Console::writeConsole, 1)	// external

	// Entity
	JS_FUNC("getEntityByUnitID", GetEntityByUnitID, 1)
	JS_FUNC("GetPlayerUnitCount", GetPlayerUnitCount, 1)
	JS_FUNC("getEntityTemplate", GetEntityTemplate, 1)
	JS_FUNC("issueCommand", IssueCommand, 2)
	JS_FUNC("startPlacing", StartPlacing, 1)

	// Formation
	JS_FUNC("createEntityFormation", CreateEntityFormation, 2)
	JS_FUNC("removeFromFormation", RemoveFromFormation, 1)
	JS_FUNC("lockEntityFormation", LockEntityFormation, 1)
	JS_FUNC("isFormationLocked", IsFormationLocked, 1)

	// Trigger
	JS_FUNC("registerTrigger", RegisterTrigger, 1)
	
	// Tech
	JS_FUNC("getTechnology", GetTechnology, 2)

	// Camera
	JS_FUNC("setCameraTarget", SetCameraTarget, 1)

	// Sky
	JS_FUNC("toggleSky", ToggleSky, 0)

	// Water
	JS_FUNC("toggleWater", ToggleWater, 0)
	JS_FUNC("setWaterHeight", SetWaterHeight, 1)
	JS_FUNC("getWaterHeight", GetWaterHeight, 0)
	JS_FUNC("setWaterColor", SetWaterColor, 3)
	JS_FUNC("setWaterTint", SetWaterTint, 3)
	JS_FUNC("setReflectionTint", SetReflectionTint, 3)
	JS_FUNC("setWaterMaxAlpha", SetWaterMaxAlpha, 0)
	JS_FUNC("setWaterFullDepth", SetWaterFullDepth, 0)
	JS_FUNC("setWaterAlphaOffset", SetWaterAlphaOffset, 0)

	// Territory rendering
	JS_FUNC("toggleTerritoryRendering", ToggleTerritoryRendering, 0)

	// Events
	JS_FUNC("addGlobalHandler", AddGlobalHandler, 2)
	JS_FUNC("removeGlobalHandler", RemoveGlobalHandler, 2)

	// Timer
	JS_FUNC("setTimeout", SetTimeout, 2)
	JS_FUNC("setInterval", SetInterval, 2)
	JS_FUNC("cancelInterval", CancelInterval, 0)
	JS_FUNC("cancelTimer", CancelTimer, 0)
	JS_FUNC("setSimRate", SetSimRate, 1)

	// Random number generator
	JS_FUNC("simRand", SimRand, 0)
	JS_FUNC("simRandInt", SimRandInt, 1)

	// Profiling
	JS_FUNC("startXTimer", StartJsTimer, 1)
	JS_FUNC("stopXTimer", StopJsTimer, 1)

	// Game Setup
	JS_FUNC("startGame", StartGame, 0)
	JS_FUNC("endGame", EndGame, 0)
	JS_FUNC("getGameMode", GetGameMode, 0)
	JS_FUNC("createClient", CreateClient, 0)
	JS_FUNC("createServer", CreateServer, 0)

	// VFS (external)
	JS_FUNC("buildDirEntList", JSI_VFS::BuildDirEntList, 1)
	JS_FUNC("getFileMTime", JSI_VFS::GetFileMTime, 1)
	JS_FUNC("getFileSize", JSI_VFS::GetFileSize, 1)
	JS_FUNC("readFile", JSI_VFS::ReadFile, 1)
	JS_FUNC("readFileLines", JSI_VFS::ReadFileLines, 1)
	JS_FUNC("archiveBuilderCancel", JSI_VFS::ArchiveBuilderCancel, 1)

	// Internationalization
	JS_FUNC("loadLanguage", LoadLanguage, 1)
	JS_FUNC("getLanguageID", GetLanguageID, 0)
	// note: i18n/ScriptInterface.cpp registers translate() itself.
	// rationale: see implementation section above.

	// Debug
	JS_FUNC("crash", ProvokeCrash, 0)
	JS_FUNC("forceGC", ForceGarbageCollection, 0)
	JS_FUNC("revealMap", RevealMap, 1)

	// Misc. Engine Interface
	JS_FUNC("writeLog", WriteLog, 1)
	JS_FUNC("exit", ExitProgram, 0)
	JS_FUNC("isPaused", IsPaused, 0)
	JS_FUNC("setPaused", SetPaused, 1)
	JS_FUNC("getGameTime", GetGameTime, 0)
	JS_FUNC("vmem", WriteVideoMemToConsole, 0)
	JS_FUNC("_rewriteMaps", _RewriteMaps, 0)
	JS_FUNC("_lodBias", _LodBias, 0)
	JS_FUNC("setCursor", SetCursor, 1)
	JS_FUNC("getCursorName", GetCursorName, 0)
	JS_FUNC("getFPS", GetFps, 0)
	JS_FUNC("isOrderQueued", isOrderQueued, 1)
	JS_FUNC("isGameRunning", isGameRunning, 0)

	// Miscellany
	JS_FUNC("v3dist", ComputeDistanceBetweenTwoPoints, 2)
	JS_FUNC("buildTime", GetBuildTimestamp, 0)
	JS_FUNC("getGlobal", GetGlobal, 0)
	JS_FUNC("saveProfileData", SaveProfileData, 0)

	// end of table marker
	{0, 0, 0, 0, 0}
};
#undef JS_FUNC


//-----------------------------------------------------------------------------
// property accessors
//-----------------------------------------------------------------------------

JSBool GetEntitySet( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(argv), jsval* vp )
{
	std::vector<HEntity> extant;
	g_EntityManager.GetExtantAsHandles(extant);
	*vp = OBJECT_TO_JSVAL(EntityCollection::Create(extant));

	return JS_TRUE;
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

JSPropertySpec ScriptGlobalTable[] =
{
	{ "selection"  , GLOBAL_SELECTION,   JSPROP_PERMANENT, JSI_Selection::getSelection, JSI_Selection::SetSelection },
	{ "groups"     , GLOBAL_GROUPSARRAY, JSPROP_PERMANENT, JSI_Selection::getGroups, JSI_Selection::setGroups },
	{ "camera"     , GLOBAL_CAMERA,      JSPROP_PERMANENT, JSI_Camera::getCamera, JSI_Camera::setCamera },
	{ "console"    , GLOBAL_CONSOLE,     JSPROP_PERMANENT|JSPROP_READONLY, JSI_Console::getConsole, 0 },
	{ "lightenv"   , GLOBAL_LIGHTENV,    JSPROP_PERMANENT, JSI_LightEnv::getLightEnv, JSI_LightEnv::setLightEnv },
	{ "entities"   , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetEntitySet, 0 },
	{ "players"    , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetPlayerSet, 0 },
	{ "localPlayer", 0,                  JSPROP_PERMANENT, GetLocalPlayer, SetLocalPlayer },
	{ "gaiaPlayer" , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetGaiaPlayer, 0 },
	{ "gameView"   , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetGameView, 0 },
	{ "renderer"   , 0,                  JSPROP_PERMANENT|JSPROP_READONLY, GetRenderer, 0 },

	// end of table marker
	{ 0, 0, 0, 0, 0 },
};
