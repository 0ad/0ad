// JavaScript interface to native code selection and group objects

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include "precompiled.h"
#include "JSInterface_Selection.h"
#include "Interact.h"

JSBool JSI_Selected::getProperty( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	if( g_Selection.m_selected.size() )
	{
		JSObject* entity = JS_NewObject( context, &JSI_Entity::JSI_class, NULL, NULL );
		JS_SetPrivate( context, entity, new HEntity( g_Selection.m_selected[0]->me ) );
		*vp = OBJECT_TO_JSVAL( entity );
		return( JS_TRUE );
	}
	else
	{
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
}

JSBool JSI_Selected::setProperty( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	g_Selection.clearSelection();
	JSObject* selection = JSVAL_TO_OBJECT( *vp );
	HEntity* entity = NULL;
	if( JSVAL_IS_OBJECT( *vp ) && ( entity = (HEntity*)JS_GetInstancePrivate( context, selection, &JSI_Entity::JSI_class, NULL ) ) )
	{
		g_Selection.addSelection( *entity );
	}
	else
		JS_ReportError( context, "[Entity] Invalid reference" );
	return( JS_TRUE );
}

JSBool JSI_Selection::getProperty( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	JSObject* selectionArray = JS_NewArrayObject( context, 0, NULL );
	std::vector<CEntity*>::iterator it; int i;
	for( it = g_Selection.m_selected.begin(), i = 0; it < g_Selection.m_selected.end(); it++, i++ )
	{
		JSObject* entity = JS_NewObject( context, &JSI_Entity::JSI_class, NULL, NULL );
		JS_SetPrivate( context, entity, new HEntity( (*it)->me ) );
		jsval j = OBJECT_TO_JSVAL( entity );
		JS_SetElement( context, selectionArray, i, &j );
	}
	// Also throw in the check/get/set context orders here.
	JS_DefineFunction( context, selectionArray, "isOrderTypeValid", JSI_Selection::isValidContextOrder, 1, 0 );
	JS_DefineProperty( context, selectionArray, "orderType", INT_TO_JSVAL( g_Selection.m_contextOrder ),
		JSI_Selection::getContextOrder, JSI_Selection::setContextOrder, 0 );
	*vp = OBJECT_TO_JSVAL( selectionArray );
	return( JS_TRUE );
}

JSBool JSI_Selection::setProperty( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	g_Selection.clearSelection();
	JSObject* selectionArray = JSVAL_TO_OBJECT( *vp );
	if( JSVAL_IS_NULL( *vp ) || !JSVAL_IS_OBJECT( *vp ) || !JS_IsArrayObject( context, selectionArray ) )
	{
		JS_ReportError( context, "Not an array" );
		return( JS_TRUE );
	}
	jsuint selectionCount;
	if( !JS_GetArrayLength( context, selectionArray, &selectionCount ) )
	{
		JS_ReportError( context, "Not an array" );
		return( JS_TRUE );
	}
	for( jsuint i = 0; i < selectionCount; i++ )
	{
		jsval entry;
		JS_GetElement( context, selectionArray, i, &entry );
		JSObject* selection = JSVAL_TO_OBJECT( entry );
		HEntity* entity = NULL;
		if( JSVAL_IS_OBJECT( entry ) && ( entity = (HEntity*)JS_GetInstancePrivate( context, JSVAL_TO_OBJECT( entry ), &JSI_Entity::JSI_class, NULL ) ) )
		{
			g_Selection.addSelection( &(**entity) );
		}
		else
			JS_ReportError( context, "[Entity] Invalid reference" );
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