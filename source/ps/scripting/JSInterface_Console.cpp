// JavaScript interface to native code selection and group objects

#include "precompiled.h"
#include "JSInterface_Console.h"
#include "ps/CConsole.h"
#include "scripting/JSConversions.h"

JSClass JSI_Console::JSI_class =
{
	"Console", 0,
	JS_PropertyStub, JS_PropertyStub,
	JSI_Console::getProperty, JSI_Console::setProperty,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL
};

JSPropertySpec JSI_Console::JSI_props[] =
{
	{ "visible", JSI_Console::console_visible, JSPROP_ENUMERATE },
	{ 0 }
};

JSFunctionSpec JSI_Console::JSI_methods[] = 
{
	{ "write", JSI_Console::writeConsole, 1, 0, 0 },
	{ 0 },
};

JSBool JSI_Console::getProperty( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	int i = JSVAL_TO_INT( id );

	switch( i )
	{
	case console_visible:
		*vp = BOOLEAN_TO_JSVAL( g_Console->IsActive() ); return( JS_TRUE );
	default:
		*vp = JSVAL_NULL; return( JS_TRUE );
	}	
}

JSBool JSI_Console::setProperty( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval id, jsval* vp )
{
	if( !JSVAL_IS_INT( id ) )
		return( JS_TRUE );

	int i = JSVAL_TO_INT( id );

	switch( i )
	{
	case console_visible:
		try
		{
			g_Console->SetVisible( ToPrimitive<bool>( *vp ) );
			return( JS_TRUE );
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
			return( JS_TRUE );
		}
	default:
		return( JS_TRUE );
	}	
}

void JSI_Console::init()
{
	g_ScriptingHost.DefineCustomObjectType( &JSI_class, NULL, 0, JSI_props, JSI_methods, NULL, NULL );
}

JSBool JSI_Console::getConsole( JSContext* cx, JSObject* UNUSED(obj), jsval UNUSED(id), jsval* vp )
{
	JSObject* console = JS_NewObject( cx, &JSI_Console::JSI_class, NULL, NULL );
	*vp = OBJECT_TO_JSVAL( console );
	return( JS_TRUE );
}

JSBool JSI_Console::writeConsole( JSContext* UNUSED(context), JSObject* UNUSED(globalObject), uintN argc, jsval* argv, jsval* UNUSED(rval) )
{
	debug_assert( argc >= 1 );
	CStrW output;
	for( unsigned int i = 0; i < argc; i++ )
	{
		try
		{
			CStrW arg = g_ScriptingHost.ValueToUCString( argv[i] );
			output += arg;
		}
		catch( PSERROR_Scripting_ConversionFailed )
		{
		}
	}

	// TODO: What if the console has been destroyed already?
	if (g_Console)
		g_Console->InsertMessage( L"%ls", output.c_str() );

	return( JS_TRUE );
}
