#include "precompiled.h"

#include "ScriptingHost.h"
#include "ScriptCustomTypes.h"

// POINT2D

JSClass Point2dClass = 
{
	"Point2d", 0,
	JS_PropertyStub, JS_PropertyStub,
	JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub,
	JS_ConvertStub, JS_FinalizeStub
};

JSPropertySpec Point2dProperties[] = 
{
	{"x",	0,	JSPROP_ENUMERATE},
	{"y",	1,	JSPROP_ENUMERATE},
	{0}
};

JSBool Point2d_Constructor(JSContext* UNUSEDPARAM(cx), JSObject* obj, uintN argc, jsval* argv, jsval* UNUSEDPARAM(rval))
{
	if (argc == 2)
	{
		g_ScriptingHost.SetObjectProperty(obj, "x", argv[0]);
		g_ScriptingHost.SetObjectProperty(obj, "y", argv[1]);
	}
	else
	{
		jsval zero = INT_TO_JSVAL(0);
		g_ScriptingHost.SetObjectProperty(obj, "x", zero);
		g_ScriptingHost.SetObjectProperty(obj, "y", zero);
	}

	return JS_TRUE;
}

// Colour

void SColour::SColourInit( float _r, float _g, float _b, float _a )
{
	r = _r; g = _g; b = _b; a = _a;
}

void SColour::ScriptingInit()
{
	AddMethod<jsval, &SColour::ToString>( "toString", 0 );
	AddClassProperty<float>( L"r", (float IJSObject::*)&SColour::r );
	AddClassProperty<float>( L"g", (float IJSObject::*)&SColour::g );
	AddClassProperty<float>( L"b", (float IJSObject::*)&SColour::b );
	AddClassProperty<float>( L"a", (float IJSObject::*)&SColour::a );

	CJSObject<SColour>::ScriptingInit( "Colour", SColour::Construct, 3 );
}

jsval SColour::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	wchar_t buffer[256];
	
	swprintf( buffer, 256, L"[object Colour: ( %f, %f, %f, %f )]", r, g, b, a );
	buffer[255] = 0;

	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}


JSBool SColour::Construct( JSContext* cx, JSObject* obj, unsigned int argc, jsval* argv, jsval* rval )
{
	assert( argc >= 3 );
	float alpha = 1.0;
	if( argc >= 4 ) alpha = ToPrimitive<float>( argv[3] );

	SColour* col = new SColour( ToPrimitive<float>( argv[0] ),
								ToPrimitive<float>( argv[1] ),
								ToPrimitive<float>( argv[2] ),
								alpha );

	col->m_EngineOwned = false;

	*rval = OBJECT_TO_JSVAL( col->GetScript() );

	return( JS_TRUE );
}

// (Simon) Added this to prevent a deep copy, which evidently makes direct
// copies of the heap allocated objects within CJSObject, which eventually
// goes boom
SColour &SColour::operator = (const SColour &o)
{
	r=o.r;
	g=o.g;
	b=o.b;
	a=o.a;

	return *this;
}
