#include "precompiled.h"

/*
 * wxJavaScript - fontenum.cpp
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
 * $Id: fontenum.cpp 715 2007-05-18 20:38:04Z fbraem $
 */
// fontenum.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/jsutil.h"

#include "fontenum.h"
#include "app.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/fontenum</file>
 * <module>gui</module>
 * <class name="wxFontEnumerator">
 *  Use wxFontEnumerator to enumerate all available fonts.
 *  See @wxFont, @wxFontEncoding
 *  <br /><br />
 *  The following example shows a dialog which contains a listbox with the names of all the available fonts.
 *  <pre><code class="whjs">
 *   var dlg = new wxDialog(null,
 *                          -1,
 *                         "Font Enumerating example");
 *
 *   // Create a wxFontEnumerator
 *   var enumerator = new wxFontEnumerator();
 *   // Set a funtion to onFacename. To get all available fonts return true. 
 *   // No arguments need to be specified, because they are not used.
 *
 *   enumerator.onFacename = function()
 *   {
 *     return true;
 *   };
 *
 *   // Use enumerateFaceNames to start the enumeration.
 *   enumerator.enumerateFacenames();
 *
 *   // Now the property facenames contains all the font names.
 *   var listbox = new wxListBox(dlg, -1, wxDefaultPosition, wxDefaultSize, enumerator.facenames);
 *  </code></pre>
 * </class>
 */

WXJS_INIT_CLASS(FontEnumerator, "wxFontEnumerator", 0)

/***
 * <class_properties>
 *  <property name="encodings" type="Array" readonly="Y">
 *   Array of strings containing all encodings found by @wxFontEnumerator#enumerateEncodings. 
 *   Returns undefined when this function is not called.
 *  </property>
 *  <property name="facenames" type="Array" readonly="Y">
 *   Array of strings containing all facenames found by @wxFontEnumerator#enumerateFacenames.
 *   Returns undefined when this function is not called.
 *  </property>
 *  <property name="onFontEncoding" type="Function">
 *   A function which receives the encoding as argument. The encoding is added
 *   to @wxFontEnumerator#encodings when the function returns true.
 *  </property>
 *  <property name="onFacename" type="Function">
 *   A function which receives the font and encoding as arguments.
 *   When the function returns true, the font is added to @wxFontEnumerator#facenames.
 *   When no function is set, all available fonts are added.
 *  </property>
 * </class_properties>
 */
WXJS_BEGIN_STATIC_PROPERTY_MAP(FontEnumerator)
  WXJS_READONLY_STATIC_PROPERTY(P_FACENAMES, "facenames")
  WXJS_READONLY_STATIC_PROPERTY(P_ENCODINGS, "encodings")
WXJS_END_PROPERTY_MAP()

FontEnumerator::FontEnumerator(JSContext *cx, JSObject *obj)
{
  m_data = new JavaScriptClientData(cx, obj, false);
}

FontEnumerator::~FontEnumerator()
{
  delete m_data;
}

bool FontEnumerator::OnFacename(const wxString& faceName)
{
	JSObject *enumObj = m_data->GetObject();

	jsval fval;
    JSContext *cx = m_data->GetContext();
	if ( GetFunctionProperty(cx, enumObj, "onFacename", &fval) == JS_TRUE )
	{
		jsval rval;
		jsval argv[] = { ToJS(cx, faceName) };
		JSBool result = JS_CallFunctionValue(cx, enumObj, fval, 1, argv, &rval);
		if ( result == JS_FALSE )
		{
            if ( JS_IsExceptionPending(cx) )
            {
                JS_ReportPendingException(cx);
            }
			return false;
		}
		else
		{
			bool ret;
			if (    ! FromJS(cx, rval, ret)
                 || ! ret                   )
                return false;
		}
	}
	
	return wxFontEnumerator::OnFacename(faceName);
}

