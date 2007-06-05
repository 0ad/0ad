#include "precompiled.h"

/*
 * wxJavaScript - caldate.cpp
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
 * $Id: caldate.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// caldate.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/calctrl.h>

/***
 * <file>control/caldate</file>
 * <module>gui</module>
 * <class name="wxCalendarDateAttr">
 *  wxCalendarDateAttr contains all attributes for a calendar date.
 *  See @wxCalendarCtrl. 
 * </class>
 */

#include "../../common/main.h"
#include "../../common/index.h"

#include "../misc/colour.h"
#include "../misc/font.h"

#include "calendar.h"
#include "caldate.h"

using namespace wxjs;
using namespace wxjs::gui;

WXJS_INIT_CLASS(CalendarDateAttr, "wxCalendarDateAttr", 0)

/***
 * <properties>
 *  <property name="backgroundColour" type="@wxColour">The background colour</property>
 *  <property name="border" type="Integer">Type of the border. See @wxCalendarDateBorder.</property>
 *  <property name="borderColour" type="#wxColour">The colour of the border</property>
 *  <property name="font" type="@wxFont">The font of the text</property>
 *  <property name="holiday" type="boolean">Is the day a holiday?</property>
 *  <property name="textColour" type="@wxColour">The colour of the text</property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(CalendarDateAttr)
  WXJS_PROPERTY(P_TEXT_COLOUR, "textColour")
  WXJS_PROPERTY(P_BG_COLOUR, "backgroundColour")
  WXJS_PROPERTY(P_BORDER_COLOUR, "borderColour")
  WXJS_PROPERTY(P_FONT, "font")
  WXJS_PROPERTY(P_BORDER, "border")
  WXJS_PROPERTY(P_HOLIDAY, "holiday")
WXJS_END_PROPERTY_MAP()

bool CalendarDateAttr::GetProperty(wxCalendarDateAttr *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id > 0 && id < 32 ) // Check the day property
    {
		JSObject *parent = JS_GetParent(cx, obj);
		wxASSERT_MSG(parent != NULL, wxT("No parent found for CalendarDateAttr"));

		wxCalendarCtrl *calendar = CalendarCtrl::GetPrivate(cx, parent);
		if ( calendar == NULL )
	        return false;

        wxCalendarDateAttr *attr = calendar->GetAttr(id);
		SetPrivate(cx, obj, (attr == NULL) ? new wxCalendarDateAttr()
                                           : CalendarDateAttr::Clone(attr));
    }
	else
	{
		switch (id) 
		{
		case P_TEXT_COLOUR:
			if ( p->HasTextColour() )
			{
				*vp = Colour::CreateObject(cx, new wxColour(p->GetTextColour()));
			}
			break;
		case P_BG_COLOUR:
			if ( p->HasBackgroundColour() )
			{
				*vp = Colour::CreateObject(cx, new wxColour(p->GetBackgroundColour()));
			}
			break;
		case P_BORDER_COLOUR:
			if ( p->HasBorderColour() )
			{
				*vp = Colour::CreateObject(cx, new wxColour(p->GetBorderColour()));
			}
			break;
		case P_FONT:
			if ( p->HasFont() )
			{
				*vp = Font::CreateObject(cx, new wxFont(p->GetFont()), obj);
			}
			break;
		case P_BORDER:
			if ( p->HasBorder() )
			{
				*vp = ToJS(cx, static_cast<int>(p->GetBorder()));
			}
			break;
		case P_HOLIDAY:
			*vp = ToJS(cx, p->IsHoliday());
			break;
		}
	}
    return true;
}

bool CalendarDateAttr::SetProperty(wxCalendarDateAttr *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id > 0 && id < 32 )
    {
		JSObject *parent = JS_GetParent(cx, obj);
		wxASSERT_MSG(parent != NULL, wxT("No parent found for CalendarAttrItem"));

		wxCalendarCtrl *calendar = CalendarCtrl::GetPrivate(cx, parent);
		if ( calendar == NULL )
			return false;

        wxCalendarDateAttr *attr = GetPrivate(cx, *vp);
        // Clone the attribute because it is owned and destroyed by wxWindows
        // which can give problems. For example: when the calendar object is
        // garbage collected and the attr object is not, the attr object
        // would have an invalid pointer.
        calendar->SetAttr(id, CalendarDateAttr::Clone(attr));
    }
	else
	{
		switch (id) 
		{
		case P_TEXT_COLOUR:
			{
				wxColour *colour = Colour::GetPrivate(cx, *vp);
				if ( colour != NULL )
					p->SetTextColour(*colour);
			}
			break;
		case P_BG_COLOUR:
			{
				wxColour *colour = Colour::GetPrivate(cx, *vp);
				if ( colour != NULL )
					p->SetBackgroundColour(*colour);
			}
			break;
		case P_BORDER_COLOUR:
			{
				wxColour *colour = Colour::GetPrivate(cx, *vp);
				if ( colour != NULL )
					p->SetBorderColour(*colour);
			}
			break;
		case P_FONT:
			{
				wxFont *font = Font::GetPrivate(cx, *vp);
				if ( font != NULL )
					p->SetFont(*font);
			}
			break;
		case P_BORDER:
			{
				int border;
				if ( FromJS<int>(cx, *vp, border) )
					p->SetBorder((wxCalendarDateBorder)border);
				break;
			}
		case P_HOLIDAY:
			{
				bool holiday;
				if ( FromJS(cx, *vp, holiday) )
					p->SetHoliday(holiday);
				break;
			}
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="TextColour" type="@wxColour">Colour of the text</arg>
 *   <arg name="BackgroundColour" type="@wxColour" default="null">Backgroundcolour</arg>
 *   <arg name="BorderColour" type="@wxColour" default="null">BorderColour</arg>
 *   <arg name="Font" type="@wxFont" default="null">The font</arg>
 *   <arg name="Border" type="Integer" default="0">The border type</arg>
 *  </function>
 *  <function>
 *   <arg name="Border" type="Integer">The border type</arg>
 *   <arg name="BorderColour" type="@wxColour">BorderColour</arg>
 *  </function>  
 *  <desc>
 *   Constructs a new wxCalendarDateAttr object.
 *  </desc>
 * </ctor>
 */
