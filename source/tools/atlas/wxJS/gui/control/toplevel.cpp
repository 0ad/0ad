#include "precompiled.h"

/*
 * wxJavaScript - toplevel.cpp
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
 * $Id: toplevel.cpp 714 2007-05-16 20:24:49Z fbraem $
 */
// toplevel.cpp

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../misc/icon.h"
#include "toplevel.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/toplevel</file>
 * <module>gui</module>
 * <class name="wxTopLevelWindow" prototype="wxWindow">
 *  wxTopLevelWindow is the prototype class for @wxDialog and @wxFrame. 
 * </class>
 */
WXJS_INIT_CLASS(TopLevelWindow, "wxTopLevelWindow", 0)

/***
 * <constants>
 *  <type name="wxFullScreen">
 *   <constant name="NOMENUBAR" /> 
 *   <constant name="NOTOOLBAR" /> 
 *   <constant name="NOSTATUSBAR" />  
 *   <constant name="NOBORDER" /> 
 *   <constant name="NOCAPTION" /> 
 *   <constant name="ALL" /> 
 *   <desc>
 *    wxFullScreen is ported to wxJS as a separate class
 *   </desc>
 *  </type>
 * </constants>
 */
void TopLevelWindow::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxFullScreenMap[] = 
    {
       WXJS_CONSTANT(wxFULLSCREEN_, NOMENUBAR)
       WXJS_CONSTANT(wxFULLSCREEN_, NOTOOLBAR)
       WXJS_CONSTANT(wxFULLSCREEN_, NOSTATUSBAR) 
       WXJS_CONSTANT(wxFULLSCREEN_, NOBORDER) 
       WXJS_CONSTANT(wxFULLSCREEN_, NOCAPTION) 
       WXJS_CONSTANT(wxFULLSCREEN_, ALL)
       { 0 }
    };
    JSObject *constObj = JS_DefineObject(cx, obj, "wxFullScreen", 
							             NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxFullScreenMap);
}

