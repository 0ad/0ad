#include "precompiled.h"
#include "Scheduler.h"
#include "Entity.h"

size_t simulationTime;
size_t frameCount;

/*
void CScheduler::pushTime( size_t delay, const HEntity& destination, const CMessage* message )
{
	timeMessage.push( SDispatchObjectMessage( destination, simulationTime + delay, message ) );
}

void CScheduler::pushFrame( size_t delay, const HEntity& destination, const CMessage* message )
{
	frameMessage.push( SDispatchObjectMessage( destination, frameCount + delay, message ) );
}
*/

void CScheduler::pushTime( size_t delay, const CStrW& fragment, JSObject* operateOn )
{
	timeScript.push( SDispatchObjectScript( fragment, simulationTime + delay, operateOn ) );
}

void CScheduler::pushFrame( size_t delay, const CStrW& fragment, JSObject* operateOn )
{
	frameScript.push( SDispatchObjectScript( fragment, frameCount + delay, operateOn ) );
}

void CScheduler::pushInterval( size_t first, size_t interval, const CStrW& fragment, JSObject* operateOn )
{
	timeScript.push( SDispatchObjectScript( fragment, simulationTime + first, operateOn, interval ) );
}

void CScheduler::pushTime( size_t delay, JSFunction* script, JSObject* operateOn )
{
	timeFunction.push( SDispatchObjectFunction( script, simulationTime + delay, operateOn ) );
}

void CScheduler::pushFrame( size_t delay, JSFunction* script, JSObject* operateOn )
{
	frameFunction.push( SDispatchObjectFunction( script, frameCount + delay, operateOn ) );
}

void CScheduler::pushInterval( size_t first, size_t interval, JSFunction* function, JSObject* operateOn )
{
	timeFunction.push( SDispatchObjectFunction( function, simulationTime + first, operateOn, interval ) );
}

void CScheduler::update(size_t simElapsed)
{
	simulationTime += simElapsed;
    frameCount++;

	while( !timeScript.empty() )
	{
		SDispatchObjectScript top = timeScript.top();
		if( top.deliveryTime > simulationTime )
			break;
		timeScript.pop();
		m_abortInterval = false;
		g_ScriptingHost.ExecuteScript( top.script, CStrW( L"timer" ), top.operateOn );
		if( top.isRecurrent && !m_abortInterval )
			pushInterval( top.delay, top.delay, top.script, top.operateOn );
	}
	while( !frameScript.empty() )
	{
		SDispatchObjectScript top = frameScript.top();
		if( top.deliveryTime > frameCount )
			break;
		frameScript.pop();
		g_ScriptingHost.ExecuteScript( top.script, CStrW( L"timer" ), top.operateOn );
	}
	while( !timeFunction.empty() )
	{
		SDispatchObjectFunction top = timeFunction.top();
		if( top.deliveryTime > simulationTime )
			break;
		timeFunction.pop();
		jsval rval;
		m_abortInterval = false;
		JS_CallFunction( g_ScriptingHost.getContext(), top.operateOn, top.function, 0, NULL, &rval ); 
		if( top.isRecurrent && !m_abortInterval )
			pushInterval( top.delay, top.delay, top.function, top.operateOn );
	}
	while( !frameFunction.empty() )
	{
		SDispatchObjectFunction top = frameFunction.top();
		if( top.deliveryTime > frameCount )
			break;
		frameFunction.pop();
		jsval rval;
		JS_CallFunction( g_ScriptingHost.getContext(), top.operateOn, top.function, 0, NULL, &rval ); 
	}
}
