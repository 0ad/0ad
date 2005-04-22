// JavaScript interface to native code selection and group objects

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include "precompiled.h"
#include "JSInterface_Selection.h"
#include "scripting/JSCollection.h"
#include "Interact.h"

JSBool JSI_Selection::getSelection( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	*vp = OBJECT_TO_JSVAL( EntityCollection::CreateReference( &( g_Selection.m_selected ) ) );

	return( JS_TRUE );
}

JSBool JSI_Selection::setSelection( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	if( !JSVAL_IS_OBJECT( *vp ) )
	{
		JS_ReportError( context, "Not a valid Collection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	
	JSObject* selectionArray = JSVAL_TO_OBJECT( *vp );
	EntityCollection::CJSCollectionData* Info = (EntityCollection::CJSCollectionData*)JS_GetInstancePrivate( context, JSVAL_TO_OBJECT( *vp ), &EntityCollection::JSI_class, NULL );

	if( !Info )
	{
		JS_ReportError( context, "Not a valid Collection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}

	g_Selection.clearSelection();
	std::vector<HEntity>::iterator it;

	for( it = Info->m_Data->begin(); it < Info->m_Data->end(); it++ )
		g_Selection.addSelection( *it );

	return( JS_TRUE );
}

JSBool JSI_Selection::getGroups( JSContext* context, JSObject* obj, jsval id, jsval* vp )
{
	JSObject* groupsArray = JS_NewArrayObject( context, 0, NULL );

	JS_AddRoot( context, &groupsArray );

	for( i8 groupId = 0; groupId < MAX_GROUPS; groupId++ )
	{
		jsval v = OBJECT_TO_JSVAL( EntityCollection::CreateReference( &( g_Selection.m_groups[groupId] ) ) );
		JS_SetElement( context, groupsArray, groupId, &v );
	}

	*vp = OBJECT_TO_JSVAL( groupsArray );

	JS_RemoveRoot( context, &groupsArray );

	return( JS_TRUE );
}

JSBool JSI_Selection::setGroups( JSContext* context, JSObject* obj, jsval id, jsval* vp )
{
	JSObject* groupsArray;
	if( !JSVAL_IS_OBJECT( *vp ) || !JS_IsArrayObject( context, groupsArray = JSVAL_TO_OBJECT( *vp ) ) )
	{
		JS_ReportError( context, "Not a valid group array" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	for( i8 groupId = 0; groupId < MAX_GROUPS; groupId++ )
	{
		jsval v;
		if( JS_GetElement( context, groupsArray, groupId, &v ) && JSVAL_IS_OBJECT( v ) )
		{
			JSObject* group = JSVAL_TO_OBJECT( v );
			EntityCollection::CJSCollectionData* Info = (EntityCollection::CJSCollectionData*)JS_GetInstancePrivate( context, group, &EntityCollection::JSI_class, NULL );
			if( Info )
			{
				g_Selection.m_groups[groupId].clear();
				std::vector<HEntity>::iterator it;

				for( it = Info->m_Data->begin(); it < Info->m_Data->end(); it++ )
					g_Selection.addToGroup( groupId, *it );

			}
		}
	}
	
	return( JS_TRUE );
}

JSBool JSI_Selection::isValidContextOrder( JSContext* context, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	int orderCode;
	try
	{
		orderCode = g_ScriptingHost.ValueToInt( argv[0] );
	}
	catch( ... )
	{
		JS_ReportError( context, "Invalid order type" );
		*rval = BOOLEAN_TO_JSVAL( false );
		return( JS_TRUE );
	}
	*rval = BOOLEAN_TO_JSVAL( g_Selection.isContextValid( orderCode ) );
	return( JS_TRUE );
}

JSBool JSI_Selection::getContextOrder( JSContext* context, JSObject* obj, jsval id, jsval* vp )
{
	*vp = INT_TO_JSVAL( g_Selection.m_contextOrder );
	return( JS_TRUE );
}
 
JSBool JSI_Selection::setContextOrder( JSContext* context, JSObject* obj, jsval id, jsval* vp )
{
	int orderCode;
	try
	{
		orderCode = g_ScriptingHost.ValueToInt( *vp );
	}
	catch( ... )
	{
		JS_ReportError( context, "Invalid order type" );
		return( JS_TRUE );
	}
	if( !g_Selection.isContextValid( orderCode ) )
	{
		JS_ReportError( context, "Order is not valid in this context: %d", orderCode );
		return( JS_TRUE );
	}
	g_Selection.setContext( orderCode );
	return( JS_TRUE );
}
