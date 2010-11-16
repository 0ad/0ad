/* Copyright (C) 2010 Wildfire Games.
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
	{ "write", JSI_Console::writeConsole, 1, 0 },
	{ 0 },
};

JSBool JSI_Console::getProperty(JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsid id, jsval* vp)
{
	if (!JSID_IS_INT(id))
		return JS_TRUE;

	int i = JSID_TO_INT(id);

	switch (i)
	{
	case console_visible:
		*vp = BOOLEAN_TO_JSVAL(g_Console->IsActive());
		return JS_TRUE;
	default:
		*vp = JSVAL_NULL;
		return JS_TRUE;
	}
}

JSBool JSI_Console::setProperty(JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsid id, jsval* vp)
{
	if (!JSID_IS_INT(id))
		return JS_TRUE;

	int i = JSID_TO_INT(id);

	switch (i)
	{
	case console_visible:
		try
		{
			g_Console->SetVisible(ToPrimitive<bool> (*vp));
			return JS_TRUE;
		}
		catch (PSERROR_Scripting_ConversionFailed)
		{
			return JS_TRUE;
		}
	default:
		return JS_TRUE;
	}
}

void JSI_Console::init()
{
	g_ScriptingHost.DefineCustomObjectType(&JSI_class, NULL, 0, JSI_props, JSI_methods, NULL, NULL);
}

JSBool JSI_Console::getConsole(JSContext* cx, JSObject* UNUSED(obj), jsid UNUSED(id), jsval* vp)
{
	JSObject* console = JS_NewObject(cx, &JSI_Console::JSI_class, NULL, NULL);
	*vp = OBJECT_TO_JSVAL(console);
	return JS_TRUE;
}

JSBool JSI_Console::writeConsole(JSContext* cx, uintN argc, jsval* vp)
{
	UNUSED2(cx);

	CStrW output;
	for (uintN i = 0; i < argc; i++)
	{
		try
		{
			CStrW arg = g_ScriptingHost.ValueToUCString(JS_ARGV(cx, vp)[i]);
			output += arg;
		}
		catch (PSERROR_Scripting_ConversionFailed)
		{
		}
	}

	// TODO: What if the console has been destroyed already?
	if (g_Console)
		g_Console->InsertMessage(L"%ls", output.c_str());

	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}
