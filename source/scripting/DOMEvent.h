// DOM-style event object
//
// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

// Note: Cancellable? Cancelable? DOM says one l, OED says 2. JS interface uses 1.

#ifndef DOMEVENT_INCLUDED
#define DOMEVENT_INCLUDED

#include "ScriptableObject.h"

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
	// EventTarget* m_Target;

	// Listening object currently being processed (currently unused)
	// EventTarget* m_CurrentTarget;

	// Phase type (currently unused)
	// EPhaseType m_EventPhase;

	// Can bubble? (currently unused)
	// bool m_Bubbles;

	// Can be cancelled (default actions prevented)
	bool m_Cancelable;

	// Timestamp (milliseconds since epoch (start of game?))
	i32 m_Timestamp;

	// Event type string
	CStrW m_Type;

	// Type code (to speed lookups)
	unsigned int m_TypeCode;

	// Has been cancelled?
	bool m_Cancelled;

// -- 

	jsval ToString( JSContext* cx, uintN argc, jsval* argv );
	jsval PreventDefault( JSContext* cx, uintN argc, jsval* argv );

public:
	CScriptEvent( const CStrW& Type, bool Cancelable, unsigned int TypeCode = -1 );
	static void ScriptingInit();
};

#endif

