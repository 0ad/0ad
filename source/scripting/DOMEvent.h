// DOM-style event object

// Note: Cancellable [UK]? Cancelable [US]? DOM says one l, OED says 2.
//       JS interface uses 1.

// Entity and e.g. projectile classes derive from this and use it for
// sending/receiving events.

#ifndef INCLUDED_DOMEVENT
#define INCLUDED_DOMEVENT

#include "ScriptableObject.h"
#include "EventTypes.h"	// for EVENT_LAST

class CScriptObject;
class CScriptEvent;

typedef CScriptObject* DOMEventHandler;

class IEventTarget
{
	// Return 'true' if we should stop propagating.
	bool _DispatchEvent( CScriptEvent* evt, IEventTarget* target );

	// Events dispatched to this object are sent here before being processed.
	IEventTarget* before;
	// Events dispatched to this object are sent here after being processed.
	IEventTarget* after;

	typedef std::vector<DOMEventHandler> HandlerList;
	HandlerList m_Handlers_id[EVENT_LAST];
	typedef STL_HASH_MULTIMAP<CStrW, DOMEventHandler, CStrW_hash_compare> HandlerMap;
	HandlerMap m_Handlers_name;
	typedef std::pair<HandlerMap::iterator, HandlerMap::iterator> HandlerRange;
public:
	IEventTarget()
	{
		before = NULL;
		after = NULL;
	}
	virtual ~IEventTarget();
	// Set target that will receive each event after it is processed.
	// unused
	inline void SetPriorObject( IEventTarget* obj )
	{
		before = obj;
	}
	// Set target that will receive each event after it is processed.
	// used by Entity and EntityTemplate.
	inline void SetNextObject( IEventTarget* obj )
	{
		after = obj;
	}

	// Register a handler for the given event type.
	// Returns false if the handler was already present
	bool AddHandler( size_t TypeCode, DOMEventHandler handler );
	bool AddHandler( const CStrW& TypeString, DOMEventHandler handler );
	// Remove a previously registered handler for the specified event.
	// Returns false if the handler was not present
	bool RemoveHandler( size_t TypeCode, DOMEventHandler handler );
	bool RemoveHandler( const CStrW& TypeString, DOMEventHandler handler );

	// called by ScriptGlue.cpp for add|RemoveGlobalHandler
	bool AddHandlerJS( JSContext* cx, uintN argc, jsval* argv );
	bool RemoveHandlerJS( JSContext* cx, uintN argc, jsval* argv );

	// Return the JSObject* we'd like to be the 'this' object
	// when executing the handler. The argument is the object
	// to which the event is targeted.
	// It is passed to CScriptObject::DispatchEvent.
	virtual JSObject* GetScriptExecContext( IEventTarget* target ) = 0;

	// Dispatch an event to its handler.
	// returns: whether the event arrived (i.e. wasn't cancelled) [bool]
	bool DispatchEvent( CScriptEvent* evt );
};

class CScriptEvent : public CJSObject<CScriptEvent>
{
public:
	enum EPhaseType
	{
		CAPTURING_PHASE = 1,
		AT_TARGET = 2,
		BUBBLING_PHASE = 3
	};

	// Target (currently unused)
	IEventTarget* m_Target;

	// Listening object currently being processed (currently unused)
	IEventTarget* m_CurrentTarget;

	// Phase type (currently unused)
	// EPhaseType m_EventPhase;

	// Can bubble? (currently unused)
	// bool m_Bubbles;

	// Can be cancelled (default actions prevented)
	bool m_Cancelable;

	// Can be blocked (prevented from propogating along the handler chain)
	bool m_Blockable;

	// Timestamp (milliseconds since epoch (start of game?))
	size_t m_Timestamp;

	// Event type string
	CStrW m_Type;

	// Type code (to speed lookups)
	size_t m_TypeCode;

	// Has been cancelled?
	bool m_Cancelled;

	// Has it been blocked (won't be sent to any more handlers)
	bool m_Blocked;

// -- 

	CStr ToString( JSContext* cx, uintN argc, jsval* argv );
	void PreventDefault( JSContext* cx, uintN argc, jsval* argv );
	void StopPropagation( JSContext* cx, uintN argc, jsval* argv );

public:
	CScriptEvent( const CStrW& Type, size_t TypeCode = ~0, bool Cancelable = true, bool Blockable = true );
	static void ScriptingInit();
};

#endif