bool FontEnumerator::OnFontencoding(const wxString& faceName, const wxString &encoding)
{
	JSObject *enumObj = m_data->GetObject();
    JSContext *cx = m_data->GetContext();

	jsval fval;
	if ( GetFunctionProperty(cx, enumObj, "onFontEncoding", &fval) == JS_TRUE )
	{
		jsval rval;
		jsval argv[] = {  ToJS(cx, faceName)
						, ToJS(cx, encoding) };
		JSBool result = JS_CallFunctionValue(cx, enumObj, fval, 2, argv, &rval);
		if ( result == JS_FALSE )
		{
            if ( JS_IsExceptionPending(cx) )
            {
                JS_ReportPendingException(cx);
            }
			return false;
		}
		else
		{
			bool ret;
			if (    ! FromJS(cx, rval, ret)
                 || ! ret                   )
                return false;
		}
	}
	
	return wxFontEnumerator::OnFontEncoding(faceName, encoding);
}

bool FontEnumerator::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
    switch (id) 
	{
	case P_FACENAMES:
		{
            wxArrayString faceNames = wxFontEnumerator::GetFacenames();
            *vp = ( faceNames == NULL ) ? JSVAL_VOID : ToJS(cx, faceNames);
		}
		break;
	case P_ENCODINGS:
		{
            wxArrayString encodings = wxFontEnumerator::GetEncodings();
            *vp = ( encodings == NULL ) ? JSVAL_VOID : ToJS(cx, encodings);
        }
		break;
    }
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Creates a wxFontEnumerator object.
 *  </desc>
 * </ctor>
 */
FontEnumerator* FontEnumerator::Construct(JSContext *cx,
                                          JSObject *obj,
                                          uintN argc,
                                          jsval *argv,
                                          bool constructing)
{
	return new FontEnumerator(cx, obj);
}

WXJS_BEGIN_METHOD_MAP(FontEnumerator)
    WXJS_METHOD("enumerateFacenames", enumerateFacenames, 2)
    WXJS_METHOD("enumerateEncodings", enumerateEncodings, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="enumerateFaceNames">
 *  <function returns="Boolean">
 *	 <arg name="FontEncoding" type="Integer" default="wxFontEncoding.SYSTEM">
 *    The encoding.
 *   </arg>
 *   <arg name="FixedWidthOnly" type="Boolean" default="false">
 *    Only report fonts with fixed width or not.
 *   </arg>
 *  </function>
 *  <desc>
 *   This will call the function which is set to the @wxFontEnumerator#onFacename property
 *   for each font which supports the given encoding. When this function returns true for the
 *   the given font, it will be added to @wxFontEnumerator#facenames.
 *   When no function is specified, all fonts will be added to @wxFontEnumerator#facenames.
 *   See also @wxFontEncoding, @wxFontEnumerator#facenames, @wxFontEnumerator#onFacename
 *  </desc>
 * </method>
 */
JSBool FontEnumerator::enumerateFacenames(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    FontEnumerator *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data for wxFontEnumerator"));

	if ( argc == 0 )
	{
		p->EnumerateFacenames();
		return JS_TRUE;
	}

	int encoding;
	if ( FromJS(cx, argv[0], encoding) )
	{
		if ( argc > 1 )
		{
			bool fixedWidth;
			if ( FromJS(cx, argv[1], fixedWidth) )
			{
				p->EnumerateFacenames((wxFontEncoding)encoding, fixedWidth);
				return JS_TRUE;
			}
		}
		else
		{
			p->EnumerateFacenames((wxFontEncoding)encoding);
			return JS_TRUE;
		}
	}
	
	return JS_FALSE;
}

/***
 * <method name="enumerateEncodings">
 *	<function returns="Boolean">
 *	 <arg name="Font" type="String" default="">
 *    A font name.
 *   </arg>
 *  </function>
 *  <desc>
 *	 The function set on the @wxFontEnumerator#onFontEncoding is called for each
 *   encoding supported by the given font.
 *   When no function is set all encodings are reported.
 *   See @wxFontEnumerator#onFontEncoding, @wxFontEnumerator#encodings
 *  </desc>
 * </method>
 */
JSBool FontEnumerator::enumerateEncodings(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    FontEnumerator *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data for wxFontEnumerator"));

	if ( argc == 0 )
	{
		p->EnumerateEncodings();
		return JS_TRUE;
	}

	wxString font;
	FromJS(cx, argv[0], font);
	p->EnumerateEncodings(font);
	
	return JS_TRUE;
}
