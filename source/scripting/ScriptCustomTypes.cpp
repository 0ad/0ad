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

JSBool Point2d_Constructor(JSContext* UNUSED(cx), JSObject* obj, uintN argc, jsval* argv, jsval* UNUSED(rval))
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
	AddMethod<CStr, &SColour::ToString>( "toString", 0 );
	AddProperty<float>( L"r", (float IJSObject::*)&SColour::r );
	AddProperty<float>( L"g", (float IJSObject::*)&SColour::g );
	AddProperty<float>( L"b", (float IJSObject::*)&SColour::b );
	AddProperty<float>( L"a", (float IJSObject::*)&SColour::a );

	CJSObject<SColour>::ScriptingInit( "Colour", SColour::Construct, 3 );
}

CStr SColour::ToString( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return "[object Colour: ( " + CStr(r) + ", " + CStr(g) + ", " + CStr(b) + ", " + CStr(a) + " )]";
}


JSBool SColour::Construct( JSContext* UNUSED(cx), JSObject* UNUSED(obj), uintN argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 3 );
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
