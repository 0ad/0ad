// JavaScript interface to native code selection and group objects

// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)

#include "precompiled.h"
#include "JSInterface_Selection.h"
#include "ps/scripting/JSCollection.h"
#include "ps/Interact.h"
#include "simulation/Entity.h"

JSBool JSI_Selection::getSelection( JSContext* UNUSED(cx), JSObject* UNUSED(obj),
	jsval UNUSED(id), jsval* vp )
{
	*vp = OBJECT_TO_JSVAL( EntityCollection::CreateReference( &( g_Selection.m_selected ) ) );

	return( JS_TRUE );
}

JSBool JSI_Selection::SetSelection( JSContext* cx, JSObject* UNUSED(obj),
	jsval UNUSED(id), jsval* vp )
{
	if( !JSVAL_IS_OBJECT( *vp ) )
	{
		JS_ReportError( cx, "Not a valid Collection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	
	JSObject* selectionArray = JSVAL_TO_OBJECT( *vp );
	EntityCollection::CJSCollectionData* Info = (EntityCollection::CJSCollectionData*)JS_GetInstancePrivate( cx, selectionArray, &EntityCollection::JSI_class, NULL );

	if( !Info )
	{
		JS_ReportError( cx, "Not a valid Collection" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}

	g_Selection.ClearSelection();
	std::vector<HEntity>::iterator it;

	for( it = Info->m_Data->begin(); it < Info->m_Data->end(); it++ )
		g_Selection.AddSelection( *it );

	return( JS_TRUE );
}

JSBool JSI_Selection::getGroups( JSContext* cx, JSObject* UNUSED(obj),
	jsval UNUSED(id), jsval* vp )
{
	JSObject* groupsArray = JS_NewArrayObject( cx, 0, NULL );

	JS_AddRoot( cx, &groupsArray );

	for( i8 groupId = 0; groupId < MAX_GROUPS; groupId++ )
	{
		jsval v = OBJECT_TO_JSVAL( EntityCollection::CreateReference( &( g_Selection.m_groups[groupId] ) ) );
		JS_SetElement( cx, groupsArray, groupId, &v );
	}

	*vp = OBJECT_TO_JSVAL( groupsArray );

	JS_RemoveRoot( cx, &groupsArray );

	return( JS_TRUE );
}

JSBool JSI_Selection::setGroups( JSContext* cx, JSObject* UNUSED(obj),
	jsval UNUSED(id), jsval* vp )
{
	JSObject* groupsArray;
	if( !JSVAL_IS_OBJECT( *vp ) || !JS_IsArrayObject( cx, groupsArray = JSVAL_TO_OBJECT( *vp ) ) )
	{
		JS_ReportError( cx, "Not a valid group array" );
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	for( i8 groupId = 0; groupId < MAX_GROUPS; groupId++ )
	{
		jsval v;
		if( JS_GetElement( cx, groupsArray, groupId, &v ) && JSVAL_IS_OBJECT( v ) )
		{
			JSObject* group = JSVAL_TO_OBJECT( v );
			EntityCollection::CJSCollectionData* Info = (EntityCollection::CJSCollectionData*)JS_GetInstancePrivate( cx, group, &EntityCollection::JSI_class, NULL );
			if( Info )
			{
				g_Selection.m_groups[groupId].clear();
				std::vector<HEntity>::iterator it;

				for( it = Info->m_Data->begin(); it < Info->m_Data->end(); it++ )
					g_Selection.AddToGroup( groupId, *it );

			}
		}
	}
	
	return( JS_TRUE );
}

