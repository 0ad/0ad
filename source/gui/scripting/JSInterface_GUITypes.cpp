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
	JSAutoRequest rq(cx);
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUISize"));

	if (args.length() == 8)
	{
		JS_SetProperty(cx, obj, "left",		&args[0]);
		JS_SetProperty(cx, obj, "top",		&args[1]);
		JS_SetProperty(cx, obj, "right",	&args[2]);
		JS_SetProperty(cx, obj, "bottom",	&args[3]);
		JS_SetProperty(cx, obj, "rleft",	&args[4]);
		JS_SetProperty(cx, obj, "rtop",		&args[5]);
		JS_SetProperty(cx, obj, "rright",	&args[6]);
		JS_SetProperty(cx, obj, "rbottom",	&args[7]);
	}
	else if (args.length() == 4)
	{
		JS::RootedValue zero(cx, JSVAL_ZERO);
		JS_SetProperty(cx, obj, "left",		&args[0]);
		JS_SetProperty(cx, obj, "top",		&args[1]);
		JS_SetProperty(cx, obj, "right",	&args[2]);
		JS_SetProperty(cx, obj, "bottom",	&args[3]);
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

	args.rval().setObject(*obj);
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
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	CStr buffer;

	try
	{
		ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
		double val, valr;
#define SIDE(side) \
		pScriptInterface->GetProperty(rec.thisv(), #side, val); \
		pScriptInterface->GetProperty(rec.thisv(), "r"#side, valr); \
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
		rec.rval().setString(JS_NewStringCopyZ(cx, "<Error converting value to numbers>"));
		return JS_TRUE;
	}

	rec.rval().setString(JS_NewStringCopyZ(cx, buffer.c_str()));
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
	JSAutoRequest rq(cx);
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUIColor"));
	
	if (args.length() == 4)
	{
		JS_SetProperty(cx, obj, "r", &args[0]);
		JS_SetProperty(cx, obj, "g", &args[1]);
		JS_SetProperty(cx, obj, "b", &args[2]);
		JS_SetProperty(cx, obj, "a", &args[3]);
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

	args.rval().setObject(*obj);
	return JS_TRUE;
}

JSBool JSI_GUIColor::toString(JSContext* cx, uint argc, jsval* vp)
{
	UNUSED2(argc);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	double r, g, b, a;
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	pScriptInterface->GetProperty(rec.thisv(), "r", r);
	pScriptInterface->GetProperty(rec.thisv(), "g", g);
	pScriptInterface->GetProperty(rec.thisv(), "b", b);
	pScriptInterface->GetProperty(rec.thisv(), "a", a);
	char buffer[256];
	// Convert to integers, to be compatible with the GUI's string SetSetting
	snprintf(buffer, 256, "%d %d %d %d",
		(int)(255.0 * r),
		(int)(255.0 * g),
		(int)(255.0 * b),
		(int)(255.0 * a));
	rec.rval().setString(JS_NewStringCopyZ(cx, buffer));
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
	JSAutoRequest rq(cx);
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUIMouse"));

	if (args.length() == 3)
	{
		JS_SetProperty(cx, obj, "x", &args[0]);
		JS_SetProperty(cx, obj, "y", &args[1]);
		JS_SetProperty(cx, obj, "buttons", &args[2]);
	}
	else
	{
		JS::RootedValue zero (cx, JS::NumberValue(0));
		JS_SetProperty(cx, obj, "x", zero.address());
		JS_SetProperty(cx, obj, "y", zero.address());
		JS_SetProperty(cx, obj, "buttons", zero.address());
	}

	args.rval().setObject(*obj);
	return JS_TRUE;
}

JSBool JSI_GUIMouse::toString(JSContext* cx, uint argc, jsval* vp)
{
	UNUSED2(argc);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	i32 x, y, buttons;
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	pScriptInterface->GetProperty(rec.thisv(), "x", x);
	pScriptInterface->GetProperty(rec.thisv(), "y", y);
	pScriptInterface->GetProperty(rec.thisv(), "buttons", buttons);

	char buffer[256];
	snprintf(buffer, 256, "%d %d %d", x, y, buttons);
	rec.rval().setString(JS_NewStringCopyZ(cx, buffer));
	return JS_TRUE;
}


// Initialise all the types at once:
void JSI_GUITypes::init(ScriptInterface& scriptInterface)
{
	scriptInterface.DefineCustomObjectType(&JSI_GUISize::JSI_class,  JSI_GUISize::construct,  1, JSI_GUISize::JSI_props,  JSI_GUISize::JSI_methods,  NULL, NULL);
	scriptInterface.DefineCustomObjectType(&JSI_GUIColor::JSI_class, JSI_GUIColor::construct, 1, JSI_GUIColor::JSI_props, JSI_GUIColor::JSI_methods, NULL, NULL);
	scriptInterface.DefineCustomObjectType(&JSI_GUIMouse::JSI_class, JSI_GUIMouse::construct, 1, JSI_GUIMouse::JSI_props, JSI_GUIMouse::JSI_methods, NULL, NULL);
}