wxCalendarDateAttr* CalendarDateAttr::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxCalendarDateAttr();

    if ( Colour::HasPrototype(cx, argv[0]) )
    {
        if ( argc > 5 )
            return NULL;

		int border = wxCAL_BORDER_NONE;
		const wxColour *textColour = &wxNullColour;
		const wxColour *bgColour = &wxNullColour;
		const wxColour *borderColour = &wxNullColour;
		const wxFont *font = &wxNullFont;

        switch(argc)
        {
        case 5:
            if ( ! FromJS(cx, argv[4], border) )
                break;
            // Fall through
        case 4:
            font = Font::GetPrivate(cx, argv[3]);
            if ( font == NULL )
                break;
            // Fall through
        case 3:
            borderColour = Colour::GetPrivate(cx, argv[2]);
            if ( borderColour == NULL )
                break;
            // Fall through
        case 2:
            bgColour = Colour::GetPrivate(cx, argv[1]);
            if ( bgColour == NULL )
                break;
            // Fall through
        default:
            textColour = Colour::GetPrivate(cx, argv[0]);
            if ( textColour == NULL )
                break;
            
			return new wxCalendarDateAttr(*textColour, *bgColour,
										  *borderColour, *font,
										  (wxCalendarDateBorder) border);
        }

        return NULL;
    }
    else
    {
        int border;
        if ( ! FromJS(cx, argv[0], border) )
            return NULL;

        const wxColour *colour = &wxNullColour;
        if (    argc > 1 
             && (colour = Colour::GetPrivate(cx, argv[1])) == NULL )
            return NULL;

        return new wxCalendarDateAttr((wxCalendarDateBorder) border, *colour);
    }
}
