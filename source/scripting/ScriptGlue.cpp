#include "precompiled.h"

#include "ScriptGlue.h"
#include "CConsole.h"
#include "CStr.h"
#include "EntityHandles.h"
#include "Entity.h"
#include "EntityManager.h"
#include "BaseEntityCollection.h"
#include "scripting/JSInterface_Entity.h"
#include "scripting/JSInterface_BaseEntity.h"
#include "scripting/JSInterface_Vector3D.h"

extern CConsole* g_Console;

// Parameters for the table are:

// 1: The name the function will be called as from script
// 2: The number of aguments this function expects
// 3: Depreciated, always zero
// 4: Reserved for future use, always zero

JSFunctionSpec ScriptFunctionTable[] = 
{
	{"WriteLog", WriteLog, 1, 0, 0},
	{"writeConsole", writeConsole, 1, 0, 0 },
	{"getEntityByHandle", getEntityByHandle, 1, 0, 0 },
	{"getEntityTemplate", getEntityTemplate, 1, 0, 0 },
	{0, 0, 0, 0, 0}, 
};

// Allow scripts to output to the global log file
JSBool WriteLog(JSContext * context, JSObject * globalObject, unsigned int argc, jsval * argv, jsval * rval)
{
	if (argc < 1)
		return JS_FALSE;

	for (int i = 0; i < (int)argc; i++)
	{
		if (JSVAL_IS_INT(argv[i]))
		{
			printf("%d", JSVAL_TO_INT(argv[i]));
		}

		if (JSVAL_IS_DOUBLE(argv[i]))
		{
			double d = g_ScriptingHost.ValueToDouble(argv[i]);
			printf("%e", d);
		}

		if (JSVAL_IS_STRING(argv[i]))
		{
			JSString * str = JS_ValueToString(context, argv[i]);
			char * chars = JS_GetStringBytes(str);
			printf(chars);
		}
	}

	printf("\n");

	return JS_TRUE;
}

JSBool writeConsole( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr output = g_ScriptingHost.ValueToString( argv[0] );
	g_Console->InsertMessage( L"%S", (const char*)output );
	return( JS_TRUE );
}

JSBool getEntityByHandle( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	i32 handle;
	try
	{
		handle = g_ScriptingHost.ValueToInt( argv[0] );
	}
	catch( ... )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	HEntity* v = g_EntityManager.getByHandle( (u16)handle );
	if( !v )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	JSObject* entity = JS_NewObject( context, &JSI_Entity::JSI_class, NULL, NULL );
	JS_SetPrivate( context, entity, v );
	*rval = OBJECT_TO_JSVAL( entity );
	return( JS_TRUE );
}

JSBool getEntityTemplate( JSContext* context, JSObject* globalObject, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 1 );
	CStr templateName;
	try
	{
		templateName = g_ScriptingHost.ValueToString( argv[0] );
	}
	catch( ... )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	CBaseEntity* v = g_EntityTemplateCollection.getTemplate( templateName );
	if( !v )
	{
		*rval = JSVAL_NULL;
		return( JS_TRUE );
	}
	JSObject* baseEntity = JS_NewObject( context, &JSI_BaseEntity::JSI_class, NULL, NULL );
	JS_SetPrivate( context, baseEntity, v );
	*rval = OBJECT_TO_JSVAL( baseEntity );
	return( JS_TRUE );
}