/***
 * <properties>
 *  <property name="fullScreen" type="Boolean">
 *   Get/Set fullscreen mode
 *  </property>
 *  <property name="icon" type="@wxIcon">
 *   Get/Set icon
 *  </property>
 *  <property name="icons" type="wxIconBundle">
 *   Not yet implemented
 *  </property>
 *  <property name="active" type="Boolean" readonly="Y">
 *   Is the window active?
 *  </property>
 *  <property name="defaultItem" type="@wxWindow" />
 *  <property name="iconized" type="Boolean">
 *   Get/Set iconized mode
 *  </property>
 *  <property name="maximized" type="Boolean">
 *   Get/Set maximized mode
 *  </property>
 *  <property name="title" type="String">
 *   Get/Set the title
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TopLevelWindow)
    WXJS_PROPERTY(P_DEFAULT_ITEM, "defaultItem")
	WXJS_PROPERTY(P_FULL_SCREEN, "fullScreen")
	WXJS_PROPERTY(P_ICON, "icon")
	WXJS_PROPERTY(P_ICONS, "icons")
	WXJS_READONLY_PROPERTY(P_ACTIVE, "active")
	WXJS_PROPERTY(P_ICONIZED, "iconized")
	WXJS_PROPERTY(P_MAXIMIZED, "maximized")
	WXJS_PROPERTY(P_TITLE, "title")
WXJS_END_PROPERTY_MAP()

bool TopLevelWindow::GetProperty(wxTopLevelWindow *p,
                                 JSContext *cx,
                                 JSObject* WXUNUSED(obj),
                                 int id,
                                 jsval *vp)
{
	switch(id)  
	{
	case P_ICON:
        *vp = Icon::CreateObject(cx, new wxIcon(p->GetIcon()));
		break;
	case P_FULL_SCREEN:
		*vp = ToJS(cx, p->IsFullScreen());
		break;
	case P_ICONS:
		//TODO
		break;
	case P_ICONIZED:
		*vp = ToJS(cx, p->IsIconized());
		break;
	case P_MAXIMIZED:
		*vp = ToJS(cx, p->IsMaximized());
		break;
	case P_TITLE:
		*vp = ToJS(cx, p->GetTitle());
		break;
    case P_DEFAULT_ITEM:
      {
        wxWindow *win = p->GetDefaultItem();
        if ( win != NULL )
        {
          JavaScriptClientData *data
            = dynamic_cast<JavaScriptClientData*>(win->GetClientObject());
          if ( data != NULL )
          {
            *vp = OBJECT_TO_JSVAL(data->GetObject());
          }
        }
        break;
      }
	}

	return true;
}

bool TopLevelWindow::SetProperty(wxTopLevelWindow *p,
                                 JSContext *cx,
                                 JSObject* WXUNUSED(obj),
                                 int id,
                                 jsval *vp)
{
	switch(id) 
	{
	case P_ICONS:
		//TODO
		break;
	case P_ICON:
		{
            wxIcon *ico = Icon::GetPrivate(cx, *vp);
			if ( ico )
				p->SetIcon(*ico);
			break;
		}
	case P_ICONIZED:
		{
			bool iconize;
			if ( FromJS(cx, *vp, iconize) )
				p->Iconize(iconize);
			break;
		}
	case P_FULL_SCREEN:
		{
			bool full;
			if ( FromJS(cx, *vp, full) )
				p->ShowFullScreen(full, wxFULLSCREEN_ALL);
			break;
		}
	case P_MAXIMIZED:
		{
			bool maximize;
			if ( FromJS(cx, *vp, maximize) )
				p->Maximize(maximize);
			break;
		}
	case P_TITLE:
		{
			wxString title;
			FromJS(cx, *vp, title);
			p->SetTitle(title);
			break;
		}
    case P_DEFAULT_ITEM:
        wxWindow *win = Window::GetPrivate(cx, *vp);
        if ( win != NULL )
        {
            p->SetDefaultItem(win);
        }
        break;
	}
    return true;
}

WXJS_BEGIN_METHOD_MAP(TopLevelWindow)
	WXJS_METHOD("requestUserAttention", requestUserAttention, 0)
	WXJS_METHOD("setLeftMenu", setLeftMenu, 0)
	WXJS_METHOD("setRightMenu", setRightMenu, 0)
WXJS_END_METHOD_MAP()

JSBool TopLevelWindow::requestUserAttention(JSContext *cx,
                                            JSObject *obj,
                                            uintN argc,
                                            jsval *argv,
                                            jsval *rval)
{
	//TODO
	return JS_TRUE;
}

JSBool TopLevelWindow::setLeftMenu(JSContext *cx,
                                   JSObject *obj,
                                   uintN argc,
                                   jsval *argv,
                                   jsval *rval)
{
	//TODO
	return JS_TRUE;
}

JSBool TopLevelWindow::setRightMenu(JSContext *cx,
                                    JSObject *obj,
                                    uintN argc,
                                    jsval *argv,
                                    jsval *rval)
{
	//TODO
	return JS_TRUE;
}
/***
 * <method name="showFullScreen">
 *  <function returns="Boolean">
 *   <arg name="Show" type="Boolean">
 *    Show the frame in full screen when true, restore when false.
 *   </arg>
 *   <arg name="Style=" type="Integer" default="wxFullScreen.ALL">
 *    Use one of @wxTopLevelWindow#wxFullScreen.
 *   </arg>
 *  </function>
 *  <desc>
 *   Shows the frame in full screen or restores the frame.
 *  </desc>
 * </method>
 */
JSBool TopLevelWindow::showFullScreen(JSContext *cx,
                                      JSObject *obj,
                                      uintN argc,
                                      jsval *argv,
                                      jsval *rval)
{
	wxTopLevelWindow *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    bool show;
    if ( ! FromJS(cx, argv[0], show) )
        return JS_FALSE;

    int style = wxFULLSCREEN_ALL;
    if (    argc > 1 
         && ! FromJS(cx, argv[1], style) )
         return JS_FALSE;

    *rval = ToJS(cx, p->ShowFullScreen(show, style));

	return JS_TRUE;
}
