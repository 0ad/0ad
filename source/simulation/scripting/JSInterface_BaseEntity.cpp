#include "precompiled.h"

#include "JSInterface_BaseEntity.h"
#include "BaseEntity.h"
#include "EntityHandles.h"

JSClass JSI_BaseEntity::JSI_class = {
	"EntityTemplate", JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub,
	JSI_BaseEntity::getProperty, JSI_BaseEntity::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL 
};

JSPropertySpec JSI_BaseEntity::JSI_props[] = 
{
	{ 0 }
};

JSFunctionSpec JSI_BaseEntity::JSI_methods[] = 
{
	{ "toString", JSI_BaseEntity::toString, 0, 0, 0 },
	{ 0 }
};

JSBool JSI_BaseEntity::getProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	CBaseEntity* e = (CBaseEntity*)JS_GetPrivate( cx, obj );
	CStr propName = g_ScriptingHost.ValueToString( id );
	
	if( e->m_properties.find( propName ) != e->m_properties.end() )
	{
		*vp = e->m_properties[propName]->tojsval();
		return( JS_TRUE );
	}
	else
		JS_ReportError( cx, "No such property on %s: %s", (const char*)e->m_name, (const char*)propName );

	return( JS_TRUE );
}

JSBool JSI_BaseEntity::setProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
{
	CBaseEntity* e = (CBaseEntity*)JS_GetPrivate( cx, obj );
	CStr propName = g_ScriptingHost.ValueToString( id );
	
	if( e->m_properties.find( propName ) != e->m_properties.end() )
	{
		*(e->m_properties[propName]) = *vp;
		e->rebuild( propName );
		return( JS_TRUE );
	}
	else
		JS_ReportError( cx, "No such property on %s: %s", (const char*)e->m_name, (const char*)propName );

	return( JS_TRUE );
}

void JSI_BaseEntity::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, NULL, 0, JSI_props, JSI_methods, NULL, NULL );
}

JSBool JSI_BaseEntity::toString( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval )
{
	CBaseEntity* e = (CBaseEntity*)JS_GetPrivate( cx, obj );

	char buffer[256];
	snprintf( buffer, 256, "[object EntityTemplate: %s]", (const TCHAR*)e->m_name );
	buffer[255] = 0;
	*rval = STRING_TO_JSVAL( JS_NewStringCopyZ( cx, buffer ) );
	return( JS_TRUE );
}
