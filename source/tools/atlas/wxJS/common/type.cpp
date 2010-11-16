#include "precompiled.h"

/*
 * wxJavaScript - type.cpp
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
 * $Id: type.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "type.h"

//template<class T>
//bool FromJS(JSContext *cx, jsval v, T& to);

template<>
bool wxjs::FromJS<int>(JSContext *cx, jsval v, int &to)
{
	int32 temp;
	if ( JS_ValueToInt32(cx, v, &temp) == JS_TRUE )
	{
		to = temp;
		return true;
	}
	else
		return false;
}

template<>
bool wxjs::FromJS<unsigned int>(JSContext *cx, jsval v, unsigned int &to)
{
	int32 temp;
	if ( JS_ValueToInt32(cx, v, &temp) == JS_TRUE )
	{
		to = temp;
		return true;
	}
	else
		return false;
}


template<>
bool wxjs::FromJS<long>(JSContext *cx, jsval v, long &to)
{
	int32 temp;
	if ( JS_ValueToInt32(cx, v, &temp) )
	{
		to = temp;
		return JS_TRUE;
	}
	return JS_FALSE;
}

template<>
bool wxjs::FromJS<unsigned long>(JSContext *cx, jsval v, unsigned long &to)
{
	int32 temp;
	if ( JS_ValueToInt32(cx, v, &temp) == JS_TRUE )
	{
		to = temp;
		return true;
	}
	else
		return false;
}

template<>
bool wxjs::FromJS<double>(JSContext *cx, jsval v, double &to)
{
    jsdouble d;
    if ( JS_ValueToNumber(cx, v, &d) == JS_TRUE )
    {
        to = d;
        return true;
    }
    else
        return false;
}

template<>
bool wxjs::FromJS<bool>(JSContext *cx, jsval v, bool &to)
{
	JSBool b;
	if ( JS_ValueToBoolean(cx, v, &b) == JS_TRUE )
	{
		to = (b == JS_TRUE);
		return true;
	}
	else
		return false;
}

template<>
bool wxjs::FromJS<wxString>(JSContext *cx, jsval v, wxString &to)
{
	wxMBConvUTF16 conv;
    JSString *str = JS_ValueToString(cx, v);
	jschar *s = JS_GetStringChars(str);
	to = wxString(conv.cMB2WX((char *) s));
    return true;
}

template<>
bool wxjs::FromJS<wxDateTime>(JSContext *cx, jsval v, wxDateTime& to)
{
    to.SetToCurrent(); // To avoid invalid date asserts.

    JSObject *obj = JSVAL_TO_OBJECT(v);
    if ( js_DateIsValid(cx, obj) )
    {
	    to.SetYear(js_DateGetYear(cx, obj));
	    to.SetMonth((wxDateTime::Month) js_DateGetMonth(cx, obj));
	    to.SetDay((unsigned short) js_DateGetDate(cx, obj));
	    to.SetHour((unsigned short) js_DateGetHours(cx, obj));
	    to.SetMinute((unsigned short) js_DateGetMinutes(cx, obj));
	    to.SetSecond((unsigned short) js_DateGetSeconds(cx, obj));
	    return true;
    }
    else
	    return false;
}

template<>
bool wxjs::FromJS<wxjs::StringsPtr>(JSContext*cx, jsval v, StringsPtr &to)
{
    if ( JSVAL_IS_OBJECT(v) )
    {
	    JSObject *objItems = JSVAL_TO_OBJECT(v);
	    if (    objItems != (JSObject *) NULL
		     && JS_IsArrayObject(cx, objItems) )
	    {
		    jsuint length = 0;
		    JS_GetArrayLength(cx, objItems, &length);
		    to.Allocate(length);
		    for(jsuint i =0; i < length; i++)
		    {
			    jsval element;
			    JS_GetElement(cx, objItems, i, &element);
                wxjs::FromJS(cx, element, to[i]);
		    }
	    }
	    return true;
    }
    else
	    return false;
}

template<>
bool wxjs::FromJS<wxArrayString>(JSContext *cx, jsval v, wxArrayString &to)
{
    if ( JSVAL_IS_OBJECT(v) )
    {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        if (    obj != NULL
             && JS_IsArrayObject(cx, obj) )
        {
		    jsuint length = 0;
		    JS_GetArrayLength(cx, obj, &length);
		    for(jsuint i =0; i < length; i++)
		    {
			    jsval element;
			    JS_GetElement(cx, obj, i, &element);
                wxString stringElement;
			    if ( FromJS(cx, element, stringElement) )
                    to.Add(stringElement);
		    }
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

template<>
bool wxjs::FromJS<wxStringList>(JSContext *cx, jsval v, wxStringList &to)
{
    if ( JSVAL_IS_OBJECT(v) )
    {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        if (    obj != NULL
             && JS_IsArrayObject(cx, obj) )
        {
		    jsuint length = 0;
		    JS_GetArrayLength(cx, obj, &length);
		    for(jsuint i =0; i < length; i++)
		    {
			    jsval element;
			    JS_GetElement(cx, obj, i, &element);
                wxString stringElement;
			    if ( FromJS(cx, element, stringElement) )
                    to.Add(stringElement);
		    }
            return true;
        }
        else
            return false;
    }
    else
        return false;
}

template<>
bool wxjs::FromJS<long long>(JSContext *cx, jsval v, long long &to)
{
    int32 temp;
    if ( JS_ValueToInt32(cx, v, &temp) == JS_TRUE )
    {
        to = temp;
        return true;
    }
    else
        return false;
}

//template<class T>
//jsval ToJS(JSContext *cx, T wx);

template<>
jsval wxjs::ToJS<int>(JSContext* WXUNUSED(cx), const int &from)
{
	return INT_TO_JSVAL(from);
}

template<>
jsval wxjs::ToJS<unsigned int>(JSContext* WXUNUSED(cx), const unsigned int&from)
{
    return INT_TO_JSVAL(from);
}

template<>
jsval wxjs::ToJS<long>(JSContext* WXUNUSED(cx), const long &from)
{
	return INT_TO_JSVAL(from);
}

template<>
jsval wxjs::ToJS<unsigned long>(JSContext* cx, const unsigned long&from)
{
    jsval v;
    JS_NewNumberValue(cx, from, &v);
	return v;
}

template<>
jsval wxjs::ToJS<float>(JSContext* cx, const float &from)
{
    jsval v;
    JS_NewNumberValue(cx, from, &v);
	return v;
}

template<>
jsval wxjs::ToJS<double>(JSContext* cx, const double &from)
{
    jsval v;
    JS_NewNumberValue(cx, from, &v);
	return v;
}

template<>
jsval wxjs::ToJS<bool>(JSContext* WXUNUSED(cx), const bool &from)
{
	return BOOLEAN_TO_JSVAL(from);
}

template<>
jsval wxjs::ToJS<wxString>(JSContext* cx, const wxString &from)
{
  int length = from.length();
    if ( length == 0 )
    {
        return STRING_TO_JSVAL(JS_NewUCStringCopyN(cx, (jschar *) "", 0));
    }
	jsval val = JSVAL_VOID;
	
	wxMBConvUTF16 utf16;
    wxCharBuffer buffer = from.mb_str(utf16);
    val = STRING_TO_JSVAL(JS_NewUCStringCopyN(cx, (jschar *) buffer.data(), length));
    return val;
}

template<>
jsval wxjs::ToJS<wxDateTime>(JSContext *cx, const wxDateTime &from)
{
    if ( from.IsValid() )
    {
        return OBJECT_TO_JSVAL(js_NewDateObject(cx, 
	    									    from.GetYear(), 
		    								    from.GetMonth(),
			    							    from.GetDay(),
				    				            from.GetHour(),
					    					    from.GetMinute(),
						    				    from.GetSecond()));
    }
    else
        return JSVAL_VOID;
}

template<>
jsval wxjs::ToJS<wxArrayString>(JSContext *cx, const wxArrayString &from)
{
    JSObject *objArray = JS_NewArrayObject(cx, 0, NULL);
    for(size_t i = 0; i < from.GetCount(); i++)
    {
	    jsval element = ToJS(cx, from.Item(i));
	    JS_SetElement(cx, objArray, i, &element);
    }
    return OBJECT_TO_JSVAL(objArray);
}

template<>
jsval wxjs::ToJS<wxStringList>(JSContext *cx, const wxStringList &from)
{
    JSObject *objArray = JS_NewArrayObject(cx, 0, NULL);

    int i = 0;
    wxStringListNode *node = from.GetFirst();
    while(node)
    {
        wxString s(node->GetData()); 
	    
        jsval element = ToJS(cx, s);
	    JS_SetElement(cx, objArray, i++, &element);

        node = node->GetNext();
    }
    return OBJECT_TO_JSVAL(objArray);
}
