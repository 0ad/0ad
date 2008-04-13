#include "precompiled.h"
#include "ProductionQueue.h"
#include "EntityManager.h"
#include "EventHandlers.h"
#include "Entity.h"
#include <algorithm>

// CProductionItem

CProductionItem::CProductionItem( int type, const CStrW& name, float totalTime )
		: m_type(type), m_name(name), m_totalTime(totalTime), m_elapsedTime(0)
{
}

CProductionItem::~CProductionItem()
{
}

void CProductionItem::Update( size_t timestep )
{
	m_elapsedTime = std::min( m_totalTime, m_elapsedTime + timestep/1000.0f );
}

bool CProductionItem::IsComplete()
{
	return( m_elapsedTime == m_totalTime );
}

float CProductionItem::GetProgress()
{
	return( m_totalTime==0 ? 1.0 : m_elapsedTime/m_totalTime );
}

jsval CProductionItem::JSI_GetProgress( JSContext* UNUSED(cx) )
{
	return ToJSVal( GetProgress() );
}

void CProductionItem::ScriptingInit()
{
	AddProperty(L"type", &CProductionItem::m_type, true);
	AddProperty(L"name", &CProductionItem::m_name, true);
	AddProperty(L"elapsedTime", &CProductionItem::m_elapsedTime, true);
	AddProperty(L"totalTime", &CProductionItem::m_totalTime, true);
	AddProperty(L"progress", static_cast<GetFn>(&CProductionItem::JSI_GetProgress));
	CJSObject<CProductionItem>::ScriptingInit("ProductionItem");
}

// CProductionQueue

CProductionQueue::CProductionQueue( CEntity* owner  )
		: m_owner(owner)
{
}

CProductionQueue::~CProductionQueue()
{
	CancelAll();
}

void CProductionQueue::AddItem( int type, const CStrW& name, float totalTime )
{
	m_items.push_back( new CProductionItem( type, name, totalTime ) );
}

void CProductionQueue::CancelAll()
{
	for( size_t i=0; i < m_items.size(); ++i )
	{
		// Cancel production of the item
		CProductionItem* item = m_items[i];
		CEventCancelProduction evt( item->m_type, item->m_name );
		m_owner->DispatchEvent( &evt );

		// Free its memory
		delete m_items[i];
	}
	m_items.clear();
}

void CProductionQueue::Update( size_t timestep )
{	
	if( !m_items.empty() )
	{
		CProductionItem* front = m_items.front();
		front->Update( timestep );
		if( front->IsComplete() )
		{
			CEventFinishProduction evt( front->m_type, front->m_name );
			m_owner->DispatchEvent( &evt );
			m_items.erase( m_items.begin() );
			delete front;
		}
		// TODO: Think about what we want to happen when there are multiple productions
		// with totalTime=0 at the front (pop them all at once or do one per update?)
	}
}

jsval CProductionQueue::JSI_GetLength( JSContext* UNUSED(cx) )
{
	return ToJSVal( (int) m_items.size() );
}

jsval_t CProductionQueue::JSI_Get( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc == 1 );
	debug_assert( JSVAL_IS_INT(argv[0]) );

	int index = ToPrimitive<int>( argv[0] );

	if(index < 0 || index >= (int)m_items.size() )
	{
		JS_ReportError( cx, "Production queue index out of bounds: %d", index );
		return JSVAL_NULL;
	}

	return ToJSVal( m_items[index] );
}

bool CProductionQueue::JSI_Cancel( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc == 1 );
	debug_assert( JSVAL_IS_INT(argv[0]) );

	int index = ToPrimitive<int>( argv[0] );

	if(index < 0 || index >= (int)m_items.size() )
	{
		JS_ReportError( cx, "Production queue index out of bounds: %d", index );
		return false;
	}

	CProductionItem* item = m_items[index];
	CEventCancelProduction evt( item->m_type, item->m_name );
	m_owner->DispatchEvent( &evt );
	m_items.erase( m_items.begin() + index );
	return true;
}

void CProductionQueue::ScriptingInit()
{
	AddProperty(L"length", static_cast<GetFn>(&CProductionQueue::JSI_GetLength));
	AddMethod<jsval_t, &CProductionQueue::JSI_Get>( "get", 1 );
	AddMethod<bool, &CProductionQueue::JSI_Cancel>( "cancel", 1 );
	CJSObject<CProductionQueue>::ScriptingInit("ProductionQueue");
}
