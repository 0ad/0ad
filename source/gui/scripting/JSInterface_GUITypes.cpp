/* Copyright (C) 2009 Wildfire Games.
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
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub,
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
	{ "toString", JSI_GUISize::toString, 0, 0 },
	{ 0 }
};

JSBool JSI_GUISize::construct(JSContext* cx, uintN argc, jsval* vp)
{
	JSObject* obj = JS_NewObject(cx, &JSI_GUISize::JSI_class, NULL, NULL);

	if (argc == 8)
	{
		JS_SetProperty(cx, obj, "left",		&JS_ARGV(cx, vp)[0]);
		JS_SetProperty(cx, obj, "top",		&JS_ARGV(cx, vp)[1]);
		JS_SetProperty(cx, obj, "right",	&JS_ARGV(cx, vp)[2]);
		JS_SetProperty(cx, obj, "bottom",	&JS_ARGV(cx, vp)[3]);
		JS_SetProperty(cx, obj, "rleft",	&JS_ARGV(cx, vp)[4]);
		JS_SetProperty(cx, obj, "rtop",		&JS_ARGV(cx, vp)[5]);
		JS_SetProperty(cx, obj, "rright",	&JS_ARGV(cx, vp)[6]);
		JS_SetProperty(cx, obj, "rbottom",	&JS_ARGV(cx, vp)[7]);
	}
	else if (argc == 4)
	{
		jsval zero = JSVAL_ZERO;
		JS_SetProperty(cx, obj, "left",		&JS_ARGV(cx, vp)[0]);
		JS_SetProperty(cx, obj, "top",		&JS_ARGV(cx, vp)[1]);
		JS_SetProperty(cx, obj, "right",	&JS_ARGV(cx, vp)[2]);
		JS_SetProperty(cx, obj, "bottom",	&JS_ARGV(cx, vp)[3]);
		JS_SetProperty(cx, obj, "rleft",	&zero);
		JS_SetProperty(cx, obj, "rtop",		&zero);
		JS_SetProperty(cx, obj, "rright",	&zero);
		JS_SetProperty(cx, obj, "rbottom",	&zero);
	}
	else
	{
		jsval zero = JSVAL_ZERO;
		JS_SetProperty(cx, obj, "left",		&zero);
		JS_SetProperty(cx, obj, "top",		&zero);
		JS_SetProperty(cx, obj, "right",	&zero);
		JS_SetProperty(cx, obj, "bottom",	&zero);
		JS_SetProperty(cx, obj, "rleft",	&zero);
		JS_SetProperty(cx, obj, "rtop",		&zero);
		JS_SetProperty(cx, obj, "rright",	&zero);
		JS_SetProperty(cx, obj, "rbottom",	&zero);
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

JSBool JSI_GUISize::toString(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(argc);

	CStr buffer;

	try
	{
#define SIDE(side) buffer += ToPercentString(g_ScriptingHost.GetObjectProperty_Double(JS_THIS_OBJECT(cx, vp), #side), g_ScriptingHost.GetObjectProperty_Double(JS_THIS_OBJECT(cx, vp), "r"#side));
		SIDE(left);
		buffer += " ";
		SIDE(top);
		buffer += " ";
		SIDE(right);
		buffer += " ";
		SIDE(bottom);
#undef SIDE
	}
	catch (PSERROR_Scripting_ConversionFailed)
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
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub,
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
	{ "toString", JSI_GUIColor::toString, 0, 0 },
	{ 0 }
};

JSBool JSI_GUIColor::construct(JSContext* cx, uintN argc, jsval* vp)
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
		jsval c;
		if (!JS_NewNumberValue(cx, 1.0, &c))
			return JS_FALSE;
		JS_SetProperty(cx, obj, "r", &c);
		JS_SetProperty(cx, obj, "b", &c);
		JS_SetProperty(cx, obj, "a", &c);
		if (!JS_NewNumberValue(cx, 0.0, &c))
			return JS_FALSE;
		JS_SetProperty(cx, obj, "g", &c);
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}

JSBool JSI_GUIColor::toString(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(argc);

	jsdouble r, g, b, a;
	if (!JS_ValueToNumber(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "r"), &r)) return JS_FALSE;
	if (!JS_ValueToNumber(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "g"), &g)) return JS_FALSE;
	if (!JS_ValueToNumber(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "b"), &b)) return JS_FALSE;
	if (!JS_ValueToNumber(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "a"), &a)) return JS_FALSE;

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
		JS_PropertyStub, JS_PropertyStub,
		JS_PropertyStub, JS_StrictPropertyStub,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub,
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
	{ "toString", JSI_GUIMouse::toString, 0, 0 },
	{ 0 }
};

JSBool JSI_GUIMouse::construct(JSContext* cx, uintN argc, jsval* vp)
{
	JSObject* obj = JS_NewObject(cx, &JSI_GUIMouse::JSI_class, NULL, NULL);

	if (argc == 3)
	{
		JS_SetProperty(cx, obj, "x", &JS_ARGV(cx, vp)[0]);
		JS_SetProperty(cx, obj, "y", &JS_ARGV(cx, vp)[1]);
		JS_SetProperty(cx, obj, "buttons", &JS_ARGV(cx, vp)[2]);
	}
	else
	{
		jsval zero = JSVAL_ZERO;
		JS_SetProperty(cx, obj, "x", &zero);
		JS_SetProperty(cx, obj, "y", &zero);
		JS_SetProperty(cx, obj, "buttons", &zero);
	}

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}

JSBool JSI_GUIMouse::toString(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(argc);

	int32 x, y, buttons;
	if (!JS_ValueToECMAInt32(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "x"), &x)) return JS_FALSE;
	if (!JS_ValueToECMAInt32(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "y"), &y)) return JS_FALSE;
	if (!JS_ValueToECMAInt32(cx, g_ScriptingHost.GetObjectProperty(JS_THIS_OBJECT(cx, vp), "buttons"), &buttons)) return JS_FALSE;

	char buffer[256];
	snprintf(buffer, 256, "%d %d %d", x, y, buttons);
	JS_SET_RVAL(cx, vp, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, buffer)));
	return JS_TRUE;
}


// Initialise all the types at once:
void JSI_GUITypes::init()
{
	g_ScriptingHost.DefineCustomObjectType(&JSI_GUISize::JSI_class,  JSI_GUISize::construct,  1, JSI_GUISize::JSI_props,  JSI_GUISize::JSI_methods,  NULL, NULL);
	g_ScriptingHost.DefineCustomObjectType(&JSI_GUIColor::JSI_class, JSI_GUIColor::construct, 1, JSI_GUIColor::JSI_props, JSI_GUIColor::JSI_methods, NULL, NULL);
	g_ScriptingHost.DefineCustomObjectType(&JSI_GUIMouse::JSI_class, JSI_GUIMouse::construct, 1, JSI_GUIMouse::JSI_props, JSI_GUIMouse::JSI_methods, NULL, NULL);
}
