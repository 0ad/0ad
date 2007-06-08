#include "precompiled.h"

/*
 * wxJavaScript - statbar.cpp
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
 * $Id: statbar.cpp 708 2007-05-14 15:30:45Z fbraem $
 */
// wxJSStatusBar.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "statbar.h"
#include "window.h"



#include "../misc/rect.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/statbar</file>
 * <module>gui</module>
 * <class name="wxStatusBar" type="@wxWindow">
 *  A status bar is a narrow window that can be placed along the bottom of a frame 
 *  to give small amounts of status information. It can contain one or more fields, 
 *  one or more of which can be variable length according to the size of the window
 *  <br /><br />The example shows a frame with a statusbar that contains a wxBitmapButton. 
 *  When you click the button the bitmap is changed.
 *  <pre><code class="whjs">
 *   wxTheApp.onInit = init;
 *   wxTheApp.mainLoop();
 *   
 *   function init()
 *   {
 *     wxImage.addHandler(new wxGIFHandler());
 *   
 *     var frame = new wxFrame(null, -1, "A Statusbar Example");
 *   
 *     var statusbar = frame.createStatusBar(2);
 *     statusbar.bitmapOn = new wxBitmap("green.gif", wxBitmapType.GIF);
 *     statusbar.bitmapOff = new wxBitmap("red.gif", wxBitmapType.GIF);
 *     statusbar.sw = true;
 *   
 *     statusbar.button = new wxBitmapButton(statusbar, 1, statusbar.bitmapOn);
 *     statusbar.button.onClicked = switched;
 *   
 *     frame.visible = true;
 *     topWindow = frame;
 *   
 *     return true;
 *   }
 *   
 *   function switched()
 *   {
 *     sw = ! sw;
 *     if ( sw )
 *        bitmapLabel = bitmapOn;
 *     else
 *        bitmapLabel = bitmapOff;
 *   
 *     refresh();
 *   }
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(StatusBar, "wxStatusBar", 2)

/***
 * <properties>
 *  <property name="fieldsCount" type="Integer">
 *   Get/Set the number of fields in the statusbar.
 *   See also @wxStatusBar#setFieldsCount
 *  </property>
 *  <property name="statusWidths" type="Array">
 *   Get/Set the width for each field. At least one element must be -1 (meaning variable width).
 *   See also @wxFrame#setStatusWidths
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(StatusBar)
    WXJS_PROPERTY(P_FIELDS_COUNT, "fieldsCount")
    WXJS_PROPERTY(P_STATUS_WIDTHS, "statusWidths")
WXJS_END_PROPERTY_MAP()

bool StatusBar::GetProperty(wxStatusBar *p,
                            JSContext *cx,
                            JSObject* WXUNUSED(obj),
                            int id,
                            jsval *vp)
{
    switch (id)
    {
    case P_FIELDS_COUNT:
        *vp = ToJS(cx, p->GetFieldsCount());
        break;
    case P_STATUS_WIDTHS:
        {
            int count = p->GetFieldsCount();
            if ( count == 0 )
                *vp = JSVAL_VOID;
            else
            {
                JSObject *objArr = JS_NewArrayObject(cx, count, NULL);
                *vp = OBJECT_TO_JSVAL(objArr);
                for(jsint i = 0; i < count; i++)
                {
                    wxRect rect;
                    p->GetFieldRect(i, rect);
                    jsval element = INT_TO_JSVAL(rect.GetWidth());
                    JS_SetElement(cx, objArr, i, &element);
                }
            }
            break;
        }
    }
    return true;
}

bool StatusBar::SetProperty(wxStatusBar *p,
                            JSContext *cx,
                            JSObject* WXUNUSED(obj),
                            int id,
                            jsval *vp)
{
    switch (id)
    {
    case P_FIELDS_COUNT:
        {
            int count;
            if ( FromJS(cx, *vp, count) )
                p->SetFieldsCount(count);
            break;
        }
    case P_STATUS_WIDTHS:
        {
            if ( JSVAL_IS_OBJECT(*vp) )
            {
                JSObject *arrObj = JSVAL_TO_OBJECT(*vp);
                if ( JS_IsArrayObject(cx, arrObj) == JS_TRUE )
                {
			        jsuint length = 0;
			        JS_GetArrayLength(cx, arrObj, &length);
                    uint fields = p->GetFieldsCount();
                    if ( length > fields )
                        length = fields;
			        int *widths = new int[length];
			        for(jsuint i = 0; i < length; i++)
			        {
				        jsval element;
				        JS_GetElement(cx, arrObj, i, &element);
				        if ( ! FromJS(cx, element, widths[i]) )
                        {
                            delete[] widths;
                            return JS_FALSE;
                        }
			        }
                    p->SetStatusWidths(length, widths);
                    delete[] widths;
                }
                break;
            }
        }
    }
    return true;
}

/***
 * <constants>
 *  <type name="Styles">
 *   <constant name="SIZEGRIP" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(StatusBar)
    WXJS_CONSTANT(wxST_, SIZEGRIP)
WXJS_END_CONSTANT_MAP()

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxStatusBar.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxStatusBar.SIZEGRIP">
 *    The wxStatusBar style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStatusBar object.
 *  </desc>
 * </ctor>
 */
wxStatusBar* StatusBar::Construct(JSContext *cx,
                                  JSObject *obj,
                                  uintN argc,
                                  jsval *argv,
                                  bool constructing)
{
  wxStatusBar *p = new wxStatusBar();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    create(cx, obj, argc, argv, &rval);
  }
  return p;
}

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxStatusBar.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxStatusBar.SIZEGRIP">
 *    The wxStatusBar style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStatusBar object.
 *  </desc>
 * </method>
 */
