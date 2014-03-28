/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_GUITypes.h"
#include "ps/CStr.h"

/**** GUISize ****/
JSClass JSI_GUISize::JSI_class = {
	"GUISize", 0,
		JS_PropertyStub, JS_DeletePropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, NULL,
		NULL, NULL, NULL, JSI_GUISize::construct
};

JSPropertySpec JSI_GUISize::JSI_props[] = 
{
	{ "left",		0,	JSPROP_ENUMERATE},
	{ "top",		1,	JSPROP_ENUMERATE},
	{ "right",		2,	JSPROP_ENUMERATE},
	{ "bottom",		3,	JSPROP_ENUMERATE},
	{ "rleft",		4,	JSPROP_ENUMERATE},
	{ "rtop",		5,	JSPROP_ENUMERATE},
	{ "rright",		6,	JSPROP_ENUMERATE},
	{ "rbottom",	7,	JSPROP_ENUMERATE},
	{ 0 }
};

JSFunctionSpec JSI_GUISize::JSI_methods[] = 
{
	JS_FS("toString", JSI_GUISize::toString, 0, 0),
	JS_FS_END
};

JSBool JSI_GUISize::construct(JSContext* cx, uint argc, jsval* vp)
{
	JSObject* obj = JS_NewObject(cx, &JSI_GUISize::JSI_class, NULL, NULL);

	if (argc == 8)
	{
		JS::RootedValue v0(cx, JS_ARGV(cx, vp)[0]);
		JS::RootedValue v1(cx, JS_ARGV(cx, vp)[1]);
		JS::RootedValue v2(cx, JS_ARGV(cx, vp)[2]);
		JS::RootedValue v3(cx, JS_ARGV(cx, vp)[3]);
		JS::RootedValue v4(cx, JS_ARGV(cx, vp)[4]);
		JS::RootedValue v5(cx, JS_ARGV(cx, vp)[5]);
		JS::RootedValue v6(cx, JS_ARGV(cx, vp)[6]);
		JS::RootedValue v7(cx, JS_ARGV(cx, vp)[7]);
		JS_SetProperty(cx, obj, "left",		v0.address());
		JS_SetProperty(cx, obj, "top",		v1.address());
		JS_SetProperty(cx, obj, "right",	v2.address());
		JS_SetProperty(cx, obj, "bottom",	v3.address());
		JS_SetProperty(cx, obj, "rleft",	v4.address());
		JS_SetProperty(cx, obj, "rtop",		v5.address());
		JS_SetProperty(cx, obj, "rright",	v6.address());
		JS_SetProperty(cx, obj, "rbottom",	v7.address());
	}
	else if (argc == 4)
	{
		JS::RootedValue zero(cx, JSVAL_ZERO);
		JS::RootedValue v0(cx, JS_ARGV(cx, vp)[0]);
		JS::RootedValue v1(cx, JS_ARGV(cx, vp)[1]);
		JS::RootedValue v2(cx, JS_ARGV(cx, vp)[2]);
		JS::RootedValue v3(cx, JS_ARGV(cx, vp)[3]);
		JS_SetProperty(cx, obj, "left",		v0.address());
		JS_SetProperty(cx, obj, "top",		v1.address());
		JS_SetProperty(cx, obj, "right",	v2.address());
		JS_SetProperty(cx, obj, "bottom",	v3.address());
		JS_SetProperty(cx, obj, "rleft",	zero.address());
		JS_SetProperty(cx, obj, "rtop",		zero.address());
		JS_SetProperty(cx, obj, "rright",	zero.address());
		JS_SetProperty(cx, obj, "rbottom",	zero.address());
	}
	else
	{
		JS::RootedValue zero(cx, JSVAL_ZERO);
		JS_SetProperty(cx, obj, "left",		zero.address());
		JS_SetProperty(cx, obj, "top",		zero.address());
		JS_SetProperty(cx, obj, "right",	zero.address());
		JS_SetProperty(cx, obj, "bottom",	zero.address());
		JS_SetProperty(cx, obj, "rleft",	zero.address());
		JS_SetProperty(cx, obj, "rtop",		zero.address());
		JS_SetProperty(cx, obj, "rright",	zero.address());
		JS_SetProperty(cx, obj, "rbottom",	zero.address());
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}

// Produces "10", "-10", "50%", "50%-10", "50%+10", etc
CStr ToPercentString(double pix, double per)
{
	if (per == 0)
		return CStr::FromDouble(pix);
	else
		return CStr::FromDouble(per)+"%"+( pix == 0.0 ? CStr() : pix > 0.0 ? CStr("+")+CStr::FromDouble(pix) : CStr::FromDouble(pix) );
}

JSBool JSI_GUISize::toString(JSContext* cx, uint argc, jsval* vp)
{
	UNUSED2(argc);

	CStr buffer;

	try
	{
		ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
		double val, valr;
#define SIDE(side) \
		pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), #side, val); \
		pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "r"#side, valr); \
		buffer += ToPercentString(val, valr);
		SIDE(left);
		buffer += " ";
		SIDE(top);
		buffer += " ";
		SIDE(right);
		buffer += " ";
		SIDE(bottom);
#undef SIDE
	}
	catch (PSERROR_Scripting_ConversionFailed&)
	{
		JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "<Error converting value to numbers>")));
		return JS_TRUE;
	}

	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer.c_str())));
	return JS_TRUE;
}


