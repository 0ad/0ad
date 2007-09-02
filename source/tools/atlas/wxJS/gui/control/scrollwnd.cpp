#include "precompiled.h"

/*
 * wxJavaScript - scrollwnd.cpp
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
 * $Id: scrollwnd.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
#include <wx/wx.h>

/***
 * <file>control/scrolwin</file>
 * <module>gui</module>
 * <class name="wxScrolledWindow" prototype="@wxPanel">
 *  The wxScrolledWindow class manages scrolling for its client area, transforming 
 *  the coordinates according to the scrollbar positions, and setting the scroll positions, 
 *  thumb sizes and ranges according to the area in view.
 * </class>
 */

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "../misc/size.h"
#include "scrollwnd.h"
#include "panel.h"
#include "window.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(ScrolledWindow, "wxScrolledWindow", 1)

/***
 * <properties>
 *  <property name="retained" type="Boolean" readonly="Y">
 *   Motif only: true if the window has a backing bitmap
 *  </property>
 *  <property name="scrollPixelsPerUnit" type="Array" readonly="Y">
 *   Get the number of pixels per scroll unit (line), in each direction, as set 
 *   by @wxScrolledWindow#setScrollbars. A value of zero indicates no scrolling 
 *   in that direction.
 *  </property>
 *  <property name="viewStart" type="Array" readonly="Y">
 *   Get the position at which the visible portion of the window starts.
 *  </property>
 *  <property name="virtualSize" type="Array" readonly="Y">
 *   Gets the size in device units of the scrollable window area 
 *   (as opposed to the client size, which is the area of the window currently
 *   visible)
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ScrolledWindow)
    WXJS_READONLY_PROPERTY(P_RETAINED, "retained")
    WXJS_READONLY_PROPERTY(P_SCROLL_PIXELS_PER_UNIT, "scrollPixelsPerUnit")
    WXJS_READONLY_PROPERTY(P_VIEW_START, "viewStart")
    WXJS_READONLY_PROPERTY(P_VIRTUAL_SIZE, "virtualSize")
WXJS_END_PROPERTY_MAP()

bool ScrolledWindow::GetProperty(wxScrolledWindow *p,
                                 JSContext *cx,
                                 JSObject* WXUNUSED(obj),
                                 int id,
                                 jsval *vp)
{
    switch (id) 
	{
    case P_RETAINED:
        *vp = ToJS(cx, p->IsRetained());
        break;
    case P_SCROLL_PIXELS_PER_UNIT:
        {
            int x  = 0;
            int y  = 0;

            p->GetScrollPixelsPerUnit(&x, &y);

            JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
            *vp = OBJECT_TO_JSVAL(objArr);
            jsval element = ToJS(cx, x);
            JS_SetElement(cx, objArr, 0, &element);
            element = ToJS(cx, y);
            JS_SetElement(cx, objArr, 1, &element);
        }
    case P_VIEW_START:
        {
            int x  = 0;
            int y  = 0;

            p->GetViewStart(&x, &y);

            JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
            *vp = OBJECT_TO_JSVAL(objArr);
            jsval element = ToJS(cx, x);
            JS_SetElement(cx, objArr, 0, &element);
            element = ToJS(cx, y);
            JS_SetElement(cx, objArr, 1, &element);
        }
    case P_VIRTUAL_SIZE:
        {
            int x  = 0;
            int y  = 0;

            p->GetVirtualSize(&x, &y);

            JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
            *vp = OBJECT_TO_JSVAL(objArr);
            jsval element = ToJS(cx, x);
            JS_SetElement(cx, objArr, 0, &element);
            element = ToJS(cx, y);
            JS_SetElement(cx, objArr, 1, &element);
        }
    }
    return true;
}

bool ScrolledWindow::AddProperty(wxScrolledWindow *p, 
                                 JSContext* WXUNUSED(cx), 
                                 JSObject* WXUNUSED(obj), 
                                 const wxString &prop, 
                                 jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    PanelEventHandler::ConnectEvent(p, prop, true);

    return true;
}


bool ScrolledWindow::DeleteProperty(wxScrolledWindow *p, 
                                    JSContext* WXUNUSED(cx), 
                                    JSObject* WXUNUSED(obj), 
                                    const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  PanelEventHandler::ConnectEvent(p, prop, false);
  return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">A windows identifier.
 *    Use -1 when you don't need it.</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the control on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the control</arg>
 *   <arg name="Style" type="Integer" 
 *        default="wxWindow.HSCROLL + wxWindow.VSCROLL">
 *    The style of the control
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxScrolledWindow object.
 *  </desc>
 * </ctor>
 */
