// Scheduler.h
//
// Mark Thompson mot20@cam.ac.uk / mark@wildfiregames.com
// 
// Message scheduler
//

#ifndef SCHEDULER_INCLUDED
#define SCHEDULER_INCLUDED

#include <queue>
#include <list>

#include "EntityMessage.h"
#include "EntityHandles.h"
#include "Singleton.h"
#include "CStr.h"
#include "scripting/ScriptableObject.h"

class CJSProgressTimer;

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

struct SDispatchObjectScript : public SDispatchObject
{
	CStrW script;
	JSObject* operateOn;
	inline SDispatchObjectScript( const CStrW& _script, const size_t _deliveryTime, JSObject* _operateOn = NULL )
		: SDispatchObject( _deliveryTime ), script( _script ), operateOn( _operateOn ) {}
	inline SDispatchObjectScript( const CStrW& _script, const size_t _deliveryTime, JSObject* _operateOn, const size_t recurrence )
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
	std::priority_queue<SDispatchObjectScript> timeScript, frameScript;
	std::priority_queue<SDispatchObjectFunction> timeFunction, frameFunction;
	std::list<CJSProgressTimer*> progressTimers;
	bool m_abortInterval;

	void pushTime( size_t delay, const CStrW& fragment, JSObject* operateOn = NULL );
	void pushFrame( size_t delay, const CStrW& fragment, JSObject* operateOn = NULL );
	void pushInterval( size_t first, size_t interval, const CStrW& fragment, JSObject* operateOn = NULL );
	void pushTime( size_t delay, JSFunction* function, JSObject* operateOn = NULL );
	void pushFrame( size_t delay, JSFunction* function, JSObject* operateOn = NULL );
	void pushInterval( size_t first, size_t interval, JSFunction* function, JSObject* operateOn = NULL );
	void pushProgressTimer( CJSProgressTimer* progressTimer );
	void update(size_t elapsedSimulationTime);
};

#define g_Scheduler CScheduler::GetSingleton()

class CJSProgressTimer : public CJSObject<CJSProgressTimer>
{
	friend struct CScheduler;
	double m_Max, m_Current, m_Increment;
	JSFunction* m_Callback;
	JSObject* m_OperateOn;
	CJSProgressTimer( double Max, double Increment, JSFunction* Callback, JSObject* OperateOn );
	static JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval );
public:
	static void ScriptingInit();
};

// made visible to main.cpp's Frame() so that it can abort after 100 frames
// if g_FixedFrameTiming == true (allows measuring performance).
extern size_t frameCount;

extern const int ORDER_DELAY;

#endif
