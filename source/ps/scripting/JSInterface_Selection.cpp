// JavaScript interface to native code selection and group objects

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include "precompiled.h"
#include "JSInterface_Selection.h"
#include "Interact.h"

JSClass JSI_Selection::JSI_class =
{
	"EntityCollection", JSCLASS_HAS_PRIVATE,
	JSI_Selection::addProperty, JSI_Selection::removeProperty,
	JSI_Selection::getProperty, JSI_Selection::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSI_Selection::finalize,
	NULL, NULL, NULL, NULL
};

JSPropertySpec JSI_Selection::JSI_props[] =
{
	{ 0 }
};

JSFunctionSpec JSI_Selection::JSI_methods[] = 
{
	{ "toString", JSI_Selection::toString, 0, 0, 0 },
	{ 0 },
};

JSBool JSI_Selection::addProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	int i = JSVAL_TO_INT( id );
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );

	// Invalid index and/or value is not an object.

	if( !set || !JSVAL_IS_OBJECT( *vp ) || ( i < 0 ) )
		return( JS_TRUE );

	HEntity* e = (HEntity*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( *vp ), &JSI_Entity::JSI_class, NULL );

	// Value is not an entity.
	if( !e )
		return( JS_TRUE );

	if( i >= (int)set->capacity() ) set->resize( i + 1 );

	// Add to set.
	set->insert( set->begin() + id, *e );
	return( JS_TRUE );
}

JSBool JSI_Selection::removeProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
	{
		*vp = JSVAL_TRUE;
		return( JS_TRUE );
	}

	int i = JSVAL_TO_INT( id );
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );

	// Invalid index and/or value is not an object.

	if( !set || !JSVAL_IS_OBJECT( *vp ) || ( i < 0 ) || ( i >= (int)set->size() ) )
	{
		*vp = JSVAL_TRUE;
		return( JS_TRUE );
	}

	// Remove from set.
	set->erase( set->begin() + id );

	*vp = JSVAL_TRUE;
	return( JS_TRUE );
}

JSBool JSI_Selection::getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	int i = JSVAL_TO_INT( id );
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );

	// Invalid index and/or value is not an object.

	if( !set || ( i < 0 ) || ( i >= (int)set->size() ) )
	{
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}

	// Retrieve from set.
	JSObject* e = JS_NewObject( cx, &JSI_Entity::JSI_class, NULL, NULL );
	JS_SetPrivate( cx, e, new HEntity( set->at( i ) ) );

	*vp = OBJECT_TO_JSVAL( e );
	return( JS_TRUE );
}

JSBool JSI_Selection::setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );
	
	int i = JSVAL_TO_INT( id );
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );

	// Invalid index and/or value is not an object.

	if( !set || !JSVAL_IS_OBJECT( *vp ) || ( i < 0 ) || ( i >= (int)set->size() ) )
		return( JS_TRUE );

	HEntity* e = (HEntity*)JS_GetInstancePrivate( cx, JSVAL_TO_OBJECT( *vp ), &JSI_Entity::JSI_class, NULL );

	// Value is not an entity.
	if( !e )
	{
		JSObject* c = JS_NewObject( cx, &JSI_Entity::JSI_class, NULL, NULL );
		JS_SetPrivate( cx, c, new HEntity( set->at( i ) ) );

		*vp = OBJECT_TO_JSVAL( e );
		return( JS_TRUE );
	}

	// Write into set.
	set->at( id ) = *e;

	return( JS_TRUE );
}

void JSI_Selection::finalize( JSContext* cx, JSObject* obj )
{
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );
	if( set )
		delete( set );
}

void JSI_Selection::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, NULL, 0, JSI_props, JSI_methods, NULL, NULL );
}

JSBool JSI_Selection::toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetPrivate( cx, obj );

	wchar_t buffer[256];
	int len=swprintf( buffer, 256, L"[object EntityCollection: %d entities]", set->size() );
	buffer[255] = 0;
	if (len < 0 || len > 255) len=255;
	utf16string u16str(buffer, buffer+len);
	*rval = STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, u16str.c_str() ) );
	return( JS_TRUE );
}

JSBool JSI_Selection::getSelection( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	std::vector<HEntity>* set = new std::vector<HEntity>;
	std::vector<CEntity*>::iterator it;

	for( it = g_Selection.m_selected.begin(); it != g_Selection.m_selected.end(); it++ )
		set->push_back( (*it)->me );
	
	JSObject* selectionArray = JS_NewObject( context, &JSI_Selection::JSI_class, NULL, NULL );
	JS_SetPrivate( context, selectionArray, set );

	JS_DefineFunction( context, selectionArray, "isOrderTypeValid", JSI_Selection::isValidContextOrder, 1, 0 );
	JS_DefineProperty( context, selectionArray, "orderType", INT_TO_JSVAL( g_Selection.m_contextOrder ),
	JSI_Selection::getContextOrder, JSI_Selection::setContextOrder, 0 );

	*vp = OBJECT_TO_JSVAL( selectionArray );
	return( JS_TRUE );
}

JSBool JSI_Selection::setSelection( JSContext* context, JSObject* globalObject, jsval id, jsval* vp )
{
	if( !JSVAL_IS_OBJECT( *vp ) )
	{
		JS_ReportError( context, "Not a valid EntityCollection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	JSObject* selectionArray = JSVAL_TO_OBJECT( *vp );

	std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetInstancePrivate( context, JSVAL_TO_OBJECT( *vp ), &JSI_Selection::JSI_class, NULL );

	if( !set )
	{
		JS_ReportError( context, "Not a valid EntityCollection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}

	g_Selection.clearSelection();
	std::vector<HEntity>::iterator it;

	for( it = set->begin(); it < set->end(); it++ )
		g_Selection.addSelection( &(**it) );
/* old-style load from array, here for reference
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
*/
	
	return( JS_TRUE );
}

JSBool JSI_Selection::getGroups( JSContext* context, JSObject* obj, jsval id, jsval* vp )
{
	JSObject* groupsArray = JS_NewArrayObject( context, 0, NULL );

	for( i8 groupId = 0; groupId < MAX_GROUPS; groupId++ )
	{
		JSObject* group = JS_NewObject( context, &JSI_Selection::JSI_class, NULL, NULL );

		std::vector<HEntity>* set = new std::vector<HEntity>;
		std::vector<CEntity*>::iterator it;

		for( it = g_Selection.m_groups[groupId].begin(); it < g_Selection.m_groups[groupId].end(); it++ )
			set->push_back( (*it)->me );

		JS_SetPrivate( context, group, set );
		
		jsval v = OBJECT_TO_JSVAL( group );
		JS_SetElement( context, groupsArray, groupId, &v );
	}

	*vp = OBJECT_TO_JSVAL( groupsArray );
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
			std::vector<HEntity>* set = (std::vector<HEntity>*)JS_GetInstancePrivate( context, group, &JSI_Selection::JSI_class, NULL );
			if( set )
			{
				g_Selection.m_groups[groupId].clear();
				std::vector<HEntity>::iterator it;

				for( it = set->begin(); it < set->end(); it++ )
					g_Selection.addToGroup( groupId, &(**it) );
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