WXJS_BEGIN_METHOD_MAP(StatusBar)
  WXJS_METHOD("create", create, 2)
  WXJS_METHOD("getFieldRect", getFieldRect, 2)
  WXJS_METHOD("getStatusText", getStatusText, 0)
  WXJS_METHOD("setStatusText", setStatusText, 1)
  WXJS_METHOD("setFieldsCount", setFieldsCount, 1)
WXJS_END_METHOD_MAP()

JSBool StatusBar::create(JSContext *cx,
                         JSObject *obj,
                         uintN argc,
                         jsval *argv,
                         jsval *rval)
{
  wxStatusBar *p = GetPrivate(cx, obj);
  *rval = JSVAL_TRUE;

  if ( argc > 3 )
      argc = 3;

  int style = wxST_SIZEGRIP;
  switch(argc)
  {
  case 3:
    if ( ! FromJS(cx, argv[2], style) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "Integer");
      return JS_FALSE;
    }
    // Fall through
  default:
    int id;
    if ( ! FromJS(cx, argv[1], id) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
      return JS_FALSE;
    }

    wxWindow *parent = Window::GetPrivate(cx, argv[0]);
	if ( parent == NULL )
    {
        JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
        return JS_FALSE;
    }

    JavaScriptClientData *clntParent =
          dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
    if ( clntParent == NULL )
    {
        JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
        return JS_FALSE;
    }
    JS_SetParent(cx, obj, clntParent->GetObject());

    if ( p->Create(parent, id, style) )
    {
      *rval = JSVAL_TRUE;
      p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
    }

    return JS_TRUE;
  }
}

/***
 * <method name="getFieldRect">
 *  <function returns="Boolean">
 *   <arg name="Field" type="Integer">
 *    The field of the statusbar.
 *   </arg>
 *   <arg name="Rect" type="@wxRect">
 *    The object that will get the rectangle information.
 *   </arg>
 *  </function>
 *  <desc>
 *   Puts the size and position of a fields internal 
 *   bounding rectangle into the <i>Rect</i> object. Returns false on failure, 
 *   true on success.
 *   <i>Rect</i> is only changed on success.
 *  </desc>
 * </method>
 */
JSBool StatusBar::getFieldRect(JSContext *cx,
                               JSObject *obj,
                               uintN argc,
                               jsval *argv,
                               jsval *rval)
{
    wxStatusBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int field;
    if ( ! FromJS(cx, argv[0], field) )
        return JS_FALSE;

    wxRect *rect = Rect::GetPrivate(cx, argv[1]);
    if ( rect == NULL )
        return JS_FALSE;

    wxRect result;
    if ( p->GetFieldRect(field, result) )
    {
        *rect = result;
        *rval = JSVAL_TRUE;
    }
    else
        *rval = JSVAL_FALSE;

    return JS_TRUE;
}

/***
 * <method name="getStatusText">
 *  <function returns="String">
 *   <arg name="Field" type="Integer" default="0"> 
 *    The field of the statusbar.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets the status text of the given field
 *  </desc>
 * </method>
 */
JSBool StatusBar::getStatusText(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                jsval *rval)
{
    wxStatusBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int field = 0;
    if ( ! FromJS(cx, argv[0], field) )
        return JS_FALSE;

    *rval = ToJS(cx, p->GetStatusText(field));
    return JS_TRUE;
}

/***
 * <method name="setFieldsCount">
 *  <function returns="Boolean">
 *   <arg name="Count" type="Integer">
 *    The number of fields.
 *   </arg>
 *   <arg name="Widths" type="Array" default="null" />
 *  </function>
 *  <desc>
 *   Sets the number of fields, and optionally the field widths.
 *  </desc>
 * </method>
 */
JSBool StatusBar::setFieldsCount(JSContext *cx,
                                 JSObject *obj,
                                 uintN argc,
                                 jsval *argv,
                                 jsval *rval)
{
    wxStatusBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int count;
    if ( ! FromJS(cx, argv[0], count) )
        return JS_FALSE;

    int *widths = NULL;
    if ( JSVAL_IS_OBJECT(argv[0]) )
    {
        JSObject *obj = JSVAL_TO_OBJECT(argv[0]);
        if ( JS_IsArrayObject(cx, obj) == JS_TRUE )
        {
			jsuint length = 0;
			JS_GetArrayLength(cx, obj, &length);
            uint fields = p->GetFieldsCount();
            if ( length > fields )
                length = fields;
			
            widths = new int[length];
			for(jsuint i =0; i < length; i++)
			{
				jsval element;
				JS_GetElement(cx, obj, i, &element);
				if ( ! FromJS(cx, element, widths[i]) )
                {
                    delete[] widths;
                    return JS_FALSE;
                }
			}
        }
	}

    p->SetFieldsCount(count, widths);
    delete[] widths;

    return JS_TRUE;
}


/***
 * <method name="setStatusText">
 *  <function>
 *   <arg name="Text" type="String">
 *    The new text for the field.
 *   </arg>
 *   <arg name="Field" type="Integer" default="0">
 *    The field to change. Default is 0.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the text of the field.
 *  </desc>
 * </method>
 */
JSBool StatusBar::setStatusText(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                jsval *rval)
{
    wxStatusBar *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int field = 0;
    if (    argc > 1 
         && ! FromJS(cx, argv[0], field) )
        return JS_FALSE;

    wxString text;
    FromJS(cx, argv[0], text);
    p->SetStatusText(text, field);

    return JS_TRUE;
}
