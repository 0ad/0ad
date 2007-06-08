#include "precompiled.h"

/*
 * wxJavaScript - cshelp.cpp
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
 * $Id: cshelp.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// cshelp.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../control/window.h"

#include "cshelp.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/cshelp</file>
 * <module>gui</module>
 * <class name="wxContextHelp">
 *  This class changes the cursor to a query and puts the application into a
 *  'context-sensitive help mode'. When the user left-clicks on a window within 
 *  the specified window, a wxEVT_HELP event is sent to that control, and the 
 *  application may respond to it by popping up some help. 
 * </class>
 */
WXJS_INIT_CLASS(ContextHelp, "wxContextHelp", 0)

/***
 * <ctor>
 *  <function>
 *   <arg name="Window" type="@wxWindow" default="null">
 *    The window that will get the @wxHelpEvent . When the object
 *    is null, the top level window will be used.
 *   </arg>
 *   <arg name="Now" type="Boolean" default="true">
 *    When true (= default), the application will enter the 'context-sensitive help mode'
 *    immediately. Otherwise @wxContextHelp#beginContextHelp needs to be called.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxContextHelp object.
 *  </desc>
 * </ctor>
 */
wxContextHelp* ContextHelp::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 2 )
        argc = 2;

    wxWindow *win = NULL;
    bool now = true;

    switch(argc)
    {
    case 2:
        if ( ! FromJS(cx, argv[1], now) )
            return NULL;
        // Fall through
    case 1:
        if ( ! JSVAL_IS_VOID(argv[0]) )
        {
            win = Window::GetPrivate(cx, argv[0]);
            if ( win == NULL )
                return NULL;
        }
    }

    return new wxContextHelp(win, now);
}

WXJS_BEGIN_METHOD_MAP(ContextHelp)
    WXJS_METHOD("beginContextHelp", beginContextHelp, 0)
    WXJS_METHOD("endContextHelp", endContextHelp, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="beginContextHelp">
 *  <function returns="Boolean">
 *   <arg name="Window" type="@wxWindow" default="null">
 *    The window that will get the @wxHelpEvent. When the object
 *    is null, the top level window will be used.
 *   </arg>
 *  </function>
 *  <desc>
 *   Puts the application into context-sensitive help mode. 
 *   Returns true, when the application was put successfully into context-sensitive help mode.
 *   This function only returns when the event loop has finished.
 *  </desc>
 * </method>
 */
JSBool ContextHelp::beginContextHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxContextHelp *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 1 )
        argc = 1;

    wxWindow *win = NULL;
    if ( argc == 1 )
    {
        win = Window::GetPrivate(cx, argv[0]);
        if ( win == NULL )
            return JS_FALSE;
    }

    *rval = ToJS(cx, p->BeginContextHelp(win));
    return JS_TRUE;
}

/***
 * <method name="endContextHelp">
 *  <function returns="Boolean" />
 *  <desc>
 *   Ends context-sensitive help mode. Not normally called by the application.
 *  </desc>
 * </method>
 */
JSBool ContextHelp::endContextHelp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxContextHelp *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->EndContextHelp();
    return JS_TRUE;
}
