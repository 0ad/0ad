#include "precompiled.h"

/*
 * wxJavaScript - fontdata.cpp
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
 * $Id: fontdata.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// fontdata.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "fontdata.h"

#include "../misc/font.h"
#include "../misc/colour.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/fontdata</file>
 * <module>gui</module>
 * <class name="wxFontData">
 *  This class holds a variety of information related to font dialogs.
 *  See @wxFontDialog
 * </class>
 */
WXJS_INIT_CLASS(FontData, "wxFontData", 0)

/***
 * <properties>
 *  <property name="allowSymbols" type="Boolean">
 *   Under MS Windows, get/set a flag determining whether symbol fonts can be selected. 
 *   Has no effect on other platforms
 *  </property>
 *  <property name="enableEffects" type="Boolean">
 *   Get/Set whether 'effects' are enabled under Windows. This refers to the controls for 
 *   manipulating colour, strikeout and underline properties.
 *  </property>
 *  <property name="chosenFont" type="@wxFont">
 *   Get the selected font
 *  </property>
 *  <property name="colour" type="@wxColour" />
 *  <property name="initialFont" type="@wxFont">
 *   Get/Set the font that will be initially used by the font dialog
 *  </property>
 *  <property name="showHelp" type="boolean">
 *   Show the help button? Windows only.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FontData)
  WXJS_PROPERTY(P_ALLOW_SYMBOLS, "allowSymbols")
  WXJS_PROPERTY(P_ENABLE_EFFECTS, "enableEffects")
  WXJS_PROPERTY(P_CHOSEN_FONT, "chosenFont")
  WXJS_PROPERTY(P_COLOUR, "colour")
  WXJS_PROPERTY(P_INITIAL_FONT, "initialFont")
  WXJS_PROPERTY(P_SHOW_HELP, "showHelp")
WXJS_END_PROPERTY_MAP()

bool FontData::GetProperty(wxFontData *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_ALLOW_SYMBOLS:
		*vp = ToJS(cx, p->GetAllowSymbols());
		break;
	case P_ENABLE_EFFECTS:
		*vp = ToJS(cx, p->GetEnableEffects());
		break;
	case P_CHOSEN_FONT:
        *vp = Font::CreateObject(cx, new wxFont(p->GetChosenFont()), obj);
		break;
	case P_COLOUR:
		*vp = Colour::CreateObject(cx, new wxColour(p->GetColour()));
		break;
	case P_INITIAL_FONT:
        *vp = Font::CreateObject(cx, new wxFont(p->GetInitialFont()), obj);
		break;
	case P_SHOW_HELP:
		*vp = ToJS(cx, p->GetShowHelp());
		break;
    }
    return true;
}

bool FontData::SetProperty(wxFontData *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch (id) 
	{
	case P_ALLOW_SYMBOLS:
		{
			bool value;
			if ( FromJS(cx, *vp, value) )
				p->SetAllowSymbols(value);
			break;
		}
	case P_ENABLE_EFFECTS:
		{
			bool value;
			if ( FromJS(cx, *vp, value) )
				p->EnableEffects(value);
			break;
		}
	case P_CHOSEN_FONT:
		{
            wxFont *value = Font::GetPrivate(cx, *vp);
			if ( value != NULL )
				p->SetChosenFont(*value);
			break;
		}
	case P_COLOUR:
		{
			wxColour *colour = Colour::GetPrivate(cx, *vp);
			if ( colour != NULL )
				p->SetColour(*colour);
			break;
		}
	case P_INITIAL_FONT:
		{
            wxFont *value = Font::GetPrivate(cx, *vp);
			if ( value != NULL )
				p->SetInitialFont(*value);
			break;
		}
	case P_SHOW_HELP:
		{
			bool value;
			if ( FromJS(cx, *vp, value) )
				p->SetShowHelp(value);
			break;
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Constructs a new wxFontData object.
 *  </desc>
 * </ctor>
 */
wxFontData* FontData::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new wxFontData();
}
