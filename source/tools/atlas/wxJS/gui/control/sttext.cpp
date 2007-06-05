#include "precompiled.h"

/*
 * wxJavaScript - sttext.cpp
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
 * $Id: sttext.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// sttext.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "../event/evthand.h"

#include "sttext.h"
#include "window.h"

#include "../misc/point.h"
#include "../misc/size.h"

using namespace wxjs;
using namespace wxjs::gui;

StaticText::StaticText(JSContext *cx, JSObject *obj)
	:    wxStaticText()
	   , Object(obj, cx)
{
	PushEventHandler(new EventHandler(this));
}
	
StaticText::~StaticText()
{
	PopEventHandler(true);
}

/***
 * <file>control/sttext</file>
 * <module>gui</module>
 * <class name="wxStaticText" prototype="@wxControl">
 *  A static text control displays one or more lines of read-only text.
 * </class>
 */
WXJS_INIT_CLASS(StaticText, "wxStaticText", 3)

/***
 * <properties>
 *  <property name="label" type="String">
 *   Get/Sets the text.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(StaticText)
  WXJS_PROPERTY(P_LABEL, "label")
WXJS_END_PROPERTY_MAP()

/***
 * <constants>
 *  <type name="Style">
 *   <constant name="ALIGN_LEFT" />
 *   <constant name="ALIGN_RIGHT" />
 *   <constant name="ALIGN_CENTER" />
 *   <constant name="NO_AUTORESIZE" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(StaticText)
  WXJS_CONSTANT(wx, ALIGN_LEFT)
  WXJS_CONSTANT(wx, ALIGN_RIGHT)
  WXJS_CONSTANT(wx, ALIGN_CENTER)
  WXJS_CONSTANT(wxST_, NO_AUTORESIZE)
WXJS_END_CONSTANT_MAP()

bool StaticText::GetProperty(wxStaticText *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if (id == P_LABEL )
    {
		*vp = ToJS(cx, p->GetLabel());
    }
    return true;
}

bool StaticText::SetProperty(wxStaticText *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_LABEL )
	{
		wxString label;
		FromJS(cx, *vp, label);
		p->SetLabel(label);
	}
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of the static text. This can't be <I>null</I>.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    The unique identifier.
 *   </arg>
 *   <arg name="Text" type="String">
 *    The text of the static text
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the static text.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the static text.
 *   </arg>
 *   <arg name="Style" type="Integer" default="0">
 *    The style of the static text. See @wxStaticText#styles
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxStaticText object
 *  </desc>
 * </ctor>
 */
wxStaticText* StaticText::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc > 6 )
        argc = 6;

	const wxPoint *pt = &wxDefaultPosition;
	const wxSize *size = &wxDefaultSize;
    int style = 0;

    switch(argc)
    {
    case 6:
        if ( ! FromJS(cx, argv[5], style) )
            break;
        // Fall through
    case 5:
		size = Size::GetPrivate(cx, argv[4]);
		if ( size == NULL )
			break;
		// Fall through
	case 4:
		pt = Point::GetPrivate(cx, argv[3]);
		if ( pt == NULL )
			break;
		// Fall through
    default:
        wxString text;
        FromJS(cx, argv[2], text);

        int id;
        if ( ! FromJS(cx, argv[1], id) )
            break;

        wxWindow *parent = Window::GetPrivate(cx, argv[0]);
		if ( parent == NULL )
			break;

        Object *wxjsParent = dynamic_cast<Object *>(parent);
        JS_SetParent(cx, obj, wxjsParent->GetObject());

	    wxStaticText *p = new StaticText(cx, obj);
	    p->Create(parent, id, text, *pt, *size, style);
        return p;
    }

    return NULL;
}