/**** GUIColor ****/


JSClass JSI_GUIColor::JSI_class = {
	"GUIColor", 0,
		JS_PropertyStub, JS_DeletePropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, NULL,
		NULL, NULL, NULL, JSI_GUIColor::construct
};

JSPropertySpec JSI_GUIColor::JSI_props[] = 
{
	{ "r",	0,	JSPROP_ENUMERATE},
	{ "g",	1,	JSPROP_ENUMERATE},
	{ "b",	2,	JSPROP_ENUMERATE},
	{ "a",	3,	JSPROP_ENUMERATE},
	{ 0 }
};

JSFunctionSpec JSI_GUIColor::JSI_methods[] = 
{
	JS_FS("toString", JSI_GUIColor::toString, 0, 0),
	JS_FS_END
};

JSBool JSI_GUIColor::construct(JSContext* cx, uint argc, jsval* vp)
{
	JSObject* obj = JS_NewObject(cx, &JSI_GUIColor::JSI_class, NULL, NULL);

	if (argc == 4)
	{
		JS_SetProperty(cx, obj, "r", &JS_ARGV(cx, vp)[0]);
		JS_SetProperty(cx, obj, "g", &JS_ARGV(cx, vp)[1]);
		JS_SetProperty(cx, obj, "b", &JS_ARGV(cx, vp)[2]);
		JS_SetProperty(cx, obj, "a", &JS_ARGV(cx, vp)[3]);
	}
	else
	{
		// Nice magenta:
		JS::RootedValue c(cx, JS::NumberValue(1.0));
		JS_SetProperty(cx, obj, "r", c.address());
		JS_SetProperty(cx, obj, "b", c.address());
		JS_SetProperty(cx, obj, "a", c.address());
		c = JS::NumberValue(0.0);
		JS_SetProperty(cx, obj, "g", c.address());
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}

JSBool JSI_GUIColor::toString(JSContext* cx, uint argc, jsval* vp)
{
	UNUSED2(argc);

	double r, g, b, a;
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "r", r);
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "g", g);
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "b", b);
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "a", a);
	char buffer[256];
	// Convert to integers, to be compatible with the GUI's string SetSetting
	snprintf(buffer, 256, "%d %d %d %d",
		(int)(255.0 * r),
		(int)(255.0 * g),
		(int)(255.0 * b),
		(int)(255.0 * a));
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer)));
	return JS_TRUE;
}

/**** GUIMouse ****/


JSClass JSI_GUIMouse::JSI_class = {
	"GUIMouse", 0,
		JS_PropertyStub, JS_DeletePropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, NULL,
		NULL, NULL, NULL, JSI_GUIMouse::construct
};

JSPropertySpec JSI_GUIMouse::JSI_props[] = 
{
	{ "x",			0,	JSPROP_ENUMERATE},
	{ "y",			1,	JSPROP_ENUMERATE},
	{ "buttons",	2,	JSPROP_ENUMERATE},
	{ 0 }
};

JSFunctionSpec JSI_GUIMouse::JSI_methods[] = 
{
	JS_FS("toString", JSI_GUIMouse::toString, 0, 0),
	JS_FS_END
};

JSBool JSI_GUIMouse::construct(JSContext* cx, uint argc, jsval* vp)
{
	JSObject* obj = JS_NewObject(cx, &JSI_GUIMouse::JSI_class, NULL, NULL);

	if (argc == 3)
	{
		JS::RootedValue v0(cx, JS_ARGV(cx, vp)[0]);
		JS::RootedValue v1(cx, JS_ARGV(cx, vp)[1]);
		JS::RootedValue v2(cx, JS_ARGV(cx, vp)[2]);
		JS_SetProperty(cx, obj, "x", v0.address());
		JS_SetProperty(cx, obj, "y", v1.address());
		JS_SetProperty(cx, obj, "buttons", v2.address());
	}
	else
	{
		JS::RootedValue zero (cx, JS::NumberValue(0));
		JS_SetProperty(cx, obj, "x", zero.address());
		JS_SetProperty(cx, obj, "y", zero.address());
		JS_SetProperty(cx, obj, "buttons", zero.address());
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}

JSBool JSI_GUIMouse::toString(JSContext* cx, uint argc, jsval* vp)
{
	UNUSED2(argc);

	i32 x, y, buttons;
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "x", x);
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "y", y);
	pScriptInterface->GetProperty(JS_THIS_VALUE(cx, vp), "buttons", buttons);

	char buffer[256];
	snprintf(buffer, 256, "%d %d %d", x, y, buttons);
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer)));
	return JS_TRUE;
}


// Initialise all the types at once:
void JSI_GUITypes::init(ScriptInterface& scriptInterface)
{
	scriptInterface.DefineCustomObjectType(&JSI_GUISize::JSI_class,  JSI_GUISize::construct,  1, JSI_GUISize::JSI_props,  JSI_GUISize::JSI_methods,  NULL, NULL);
	scriptInterface.DefineCustomObjectType(&JSI_GUIColor::JSI_class, JSI_GUIColor::construct, 1, JSI_GUIColor::JSI_props, JSI_GUIColor::JSI_methods, NULL, NULL);
	scriptInterface.DefineCustomObjectType(&JSI_GUIMouse::JSI_class, JSI_GUIMouse::construct, 1, JSI_GUIMouse::JSI_props, JSI_GUIMouse::JSI_methods, NULL, NULL);
}
