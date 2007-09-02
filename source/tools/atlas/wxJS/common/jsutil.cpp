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
 * $Id: jsutil.cpp 810 2007-07-13 20:07:05Z fbraem $
 */

#include <js/jsapi.h>
#include <wx/wx.h>

#include "jsutil.h"
#include "defs.h"

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
  if ( JS_GetProperty(cx, JS_GetGlobalObject(cx), "script", v) == JS_TRUE )
  {
    if ( JSVAL_IS_OBJECT(*v) )
    {
      JSObject *obj = JSVAL_TO_OBJECT(*v);
      return JS_GetProperty(cx, obj, "root", v) == JS_TRUE;
    }
  }
  return false;
}

JSBool wxjs::wxDefineProperty(JSContext *cx, 
                              JSObject *obj, 
                              const wxString &propertyName, 
                              jsval *propertyValue)
{
    wxCSConv conv(wxJS_INTERNAL_ENCODING);
    int length = propertyName.length();
    wxCharBuffer s = propertyName.mb_str(conv);
    
/*
    int jsLength = utf16.WC2MB(NULL, propertyName, 0);
    char *jsValue = new char[jsLength + conv.GetMBNulLen()];
    jsLength = conv.WC2MB(jsValue, propertyName, jsLength + conv.GetMBNulLen());
    JSBool ret = JS_DefineUCProperty(cx, obj, (jschar *) jsValue, jsLength / conv.GetMBNulLen(), 
                                     *propertyValue, NULL, NULL, JSPROP_ENUMERATE);
    delete[] jsValue;
    return ret;
*/
    return JS_DefineUCProperty(cx, obj, (const jschar *) s.data(), length, 
                               *propertyValue, NULL, NULL, JSPROP_ENUMERATE);
}

JSBool wxjs::wxGetProperty(JSContext *cx, 
                           JSObject *obj, 
                           const wxString &propertyName, 
                           jsval *propertyValue)
{
  wxCSConv conv(wxJS_INTERNAL_ENCODING);
  int length = propertyName.length();
  wxCharBuffer s = propertyName.mb_str(conv);

  return JS_GetUCProperty(cx, obj, 
                         (const jschar *) s.data(), length, propertyValue);
}

JSBool wxjs::wxSetProperty(JSContext *cx, 
                           JSObject *obj, 
                           const wxString &propertyName, 
                           jsval *propertyValue)
{
  wxCSConv conv(wxJS_INTERNAL_ENCODING);
  int length = propertyName.length();
  wxCharBuffer s = propertyName.mb_str(conv);

  return JS_SetUCProperty(cx, obj, 
                         (const jschar *) s.data(), length, propertyValue);
}
