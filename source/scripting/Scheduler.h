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

// Scheduler.h
// 
// Message scheduler
//

#ifndef INCLUDED_SCHEDULER
#define INCLUDED_SCHEDULER

#include <queue>
#include <list>
#include <set>

#include "ps/Singleton.h"
#include "ps/CStr.h"
#include "scripting/ScriptableObject.h"

class CJSProgressTimer;

// Message, destination and delivery time information.
struct SDispatchObject
{
	int id;
	int deliveryTime;
	bool isRecurrent; int delay;
	SDispatchObject( int _id, const int _deliveryTime )
		: id(_id), deliveryTime( _deliveryTime ), isRecurrent( false ) {}
	SDispatchObject( int _id, const int _deliveryTime, const int _recurrence )
		: id(_id), deliveryTime( _deliveryTime ), isRecurrent( true ), delay( _recurrence ) {}
	inline bool operator<( const SDispatchObject& compare ) const
	{
		return( deliveryTime > compare.deliveryTime );
	}
};

struct SDispatchObjectScript : public SDispatchObject
{
	CStrW script;
	JSObject* operateOn;
	inline SDispatchObjectScript( int _id, const CStrW& _script, 
			const int _deliveryTime, JSObject* _operateOn = NULL )
		: SDispatchObject( _id, _deliveryTime ), script( _script ), operateOn( _operateOn ) {}
	inline SDispatchObjectScript( int _id, const CStrW& _script, 
			const int _deliveryTime, JSObject* _operateOn, const int recurrence )
		: SDispatchObject( _id, _deliveryTime, recurrence ), script( _script ), operateOn( _operateOn ) {}
};

struct SDispatchObjectFunction : public SDispatchObject
{
	JSFunction* function;
	JSObject* operateOn;
	inline SDispatchObjectFunction( int _id, JSFunction* _function, 
			const int _deliveryTime, JSObject* _operateOn = NULL )
		: SDispatchObject( _id, _deliveryTime ), function( _function ), operateOn( _operateOn ) {}
	inline SDispatchObjectFunction( int _id, JSFunction* _function, 
			const int _deliveryTime, JSObject* _operateOn, const int recurrence )
		: SDispatchObject( _id, _deliveryTime, recurrence ), function( _function ), operateOn( _operateOn ) {}
};

struct CScheduler : public Singleton<CScheduler>
{
	std::priority_queue<SDispatchObjectScript> timeScript, frameScript;
	std::priority_queue<SDispatchObjectFunction> timeFunction, frameFunction;
	std::list<CJSProgressTimer*> progressTimers;
	int m_nextTaskId;
	bool m_abortInterval;
	STL_HASH_SET<int> tasksToCancel;

	CScheduler();
	int PushTime( int delay, const CStrW& fragment, JSObject* operateOn = NULL );
	int PushFrame( int delay, const CStrW& fragment, JSObject* operateOn = NULL );
	int PushInterval( int first, int interval, const CStrW& fragment, JSObject* operateOn = NULL, int id = 0 );
	int PushTime( int delay, JSFunction* function, JSObject* operateOn = NULL );
	int PushFrame( int delay, JSFunction* function, JSObject* operateOn = NULL );
	int PushInterval( int first, int interval, JSFunction* function, JSObject* operateOn = NULL, int id = 0 );
	void PushProgressTimer( CJSProgressTimer* progressTimer );
	void CancelTask( int id );
	void Update(int elapsedSimulationTime);
};

#define g_Scheduler CScheduler::GetSingleton()

class CJSProgressTimer : public CJSObject<CJSProgressTimer>
{
	friend struct CScheduler;
	double m_Max, m_Current, m_Increment;
	JSFunction* m_Callback;
	JSObject* m_OperateOn;
	CJSProgressTimer( double Max, double Increment, JSFunction* Callback, JSObject* OperateOn );
	static JSBool Construct( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
public:
	static void ScriptingInit();
};

extern const int ORDER_DELAY;

#endif
