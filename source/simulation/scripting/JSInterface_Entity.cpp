#include "precompiled.h"

#include "JSInterface_Entity.h"
#include "scripting/JSInterface_BaseEntity.h"
#include "scripting/JSInterface_Vector3D.h"
#include "EntityHandles.h"
#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "CConsole.h"

JSClass JSI_Entity::JSI_class = {
	"Entity", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_Entity::getProperty, JSI_Entity::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JSI_Entity::finalize,
	NULL, NULL, NULL, NULL 
};

JSPropertySpec JSI_Entity::JSI_props[] = 
{
	{ 0 }
};

JSFunctionSpec JSI_Entity::JSI_methods[] = 
{
	{ "toString", JSI_Entity::toString, 0, 0, 0 },
	{ 0 }
};

JSBool JSI_Entity::getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	HEntity* e = (HEntity*)JS_GetPrivate( cx, obj );
	if( !e )
	{
		*vp = JSVAL_NULL;
		return( JS_TRUE );
	}
	CStr propName = g_ScriptingHost.ValueToString( id );
	
	if( (*e)->m_properties.find( propName ) != (*e)->m_properties.end() )
	{
		*vp = *((*e)->m_properties[propName]);
		return( JS_TRUE );
	}
	return( JS_TRUE );
}

JSBool JSI_Entity::setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	HEntity* e = (HEntity*)JS_GetPrivate( cx, obj );
	CStr propName = g_ScriptingHost.ValueToString( id );
	
	if( (*e)->m_properties.find( propName ) != (*e)->m_properties.end() )
	{
		*((*e)->m_properties[propName]) = *vp;
		(*e)->rebuild( propName );
		return( JS_TRUE );
	}
	return( JS_TRUE );
}

JSBool JSI_Entity::construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 2 );
	CBaseEntity* baseEntity;
	CVector3D position;
	float orientation = 0.0f;
	JSObject* jsBaseEntity = JSVAL_TO_OBJECT( argv[0] );
	if( JSVAL_IS_OBJECT( argv[0] ) && ( JS_GetClass( jsBaseEntity ) == &JSI_BaseEntity::JSI_class ) )
	{
		baseEntity = (CBaseEntity*)JS_GetPrivate( cx, jsBaseEntity );
	}
	else
	{
		CStr templateName;
		try
		{
			templateName = g_ScriptingHost.ValueToString( argv[0] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			*rval = JSVAL_NULL;
			return( JS_TRUE );
		}
		baseEntity = g_EntityTemplateCollection.getTemplate( templateName );
	}
	if( !baseEntity )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	JSObject* jsVector3D = JSVAL_TO_OBJECT( argv[1] );
	if( JSVAL_IS_OBJECT( argv[1] ) && ( JS_GetClass( jsVector3D ) == &JSI_Vector3D::JSI_class ) )
		position = *( ( (JSI_Vector3D::Vector3D_Info*)JS_GetPrivate( cx, jsVector3D ) )->vector );
	if( argc >= 3 )
	{
		try
		{
			orientation = (float)g_ScriptingHost.ValueToDouble( argv[2] );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			orientation = 0.0f;
		}
	}

	HEntity* handle = new HEntity( g_EntityManager.create( baseEntity, position, orientation ) );
	CMessage message( CMessage::EMSG_INIT );
	(*handle)->dispatch( &message );
	JSObject* entity = JS_NewObject( cx, &JSI_Entity::JSI_class, NULL, NULL );
	JS_SetPrivate( cx, entity, handle );
	*rval = OBJECT_TO_JSVAL( entity );
	return( JS_TRUE );
}

void JSI_Entity::finalize( JSContext* cx, JSObject* obj )
{
	delete( (HEntity*)JS_GetPrivate( cx, obj ) );
}

void JSI_Entity::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, construct, 2, JSI_props, JSI_methods, NULL, NULL );
}

JSBool JSI_Entity::toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	HEntity* e = (HEntity*)JS_GetPrivate( cx, obj );

	char buffer[256];
	snprintf( buffer, 256, "[object Entity: \"%s\" (%s)]", (const TCHAR*)(*e)->m_name, (const TCHAR*)(*e)->m_base->m_name );
	buffer[255] = 0;
	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, buffer ) );
	return( JS_TRUE );
}
