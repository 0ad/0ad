// Scheduler.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Message scheduler
//

#ifndef SCHEDULER_INCLUDED
#define SCHEDULER_INCLUDED

#include <queue>
#include "EntityMessage.h"
#include "EntityHandles.h"
#include "Singleton.h"
#include "CStr.h"
#include "scripting/ScriptingHost.h"

// Message, destination and delivery time information.
struct SDispatchObject
{
	size_t deliveryTime;
	bool isRecurrent; size_t delay;
	SDispatchObject( const size_t _deliveryTime )
		: deliveryTime( _deliveryTime ), isRecurrent( false ) {}
	SDispatchObject( const size_t _deliveryTime, const size_t _recurrence )
		: deliveryTime( _deliveryTime ), isRecurrent( true ), delay( _recurrence ) {}
	inline bool operator<( const SDispatchObject& compare ) const
	{
		return( deliveryTime > compare.deliveryTime );
	}
};

struct SDispatchObjectMessage : public SDispatchObject
{
	HEntity destination;
	const CMessage* message;
	inline SDispatchObjectMessage( const HEntity& _destination, size_t _deliveryTime, const CMessage* _message )
		: SDispatchObject( _deliveryTime ), destination( _destination ), message( _message ) {}
	inline SDispatchObjectMessage( const HEntity& _destination, size_t _deliveryTime, const CMessage* _message, const size_t recurrence )
		: SDispatchObject( _deliveryTime, recurrence ), destination( _destination ), message( _message ) {}
	
};

struct SDispatchObjectScript : public SDispatchObject
{
	CStr16 script;
	JSObject* operateOn;
	inline SDispatchObjectScript( const CStr16& _script, const size_t _deliveryTime, JSObject* _operateOn = NULL )
		: SDispatchObject( _deliveryTime ), script( _script ), operateOn( _operateOn ) {}
	inline SDispatchObjectScript( const CStr16& _script, const size_t _deliveryTime, JSObject* _operateOn, const size_t recurrence )
		: SDispatchObject( _deliveryTime, recurrence ), script( _script ), operateOn( _operateOn ) {}
};

struct SDispatchObjectFunction : public SDispatchObject
{
	JSFunction* function;
	JSObject* operateOn;
	inline SDispatchObjectFunction( JSFunction* _function, const size_t _deliveryTime, JSObject* _operateOn = NULL )
		: SDispatchObject( _deliveryTime ), function( _function ), operateOn( _operateOn ) {}
	inline SDispatchObjectFunction( JSFunction* _function, const size_t _deliveryTime, JSObject* _operateOn, const size_t recurrence )
		: SDispatchObject( _deliveryTime, recurrence ), function( _function ), operateOn( _operateOn ) {}
};

struct CScheduler : public Singleton<CScheduler>
{
	std::priority_queue<SDispatchObjectMessage> timeMessage, frameMessage;
	std::priority_queue<SDispatchObjectScript> timeScript, frameScript;
	std::priority_queue<SDispatchObjectFunction> timeFunction, frameFunction;

	bool m_abortInterval;

	void pushTime( size_t delay, const HEntity& destination, const CMessage* message );
	void pushFrame( size_t delay, const HEntity& destination, const CMessage* message );
	void pushTime( size_t delay, const CStr16& fragment, JSObject* operateOn = NULL );
	void pushFrame( size_t delay, const CStr16& fragment, JSObject* operateOn = NULL );
	void pushInterval( size_t first, size_t interval, const CStr16& fragment, JSObject* operateOn = NULL );
	void pushTime( size_t delay, JSFunction* function, JSObject* operateOn = NULL );
	void pushFrame( size_t delay, JSFunction* function, JSObject* operateOn = NULL );
	void pushInterval( size_t first, size_t interval, JSFunction* function, JSObject* operateOn = NULL );
	void update();
};

#define g_Scheduler CScheduler::GetSingleton()

#endif