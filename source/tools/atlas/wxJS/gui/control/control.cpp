#include "precompiled.h"

/*
 * wxJavaScript - control.cpp
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
 * $Id: control.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// control.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "control.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/control</file>
 * <module>gui</module>
 * <class name="wxControl" prototype="@wxWindow">
 *  This is the prototype for a control or 'widget'.
 *  A control is generally a small window which processes user input
 *  and/or displays one or more item of data.
 * </class>
 */
WXJS_INIT_CLASS(Control, "wxControl", 0)

/***
 * <properties>
 *  <property name="label" type="String">
 *   Get/Set the label
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Control)
    WXJS_PROPERTY(P_LABEL, "label")
WXJS_END_PROPERTY_MAP()

bool Control::GetProperty(wxControl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_LABEL )
        *vp = ToJS(cx, p->GetLabel());
    return true;
}

bool Control::SetProperty(wxControl *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_LABEL )
    {
        wxString label;
        FromJS(cx, *vp, label);
        p->SetLabel(label);
    }
    return true;
}

void Control::Destruct(JSContext *cx, wxControl* p)
{
}

WXJS_BEGIN_METHOD_MAP(Control)
WXJS_END_METHOD_MAP()

//TODO: An event can't be created yet, so this function is not used.
JSBool Control::command(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxControl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;
    return JS_TRUE;
}
