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

#include "precompiled.h"
#include "DOMEvent.h"
#include "lib/timer.h"
#include "ps/Profile.h"
#include "simulation/ScriptObject.h"

IEventTarget::~IEventTarget()
{
	HandlerMap::iterator it;
	for( it = m_Handlers_name.begin(); it != m_Handlers_name.end(); it++ )
		delete( it->second );
}

bool IEventTarget::_DispatchEvent( CScriptEvent* evt, IEventTarget* target )
{	
	PROFILE_START( "_DispatchEvent" );

	// TODO: Deal correctly with multiple handlers

	if( before && before->_DispatchEvent( evt, target ) )
	{
		return( true ); // Stop propagation.
	}

	evt->m_CurrentTarget = this;
	
	HandlerList::const_iterator it;
	const HandlerList &handlers=m_Handlers_id[evt->m_TypeCode];
	for( it = handlers.begin(); it != handlers.end(); it++ )
	{
		DOMEventHandler id = *it;
		if( id && id->DispatchEvent( GetScriptExecContext( target ), evt ) )
		{
			return( true );
		}
	}

	HandlerRange range = m_Handlers_name.equal_range( evt->m_Type );
	HandlerMap::iterator itm;
	for( itm = range.first; itm != range.second; itm++ )
	{
		DOMEventHandler id = itm->second;
		if( id && id->DispatchEvent( GetScriptExecContext( target ), evt ) )
		{
			return( true );
		}
	}
	
	if( after && after->_DispatchEvent( evt, target ) )
	{
		return( true ); // Stop propagation.
	}

	return( false );

	PROFILE_END( "_DispatchEvent" );
}

// Dispatch an event to its handler.
// returns: whether the event arrived (i.e. wasn't cancelled) [bool]
bool IEventTarget::DispatchEvent( CScriptEvent* evt )
{
	char* data;
	PROFILE_START( "intern string" );
	data = (char*) g_Profiler.InternString( "script: " + (CStr8)evt->m_Type );
	PROFILE_END( "intern string" );
	g_Profiler.StartScript( data );
	evt->m_Target = this;
	_DispatchEvent( evt, this );
	g_Profiler.Stop();
	return( !evt->m_Cancelled );
}

bool IEventTarget::AddHandler( size_t TypeCode, DOMEventHandler handler )
{
	HandlerList::iterator it;
	for( it = m_Handlers_id[TypeCode].begin(); it != m_Handlers_id[TypeCode].end(); it++ )
		if( **it == *handler ) return( false );
	m_Handlers_id[TypeCode].push_back( handler );
	return( true );
}

bool IEventTarget::AddHandler( const CStrW& TypeString, DOMEventHandler handler )
{
	HandlerMap::iterator it;
	HandlerRange range = m_Handlers_name.equal_range( TypeString );
	for( it = range.first; it != range.second; it++ )
		if( *( it->second ) == *handler ) return( false );
	m_Handlers_name.insert( HandlerMap::value_type( TypeString, handler ) );
	return( true );
}

bool IEventTarget::RemoveHandler( size_t TypeCode, DOMEventHandler handler )
{
	HandlerList::iterator it;
	for( it = m_Handlers_id[TypeCode].begin(); it != m_Handlers_id[TypeCode].end(); it++ )
		if( **it == *handler )
		{
			m_Handlers_id[TypeCode].erase( it );
			return( true );
		}

	return( false );
}

bool IEventTarget::RemoveHandler( const CStrW& TypeString, DOMEventHandler handler )
{
	HandlerMap::iterator it;
	HandlerRange range = m_Handlers_name.equal_range( TypeString );
	for( it = range.first; it != range.second; it++ )
		if( *( it->second ) == *handler )
		{
			delete( it->second );
			m_Handlers_name.erase( it );
			return( true );
		}

	return( false );
}

bool IEventTarget::AddHandlerJS( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	debug_assert( argc >= 2 );
	DOMEventHandler handler = new CScriptObject( argv[1] );
	if( !handler->Defined() )
	{
		delete( handler );
		return( false );
	}
	if( !AddHandler( ToPrimitive<CStrW>( argv[0] ), handler ) )
	{
		delete( handler );
		return( false );
	}
	return( true );
}

bool IEventTarget::RemoveHandlerJS( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	debug_assert( argc >= 2 );
	DOMEventHandler handler = new CScriptObject( argv[1] );
	if( !handler->Defined() )
	{
		delete( handler );
		return( false );
	}
	if( !RemoveHandler( ToPrimitive<CStrW>( argv[0] ), handler ) )
	{
		delete( handler );
		return( false );
	}
	delete( handler );
	return( true );
}

CScriptEvent::CScriptEvent( const CStrW& Type, size_t TypeCode, bool Cancelable, bool Blockable )
{
	m_Type = Type; m_TypeCode = TypeCode;
	m_Cancelable = Cancelable; m_Cancelled = false;
	m_Blockable = Blockable; m_Blocked = false;
	m_Timestamp = (size_t)( timer_Time() * 1000.0 );
}

void CScriptEvent::ScriptingInit()
{
	AddMethod<CStr, &CScriptEvent::ToString>( "toString", 0 );
	AddMethod<void, &CScriptEvent::PreventDefault>( "preventDefault", 0 );
	AddMethod<void, &CScriptEvent::PreventDefault>( "cancel", 0 );
	AddMethod<void, &CScriptEvent::StopPropagation>( "stopPropagation", 0 );

	AddProperty( L"type", &CScriptEvent::m_Type, true );
	AddProperty( L"cancelable", &CScriptEvent::m_Cancelable, true );
	AddProperty( L"blockable", &CScriptEvent::m_Blockable, true );
	AddProperty( L"timestamp", &CScriptEvent::m_Timestamp, true );

	CJSObject<CScriptEvent>::ScriptingInit( "Event" );
}

void CScriptEvent::PreventDefault( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if( m_Cancelable )
		m_Cancelled = true;
}

void CScriptEvent::StopPropagation( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	if( m_Blockable )
		m_Blocked = true;
}

CStr CScriptEvent::ToString( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return "[object Event: " + CStr(m_Type) + "]";
}
