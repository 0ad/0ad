#include "precompiled.h"

/*
 * wxJavaScript - listitattr.cpp
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
 * $Id: listitattr.cpp 784 2007-06-25 18:34:22Z fbraem $
 */
// listitem.cpp
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/listctrl.h>

#include "../../common/main.h"

#include "../misc/font.h"
#include "../misc/colour.h"

#include "listitattr.h"
#include "listctrl.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/listitattr</file>
 * <module>gui</module>
 * <class name="wxListItemAttr">
 *  wxListItemAttr is used in virtual list controls.
 *  See @wxListCtrl#onGetItemAttr property
 * </class>
 */
WXJS_INIT_CLASS(ListItemAttr, "wxListItemAttr", 0)

/***
 * <properties>
 *  <property name="textColour" type="@wxColour">
 *   The colour used for displaying text.
 *  </property>
 *  <property name="backgroundColour" type="@wxColour">
 *   The colour used as background.
 *  </property>
 *  <property name="font" type="@wxFont">
 *   The font used for displaying text.
 *  </property>
 *  <property name="hasTextColour" type="Boolean" readonly="Y">
 *   Indicates that this attribute defines the text colour.
 *  </property>
 *  <property name="hasBackgroundColour" type="Boolean" readonly="Y">
 *   Indicates that this attribute defines the background colour.
 *  </property>
 *  <property name="hasFont" type="Boolean" readonly="Y">
 *   Indicates that this attributes defines a font.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ListItemAttr)
    WXJS_PROPERTY(P_TEXT_COLOUR, "textColour")
    WXJS_PROPERTY(P_BG_COLOUR, "backgroundColour")
    WXJS_PROPERTY(P_FONT, "font")
    WXJS_READONLY_PROPERTY(P_HAS_TEXT_COLOUR, "hasTextColour")
    WXJS_READONLY_PROPERTY(P_HAS_BG_COLOUR, "hasBackgroundColour")
    WXJS_READONLY_PROPERTY(P_HAS_FONT, "hasFont")
WXJS_END_PROPERTY_MAP()

bool ListItemAttr::GetProperty(wxListItemAttr *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_TEXT_COLOUR:
        {
            wxColour colour = p->GetTextColour();
            *vp = ( colour == wxNullColour ) ? JSVAL_VOID
                                             : Colour::CreateObject(cx, new wxColour(colour));
            break;
        }
    case P_BG_COLOUR:
        {
            wxColour colour = p->GetBackgroundColour();
            *vp = ( colour == wxNullColour ) ? JSVAL_VOID
                                             : Colour::CreateObject(cx, new wxColour(colour));
            break;
        }
    case P_FONT:
        {
            wxFont font = p->GetFont();
            *vp = ( font == wxNullFont ) ? JSVAL_VOID
                                         : Font::CreateObject(cx, new wxFont(font));
            break;
        }
    case P_HAS_TEXT_COLOUR:
        *vp = ToJS(cx, p->HasTextColour());
        break;
    case P_HAS_BG_COLOUR:
        *vp = ToJS(cx, p->HasBackgroundColour());
        break;
    case P_HAS_FONT:
        *vp = ToJS(cx, p->HasFont());
        break;
    }
    return true;
}

bool ListItemAttr::SetProperty(wxListItemAttr *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_TEXT_COLOUR:
        {
            wxColour *colour = Colour::GetPrivate(cx, obj);
            if ( colour != NULL )
                p->SetTextColour(*colour);
            break;
        }
    case P_BG_COLOUR:
        {
            wxColour *colour = Colour::GetPrivate(cx, obj);
            if ( colour != NULL )
                p->SetBackgroundColour(*colour);
            break;
        }
    case P_FONT:
        {
            wxFont *font = Font::GetPrivate(cx, obj);
            if ( font != NULL )
                p->SetFont(*font);
            break;
        }
    }
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Creates a new wxListItem object.
 *  </desc>
 * </ctor>
 */
wxListItemAttr *ListItemAttr::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new wxListItemAttr();
}