wxScrolledWindow* ScrolledWindow::Construct(JSContext *cx,
                                            JSObject *obj,
                                            uintN argc,
                                            jsval *argv,
                                            bool WXUNUSED(constructing))
{
  wxScrolledWindow *p = new wxScrolledWindow();
  SetPrivate(cx, obj, p);

  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(ScrolledWindow)
    WXJS_METHOD("calcScrolledPosition", calcScrolledPosition, 2)
    WXJS_METHOD("calcUnscrolledPosition", calcUnscrolledPosition, 2)
    WXJS_METHOD("enableScrolling", enableScrolling, 2)
    WXJS_METHOD("scroll", scroll, 2)
    WXJS_METHOD("setScrollbars", setScrollbars, 4)
    WXJS_METHOD("setScrollRate", setScrollRate, 2)
    WXJS_METHOD("setTargetWindow", setTargetWindow, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function>
 *   <arg name="Parent" type="@wxWindow">The parent window</arg>
 *   <arg name="Id" type="Integer">A windows identifier.
 *    Use -1 when you don't need it.</arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the control on the given parent</arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the control</arg>
 *   <arg name="Style" type="Integer" 
 *        default="wxWindow.HSCROLL + wxWindow.VSCROLL">
 *    The style of the control
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxScrolledWindow object.
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::create(JSContext *cx,
                              JSObject *obj,
                              uintN argc,
                              jsval *argv,
                              jsval *rval)
{
  wxScrolledWindow *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;
  int style = wxHSCROLL + wxVSCROLL;
  int id = -1;

  if ( argc > 5 )
    argc = 5;

  switch(argc)
  {
  case 5:
	if ( ! FromJS(cx, argv[4], style) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
      return JS_FALSE;
    }
	// Walk through
  case 4:
	size = Size::GetPrivate(cx, argv[3]);
	if ( size == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxSize");
      return JS_FALSE;
    }
	// Walk through
  case 3:
    pt = wxjs::ext::GetPoint(cx, argv[2]);
	if ( pt == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxPoint");
      return JS_FALSE;
    }
	// Walk through
  case 2:
    if ( ! FromJS(cx, argv[1], id) )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
      return JS_FALSE;
    }
    // Walk through
  default:

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

	if ( p->Create(parent, id, *pt, *size, style) )
    {
      *rval = JSVAL_TRUE;
      p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
    }
  }
  return JS_TRUE;
}

/***
 * <method name="calcScrolledPosition">
 *  <function returns="Array">
 *   <arg name="x" type="Integer" />
 *   <arg name="y" type="Integer" />
 *  </function>
 *  <desc>
 *   Translates the logical coordinates to the device ones.
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::calcScrolledPosition(JSContext* cx,
                                            JSObject* obj,
                                            uintN WXUNUSED(argc),
                                            jsval* argv,
                                            jsval* rval)
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int x  = 0;
    int y  = 0;
    int xx = 0;
    int yy = 0;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y) )
    {
        p->CalcScrolledPosition(x, y, &xx, &yy);
    }

    JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
    *rval = OBJECT_TO_JSVAL(objArr);
    jsval element = ToJS(cx, xx);
    JS_SetElement(cx, objArr, 0, &element);
    element = ToJS(cx, yy);
    JS_SetElement(cx, objArr, 1, &element);

	return JS_TRUE;
}

/***
 * <method name="calcUnscrolledPosition">
 *  <function returns="Array">
 *   <arg name="x" type="Integer" />
 *   <arg name="y" type="Integer" />
 *  </function>
 *  <desc>
 *   Translates the device coordinates to the logical ones.
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::calcUnscrolledPosition(JSContext *cx,
                                              JSObject *obj,
                                              uintN WXUNUSED(argc),
                                              jsval *argv,
                                              jsval *rval)
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int x  = 0;
    int y  = 0;
    int xx = 0;
    int yy = 0;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y) )
    {
        p->CalcUnscrolledPosition(x, y, &xx, &yy);
    }

    JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
    *rval = OBJECT_TO_JSVAL(objArr);
    jsval element = ToJS(cx, xx);
    JS_SetElement(cx, objArr, 0, &element);
    element = ToJS(cx, yy);
    JS_SetElement(cx, objArr, 1, &element);

	return JS_TRUE;
}

/***
 * <method name="enableScrolling">
 *  <function>
 *   <arg name="xScrolling" type="Boolean" />
 *   <arg name="yScrolling" type="Boolean" />
 *  </function>
 *  <desc>
 *   Enable or disable physical scrolling in the given direction.
 *   Physical scrolling is the physical transfer of bits up or down the 
 *   screen when a scroll event occurs. If the application scrolls by a 
 *   variable amount (e.g. if there are different font sizes) 
 *   then physical scrolling will not work, and you should switch it off. 
 *   Note that you will have to reposition child windows yourself, if physical 
 *   scrolling is disabled.
 *   <blockquote>
 *    Physical scrolling may not be available on all platforms. Where it is 
 *    available, it is enabled by default.
 *   </blockquote>
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::enableScrolling(JSContext *cx,
                                       JSObject *obj,
                                       uintN WXUNUSED(argc),
                                       jsval *argv,
                                       jsval* WXUNUSED(rval))
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    bool xScrolling = true;
    bool yScrolling = true;

    if (    FromJS(cx, argv[0], xScrolling)
         && FromJS(cx, argv[1], yScrolling) )
    {
        p->EnableScrolling(xScrolling, yScrolling);
    }

    return JS_TRUE;
}

/***
 * <method name="getView">
 *  <function returns="Array" />
 *  <desc>
 *   Get the number of pixels per scroll unit (line), in each direction, as set 
 *   by @wxScrolledWindow#setScrollbars. A value of zero indicates no scrolling 
 *   in that direction.
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::getScrollPixelsPerUnit(JSContext *cx,
                                              JSObject *obj,
                                              uintN WXUNUSED(argc),
                                              jsval* WXUNUSED(argv),
                                              jsval *rval)
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int x  = 0;
    int y  = 0;

    p->GetScrollPixelsPerUnit(&x, &y);

    JSObject *objArr = JS_NewArrayObject(cx, 2, NULL);
    *rval = OBJECT_TO_JSVAL(objArr);
    jsval element = ToJS(cx, x);
    JS_SetElement(cx, objArr, 0, &element);
    element = ToJS(cx, y);
    JS_SetElement(cx, objArr, 1, &element);

	return JS_TRUE;
}

/***
 * <method name="scroll">
 *  <function>
 *   <arg name="x" type="Integer">The x position to scroll to</arg>
 *   <arg name="y" type="Integer">The y position to scroll to</arg>
 *  </function>
 *  <desc>
 *   Scrolls a window so the view start is at the given point.
 *   <blockquote>
 *    The positions are in scroll units, not pixels, so to convert to pixels
 *    you will have to multiply by the number of pixels per scroll increment. 
 *    If either parameter is -1, 
 *    that position will be ignored (no change in that direction).
 *   </blockquote>
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::scroll(JSContext *cx, 
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval* WXUNUSED(rval))
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y) )
    {
        p->Scroll(x, y);
    }

    return JS_TRUE;
}

/***
 * <method name="setScrollbars">
 *  <function>
 *   <arg name="pixelsPerUnitX" type="Integer">
 *    Pixels per scroll unit in the horizontal direction.
 *   </arg>
 *   <arg name="pixelsPerUnitY" type="Integer">
 *    Pixels per scroll unit in the vertical direction.
 *   </arg>
 *   <arg name="noUnitsX" type="Integer">
 *    Number of units in the horizontal direction.
 *   </arg>
 *   <arg name="noUnitsY" type="Integer">
 *    Number of units in the vertical direction.
 *   </arg>
 *   <arg name="xPos" type="Integer" default="0">
 *    Position to initialize the scrollbars in the horizontal direction, 
 *    in scroll units.
 *   </arg>
 *   <arg name="yPos" type="Integer" default="0">
 *    Position to initialize the scrollbars in the vertical direction, 
 *    in scroll units.
 *   </arg>
 *   <arg name="noRefresh" type="Integer" default="false">
 *    Will not refresh window if true.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets up vertical and/or horizontal scrollbars.
 *   <blockquote>
 *    The first pair of parameters give the number of pixels per 'scroll step', 
 *    i.e. amount moved when the up or down scroll arrows are pressed. 
 *    The second pair gives the length of scrollbar in scroll steps, which sets 
 *    the size of the virtual window.
 *    xPos and yPos optionally specify a position to scroll to immediately.
 *    For example, the following gives a window horizontal and vertical 
 *    scrollbars with 20 pixels per scroll step, and a size of 50 steps 
 *    (1000 pixels) in each direction.
 *    <code class="whjs">
 *     window.setScrollbars(20, 20, 50, 50);
 *    </code>
 *    wxScrolledWindow manages the page size itself, using the current client 
 *    window size as the page size.
 *   </blockquote>
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::setScrollbars(JSContext *cx,
                                     JSObject *obj,
                                     uintN argc,
                                     jsval *argv,
                                     jsval* WXUNUSED(rval))
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int pixelsPerUnitX;
    int pixelsPerUnitY;
    int noUnitsX;
    int noUnitsY;
    int xPos = 0;
    int yPos = 0;
    bool noRefresh = false;

    if ( argc > 7 )
        argc = 7;
    switch(argc)
    {
    case 7:
        if ( ! FromJS(cx, argv[6], noRefresh) )
            break;
        // fall trough
    case 6:
        if ( ! FromJS(cx, argv[5], yPos) )
            break;
        // fall through
    case 5:
        if ( ! FromJS(cx, argv[4], xPos) )
            break;
        // fall through
    default:
        {
            if (    FromJS(cx, argv[0], pixelsPerUnitX)
                 && FromJS(cx, argv[1], pixelsPerUnitY)
                 && FromJS(cx, argv[2], noUnitsX)
                 && FromJS(cx, argv[3], noUnitsY) )
            {
                p->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
                                 noUnitsX, noUnitsY, xPos, yPos, noRefresh);
            }
        }
    }
    return JS_TRUE;
}

/***
 * <method name="setScrollRate">
 *  <function>
 *   <arg name="xStep" type="Integer" />
 *   <arg name="yStep" type="Integer" />
 *  </function>
 *  <desc>
 *   Set the horizontal and vertical scrolling increment only. 
 *   See the pixelsPerUnit parameter in @wxScrolledWindow#setScrollbars.
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::setScrollRate(JSContext *cx,
                                     JSObject *obj,
                                     uintN WXUNUSED(argc),
                                     jsval *argv,
                                     jsval* WXUNUSED(rval))
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int x = 0;
    int y = 0;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y) )
    {
        p->SetScrollRate(x, y);
    }

    return JS_TRUE;
}

/***
 * <method name="setTargetWindow">
 *  <function>
 *   <arg name="Window" type="@wxWindow" />
 *  </function>
 *  <desc>
 *   Call this function to tell wxScrolledWindow to perform the actual scrolling
 *   on a different window (and not on itself).
 *  </desc>
 * </method>
 */
JSBool ScrolledWindow::setTargetWindow(JSContext *cx,
                                       JSObject *obj,
                                       uintN WXUNUSED(argc),
                                       jsval *argv,
                                       jsval* WXUNUSED(rval))
{
    wxScrolledWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    wxWindow *win = Window::GetPrivate(cx, argv[0]);
    if ( win )
    {
        p->SetTargetWindow(win);
    }

    return JS_TRUE;
}
