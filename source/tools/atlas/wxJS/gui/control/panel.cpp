#include "precompiled.h"

/*
 * wxJavaScript - panel.cpp
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
 * $Id: panel.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// panel.cpp

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "../../common/main.h"

#include "../event/jsevent.h"
#include "../event/evthand.h"

#include "../misc/size.h"
#include "../misc/point.h"

#include "button.h"
#include "panel.h"
#include "window.h"

using namespace wxjs;
using namespace wxjs::gui;

Panel::Panel(JSContext *cx, JSObject *obj)
	: wxPanel()
	, Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}

Panel::~Panel()
{
	PopEventHandler(true);
}

/***
 * <file>control/panel</file>
 * <module>gui</module>
 * <class name="wxPanel" prototype="@wxWindow">
 *  A panel is a window on which controls are placed. It is usually placed within a frame. 
 *  Its main purpose is to be similar in appearance and functionality to a dialog,
 *  but with the flexibility of having any window as a parent.
 *  See @wxDialog and @wxFrame
 * </class>
 */
WXJS_INIT_CLASS(Panel, "wxPanel", 2)

/***
 * <properties>
 *  <property name="defaultItem" type="@wxButton">
 *   Get/Set the default button.
 *  </property>
 * </properties> 
 */
WXJS_BEGIN_PROPERTY_MAP(Panel)
	WXJS_PROPERTY(P_DEFAULT_ITEM, "defaultItem")
WXJS_END_PROPERTY_MAP()

bool Panel::GetProperty(wxPanel *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_DEFAULT_ITEM )
	{
        wxWindow *win = NULL;
       
        #if wxCHECK_VERSION(2,7,0)
            wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(p), wxTopLevelWindow);
            if ( tlw )
                win = tlw->GetDefaultItem();
        #else       
            win = p->GetDefaultItem();
        #endif

        Object *winObject = dynamic_cast<Object*>(win);
        *vp = winObject == NULL ? JSVAL_VOID : OBJECT_TO_JSVAL(winObject->GetObject()); 
    }
    return true;
}

bool Panel::SetProperty(wxPanel *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_DEFAULT_ITEM )
    {
        wxWindow *win = Window::GetPrivate(cx, *vp);
        if ( win != NULL )
        {
            #if wxCHECK_VERSION(2,7,0)
                wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(p), wxTopLevelWindow);
                if ( tlw )
                    tlw->SetDefaultItem(win);
            #else
                p->SetDefaultItem(win);
            #endif
        } 
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxPanel.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    A window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the Panel control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the Panel control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The wxPanel style.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxPanel object.
 *  </desc>
 * </ctor>
 */
wxPanel* Panel::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 5 )
        argc = 5;

    int style = 0;
    const wxPoint *pt = &wxDefaultPosition;
    const wxSize *size = &wxDefaultSize;

    switch(argc)
    {
    case 5:
        if ( ! FromJS(cx, argv[4], style) )
            break;
        // Fall through
    case 4:
		size = Size::GetPrivate(cx, argv[3]);
		if ( size == NULL )
			break;
        // Fall through
    case 3:
		pt = Point::GetPrivate(cx, argv[2]);
		if ( pt == NULL )
			break;
        // Fall through
    default:
        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
            break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

		wxPanel *p = new Panel(cx, obj);
		p->Create(parent, id, *pt, *size, style); 

        return p;
    }

    return NULL;
}

/***
 * <events>
 *  <event name="onSysColourChanged">
 *   To process a system colour changed event, use this property to set
 *   an event handler function. The function takes a @wxSysColourChangedEvent argument.
 *  </event>
 * </events>
 */
void Panel::OnSysColourChanged(wxSysColourChangedEvent &event)
{
	PrivSysColourChangedEvent::Fire<SysColourChangedEvent>(this, event, "onSysColourChanged");
}

BEGIN_EVENT_TABLE(Panel, wxPanel)
	EVT_SYS_COLOUR_CHANGED(Panel::OnSysColourChanged)
END_EVENT_TABLE()
