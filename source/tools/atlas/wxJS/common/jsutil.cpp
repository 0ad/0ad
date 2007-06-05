#include "precompiled.h"

/*
 * wxJavaScript - jsutil.cpp
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: jsutil.cpp 603 2007-03-08 20:36:17Z fbraem $
 */

#include <js/jsapi.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "jsutil.h"

JSBool wxjs::GetFunctionProperty(JSContext *cx, JSObject *obj, const char *propertyName, jsval *property)
{
	if (    JS_GetProperty(cx, obj, propertyName, property) == JS_TRUE 
		 && JS_TypeOfValue(cx, *property) == JSTYPE_FUNCTION ) 
	{
		return JS_TRUE;
	}
	else
	{
		return JS_FALSE;
	}
}

JSBool wxjs::CallFunctionProperty(JSContext *cx, JSObject *obj, const char *propertyName, uintN argc, jsval* args, jsval *rval)
{
	jsval property;
    if ( ( GetFunctionProperty(cx, obj, propertyName, &property) == JS_TRUE ) )
	{
		if ( JS_CallFunctionValue(cx, obj, property, argc, args, rval) == JS_FALSE )
        {
            if ( JS_IsExceptionPending(cx) )
            {
                JS_ReportPendingException(cx);
            }
            return JS_FALSE;
        }
        return JS_TRUE;
	}
	return JS_FALSE;
}

JSClass* wxjs::GetClass(JSContext *cx, const char* className)
{
	jsval ctor, proto;

	if (JS_LookupProperty(cx, JS_GetGlobalObject(cx), className, &ctor) == JS_FALSE)
		return NULL;

	if (JS_LookupProperty(cx, JSVAL_TO_OBJECT(ctor), "prototype", &proto) == JS_FALSE)
		return NULL;

	JSObject *protoObj = JSVAL_TO_OBJECT(proto);
	
	return JS_GET_CLASS(cx, protoObj);
}

bool wxjs::HasPrototype(JSContext *cx, JSObject *obj, const char *className)
{
    JSClass *jsclass = GetClass(cx, className);
    if ( jsclass == NULL )
        return false;

	JSObject *prototype = JS_GetPrototype(cx, obj);
	while(    prototype != NULL
		   && JS_InstanceOf(cx, prototype, jsclass, NULL) == JS_FALSE )
	{
		prototype = JS_GetPrototype(cx, prototype);
	}
	return prototype != NULL;
}

bool wxjs::HasPrototype(JSContext *cx, jsval v, const char *className)
{
    if ( JSVAL_IS_OBJECT(v) )
        return HasPrototype(cx, JSVAL_TO_OBJECT(v), className);
    else
        return false;
}

bool wxjs::GetScriptRoot(JSContext *cx, jsval *v)
{
    return JS_GetProperty(cx, JS_GetGlobalObject(cx), "scriptRoot", v) == JS_TRUE;
}

JSBool wxjs::DefineUnicodeProperty(JSContext *cx, 
                                   JSObject *obj, 
                                   const wxString &propertyName, 
                                   jsval *propertyValue)
{
    wxMBConvUTF16 utf16;
    int jsLength = utf16.WC2MB(NULL, propertyName, 0);
    char *jsValue = new char[jsLength + utf16.GetMBNulLen()];
    jsLength = utf16.WC2MB(jsValue, propertyName, jsLength + utf16.GetMBNulLen());
    JSBool ret = JS_DefineUCProperty(cx, obj, (jschar *) jsValue, jsLength / utf16.GetMBNulLen(), 
                                     *propertyValue, NULL, NULL, JSPROP_ENUMERATE);
    delete[] jsValue;
    return ret;
}
