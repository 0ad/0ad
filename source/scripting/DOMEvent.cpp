#include "precompiled.h"
#include "DOMEvent.h"
#include "timer.h"

CScriptEvent::CScriptEvent( const CStrW& Type, bool Cancelable, unsigned int TypeCode )
{
	m_Type = Type; m_TypeCode = TypeCode; m_Cancelable = Cancelable; m_Cancelled = false; m_Timestamp = (long)( get_time() * 1000.0 );
	AddReadOnlyProperty( L"type", &m_Type );
	AddReadOnlyProperty( L"cancelable", &m_Cancelable );
	AddReadOnlyProperty( L"timeStamp", &m_Timestamp );
}

void CScriptEvent::ScriptingInit()
{
	AddMethod<jsval, ToString>( "toString", 0 );
	AddMethod<jsval, PreventDefault>( "preventDefault", 0 );

	CJSObject<CScriptEvent>::ScriptingInit( "Event" );
}

jsval CScriptEvent::PreventDefault( JSContext* cx, uintN argc, jsval* argv )
{
	if( m_Cancelable )
		m_Cancelled = true;
	return( JSVAL_VOID );
}

jsval CScriptEvent::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	utf16_t buffer[256];
	swprintf( buffer, 256, L"[object Event: %ls]", m_Type.c_str() );
	buffer[255] = 0;
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, buffer ) ) );
}

