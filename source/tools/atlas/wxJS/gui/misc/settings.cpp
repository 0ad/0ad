#include "precompiled.h"

/*
 * wxJavaScript - settings.cpp
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
 * Remark: This class was donated by Philip Taylor
 *
 * $Id$
 */
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/jsutil.h"

#include "settings.h"

#include "colour.h"
#include "font.h"
#include "../control/window.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <module>gui</module>
 * <file>misc/settings</file>
 * <class name="wxSystemSettings">
 *  wxSystemSettings allows the application to ask for details about the system. 
 *  This can include settings such as standard colours, fonts, and user 
 *  interface element sizes.
 * </class>
 */
WXJS_INIT_CLASS(SystemSettings, "wxSystemSettings", 0)

WXJS_BEGIN_STATIC_PROPERTY_MAP(SystemSettings)
    WXJS_STATIC_PROPERTY(P_SCREEN_TYPE, "screenType")
WXJS_END_PROPERTY_MAP()

/***
 * <class_properties>
 *  <property name="screenType" type="Integer">
 *   Get/Set the screen type.
 *  </property>
 * </class_properties>
 */
bool SystemSettings::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
	switch ( id )
	{
	case P_SCREEN_TYPE:
		*vp = ToJS(cx, static_cast<int>(wxSystemSettings::GetScreenType()));
		break;
	}
	return true;
}

bool SystemSettings::SetStaticProperty(JSContext *cx, int id, jsval *vp)
{
	switch ( id )
	{
	case P_SCREEN_TYPE:
		int screenType;
		if (! FromJS(cx, *vp, screenType) )
			return false;
		wxSystemSettings::SetScreenType(static_cast<wxSystemScreenType>(screenType));
		break;
	}
	return true;
}

WXJS_BEGIN_STATIC_METHOD_MAP(SystemSettings)
	WXJS_METHOD("getColour", getColour, 1)
	WXJS_METHOD("getFont", getFont, 1)
	WXJS_METHOD("getMetric", getMetric, 1)
	WXJS_METHOD("hasFeature", hasFeature, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="getColour">
 *  <function returns="@wxColour">
 *   <arg name="Index" type="Integer" />
 *  </function>
 *  <desc>
 *   Returns a system colour.
 *  </desc>
 * </class_method>
 */
JSBool SystemSettings::getColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	wxColour colour = wxSystemSettings::GetColour(static_cast<wxSystemColour>(index));
	*rval = Colour::CreateObject(cx, new wxColour(colour));
	return JS_TRUE;
}

/***
 * <class_method name="getFont">
 *  <function returns="@wxFont">
 *   <arg name="Index" type="Integer" />
 *  </function>
 *  <desc>
 *   Returns a system font.
 *  </desc>
 * </class_method>
 */
JSBool SystemSettings::getFont(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	wxFont font = wxSystemSettings::GetFont(static_cast<wxSystemFont>(index));
	*rval = Font::CreateObject(cx, new wxFont(font));
	return JS_TRUE;
}

/***
 * <class_method name="getColour">
 *  <function returns="Integer">
 *   <arg name="Index" type="Integer" />
 *   <arg name="Win" type="@wxWindow" default="null" />
 *  </function>
 *  <desc>
 *   Returns the value of a system metric, or -1 if the metric is not
 *   supported on the current system. Specifying the win parameter is 
 *   encouraged, because some metrics on some ports are not supported without 
 *   one, or they might be capable of reporting better values if given one. 
 *   If a window does not make sense for a metric, one should still be given, 
 *   as for example it might determine which displays cursor width is requested 
 *   with wxSystemSettings.CURSOR_X
 *  </desc>
 * </class_method>
 */
JSBool SystemSettings::getMetric(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	wxWindow* window = NULL;
	if ( argc >= 2 && ! JSVAL_IS_NULL(argv[1]) )
	{
		window = Window::GetPrivate(cx, argv[1]);
		if ( window == NULL )
			return JS_FALSE;
	}
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	int metric = wxSystemSettings::GetMetric(static_cast<wxSystemMetric>(index), window);
	*rval = ToJS(cx, metric);
	return JS_TRUE;
}

JSBool SystemSettings::hasFeature(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int index;
	if (! FromJS(cx, argv[0], index))
		return JS_FALSE;
	bool present = wxSystemSettings::HasFeature(static_cast<wxSystemFeature>(index));
	*rval = ToJS(cx, present);
	return JS_TRUE;
}

/***
 * <constants>
 *  <type name="System Colours">
 *   <constant name="COLOUR_SCROLLBAR">The scrollbar grey area.</constant>
 *   <constant name="COLOUR_BACKGROUND">The desktop colour.</constant>
 *   <constant name="COLOUR_ACTIVECAPTION">Active window caption.</constant>
 *   <constant name="COLOUR_INACTIVECAPTION">Inactive window caption.</constant>
 *   <constant name="COLOUR_MENU">Menu background.</constant>
 *   <constant name="COLOUR_WINDOW">Window background.</constant>
 *   <constant name="COLOUR_WINDOWFRAME">Window frame.</constant>
 *   <constant name="COLOUR_MENUTEXT">Menu text.</constant>
 *   <constant name="COLOUR_WINDOWTEXT">Text in windows.</constant>
 *   <constant name="COLOUR_CAPTIONTEXT">Text in caption, size box and scrollbar arrow box.</constant>
 *   <constant name="COLOUR_ACTIVEBORDER">Active window border.</constant>
 *   <constant name="COLOUR_INACTIVEBORDER">Inactive window border.</constant>
 *   <constant name="COLOUR_APPWORKSPACE">Background colour MDI applications.</constant>
 *   <constant name="COLOUR_HIGHLIGHT">Item(s) selected in a control.</constant>
 *   <constant name="COLOUR_HIGHLIGHTTEXT">Text of item(s) selected in a control.</constant>
 *   <constant name="COLOUR_BTNFACE">Face shading on push buttons.</constant>
 *   <constant name="COLOUR_BTNSHADOW">Edge shading on push buttons.</constant>
 *   <constant name="COLOUR_GRAYTEXT">Greyed (disabled) text.</constant>
 *   <constant name="COLOUR_BTNTEXT">Text on push buttons.</constant>
 *   <constant name="COLOUR_INACTIVECAPTIONTEXT">Colour of text in active captions.</constant>
 *   <constant name="COLOUR_BTNHIGHLIGHT">Highlight colour for buttons (same as constant name="COLOUR_3DHILIGHT).</constant>
 *   <constant name="COLOUR_3DDKSHADOW">Dark shadow for three-dimensional display elements.</constant>
 *   <constant name="COLOUR_3DLIGHT">Light colour for three-dimensional display elements.</constant>
 *   <constant name="COLOUR_INFOTEXT">Text colour for tooltip controls.</constant>
 *   <constant name="COLOUR_INFOBK">Background colour for tooltip controls.</constant>
 *   <constant name="COLOUR_DESKTOP">Same as COLOUR_BACKGROUND.</constant>
 *   <constant name="COLOUR_3DFACE">Same as COLOUR_BTNFACE.</constant>
 *   <constant name="COLOUR_3DSHADOW">Same as COLOUR_BTNSHADOW.</constant>
 *   <constant name="COLOUR_3DHIGHLIGHT">Same as COLOUR_BTNHIGHLIGHT.</constant>
 *   <constant name="COLOUR_3DHILIGHT">Same as COLOUR_BTNHIGHLIGHT.</constant>
 *   <constant name="COLOUR_BTNHILIGHT">Same as COLOUR_BTNHIGHLIGHT.</constant>
 *  </type>
 *  <type name="System Fonts">
 *   <constant name="OEM_FIXED_FONT">Original equipment manufacturer dependent fixed-pitch font.</constant>
 *   <constant name="ANSI_FIXED_FONT">Windows fixed-pitch font.</constant>
 *   <constant name="ANSI_VAR_FONT">Windows variable-pitch (proportional) font.</constant>
 *   <constant name="SYSTEM_FONT">System font.</constant>
 *   <constant name="DEVICE_DEFAULT_FONT">Device-dependent font (Windows NT only).</constant>
 *   <constant name="DEFAULT_GUI_FONT">Default font for user interface objects such as menus and dialog boxes. Note that with modern GUIs nothing guarantees that the same font is used for all GUI elements, so some controls might use a different font by default.</constant>
 *  </type>
 *  <type name="System Metrics">
 *   <constant name="MOUSE_BUTTONS">Number of buttons on mouse, or zero if no mouse was installed.</constant>
 *   <constant name="BORDER_X">Width of single border.</constant>
 *   <constant name="BORDER_Y">Height of single border.</constant>
 *   <constant name="CURSOR_X">Width of cursor.</constant>
 *   <constant name="CURSOR_Y">Height of cursor.</constant>
 *   <constant name="DCLICK_X">Width in pixels of rectangle within which two successive mouse clicks must fall to generate a double-click.</constant>
 *   <constant name="DCLICK_Y">Height in pixels of rectangle within which two successive mouse clicks must fall to generate a double-click.</constant>
 *   <constant name="DRAG_X">Width in pixels of a rectangle centered on a drag point to allow for limited movement of the mouse pointer before a drag operation begins.</constant>
 *   <constant name="DRAG_Y">Height in pixels of a rectangle centered on a drag point to allow for limited movement of the mouse pointer before a drag operation begins.</constant>
 *   <constant name="EDGE_X">Width of a 3D border, in pixels.</constant>
 *   <constant name="EDGE_Y">Height of a 3D border, in pixels.</constant>
 *   <constant name="HSCROLL_ARROW_X">Width of arrow bitmap on horizontal scrollbar.</constant>
 *   <constant name="HSCROLL_ARROW_Y">Height of arrow bitmap on horizontal scrollbar.</constant>
 *   <constant name="HTHUMB_X">Width of horizontal scrollbar thumb.</constant>
 *   <constant name="ICON_X">The default width of an icon.</constant>
 *   <constant name="ICON_Y">The default height of an icon.</constant>
 *   <constant name="ICONSPACING_X">Width of a grid cell for items in large icon view, in pixels. Each item fits into a rectangle of this size when arranged.</constant>
 *   <constant name="ICONSPACING_Y">Height of a grid cell for items in large icon view, in pixels. Each item fits into a rectangle of this size when arranged.</constant>
 *   <constant name="WINDOWMIN_X">Minimum width of a window.</constant>
 *   <constant name="WINDOWMIN_Y">Minimum height of a window.</constant>
 *   <constant name="SCREEN_X">Width of the screen in pixels.</constant>
 *   <constant name="SCREEN_Y">Height of the screen in pixels.</constant>
 *   <constant name="FRAMESIZE_X">Width of the window frame for a wxTHICK_FRAME window.</constant>
 *   <constant name="FRAMESIZE_Y">Height of the window frame for a wxTHICK_FRAME window.</constant>
 *   <constant name="SMALLICON_X">Recommended width of a small icon (in window captions, and small icon view).</constant>
 *   <constant name="SMALLICON_Y">Recommended height of a small icon (in window captions, and small icon view).</constant>
 *   <constant name="HSCROLL_Y">Height of horizontal scrollbar in pixels.</constant>
 *   <constant name="VSCROLL_X">Width of vertical scrollbar in pixels.</constant>
 *   <constant name="VSCROLL_ARROW_X">Width of arrow bitmap on a vertical scrollbar.</constant>
 *   <constant name="VSCROLL_ARROW_Y">Height of arrow bitmap on a vertical scrollbar.</constant>
 *   <constant name="VTHUMB_Y">Height of vertical scrollbar thumb.</constant>
 *   <constant name="CAPTION_Y">Height of normal caption area.</constant>
 *   <constant name="MENU_Y">Height of single-line menu bar.</constant>
 *   <constant name="NETWORK_PRESENT">1 if there is a network present, 0 otherwise.</constant>
 *   <constant name="PENWINDOWS_PRESENT">1 if PenWindows is installed, 0 otherwise.</constant>
 *   <constant name="SHOW_SOUNDS">Non-zero if the user requires an application to present information visually in situations where it would otherwise present the information only in audible form; zero otherwise.</constant>
 *   <constant name="SWAP_BUTTONS">Non-zero if the meanings of the left and right mouse buttons are swapped; zero otherwise.</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(SystemSettings)
	// wxSystemFont
	WXJS_CONSTANT(wxSYS_, OEM_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, ANSI_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, ANSI_VAR_FONT)
	WXJS_CONSTANT(wxSYS_, SYSTEM_FONT)
	WXJS_CONSTANT(wxSYS_, DEVICE_DEFAULT_FONT)
	WXJS_CONSTANT(wxSYS_, DEFAULT_PALETTE)
	WXJS_CONSTANT(wxSYS_, SYSTEM_FIXED_FONT)
	WXJS_CONSTANT(wxSYS_, DEFAULT_GUI_FONT)
	// wxSystemColour
	WXJS_CONSTANT(wxSYS_, COLOUR_SCROLLBAR)
	WXJS_CONSTANT(wxSYS_, COLOUR_BACKGROUND)
	WXJS_CONSTANT(wxSYS_, COLOUR_DESKTOP)
	WXJS_CONSTANT(wxSYS_, COLOUR_ACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENU)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOWFRAME)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_WINDOWTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_CAPTIONTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_ACTIVEBORDER)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVEBORDER)
	WXJS_CONSTANT(wxSYS_, COLOUR_APPWORKSPACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_HIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_HIGHLIGHTTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNFACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DFACE)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRAYTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INACTIVECAPTIONTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNHIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_BTNHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DHIGHLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DDKSHADOW)
	WXJS_CONSTANT(wxSYS_, COLOUR_3DLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INFOTEXT)
	WXJS_CONSTANT(wxSYS_, COLOUR_INFOBK)
	WXJS_CONSTANT(wxSYS_, COLOUR_LISTBOX)
	WXJS_CONSTANT(wxSYS_, COLOUR_HOTLIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRADIENTACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_GRADIENTINACTIVECAPTION)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUHILIGHT)
	WXJS_CONSTANT(wxSYS_, COLOUR_MENUBAR)
	// wxSystemMetric
	WXJS_CONSTANT(wxSYS_, MOUSE_BUTTONS)
	WXJS_CONSTANT(wxSYS_, BORDER_X)
	WXJS_CONSTANT(wxSYS_, BORDER_Y)
	WXJS_CONSTANT(wxSYS_, CURSOR_X)
	WXJS_CONSTANT(wxSYS_, CURSOR_Y)
	WXJS_CONSTANT(wxSYS_, DCLICK_X)
	WXJS_CONSTANT(wxSYS_, DCLICK_Y)
	WXJS_CONSTANT(wxSYS_, DRAG_X)
	WXJS_CONSTANT(wxSYS_, DRAG_Y)
	WXJS_CONSTANT(wxSYS_, EDGE_X)
	WXJS_CONSTANT(wxSYS_, EDGE_Y)
	WXJS_CONSTANT(wxSYS_, HSCROLL_ARROW_X)
	WXJS_CONSTANT(wxSYS_, HSCROLL_ARROW_Y)
	WXJS_CONSTANT(wxSYS_, HTHUMB_X)
	WXJS_CONSTANT(wxSYS_, ICON_X)
	WXJS_CONSTANT(wxSYS_, ICON_Y)
	WXJS_CONSTANT(wxSYS_, ICONSPACING_X)
	WXJS_CONSTANT(wxSYS_, ICONSPACING_Y)
	WXJS_CONSTANT(wxSYS_, WINDOWMIN_X)
	WXJS_CONSTANT(wxSYS_, WINDOWMIN_Y)
	WXJS_CONSTANT(wxSYS_, SCREEN_X)
	WXJS_CONSTANT(wxSYS_, SCREEN_Y)
	WXJS_CONSTANT(wxSYS_, FRAMESIZE_X)
	WXJS_CONSTANT(wxSYS_, FRAMESIZE_Y)
	WXJS_CONSTANT(wxSYS_, SMALLICON_X)
	WXJS_CONSTANT(wxSYS_, SMALLICON_Y)
	WXJS_CONSTANT(wxSYS_, HSCROLL_Y)
	WXJS_CONSTANT(wxSYS_, VSCROLL_X)
	WXJS_CONSTANT(wxSYS_, VSCROLL_ARROW_X)
	WXJS_CONSTANT(wxSYS_, VSCROLL_ARROW_Y)
	WXJS_CONSTANT(wxSYS_, VTHUMB_Y)
	WXJS_CONSTANT(wxSYS_, CAPTION_Y)
	WXJS_CONSTANT(wxSYS_, MENU_Y)
	WXJS_CONSTANT(wxSYS_, NETWORK_PRESENT)
	WXJS_CONSTANT(wxSYS_, PENWINDOWS_PRESENT)
	WXJS_CONSTANT(wxSYS_, SHOW_SOUNDS)
	WXJS_CONSTANT(wxSYS_, SWAP_BUTTONS)
	// wxSystemFeature
	WXJS_CONSTANT(wxSYS_, CAN_DRAW_FRAME_DECORATIONS)
	WXJS_CONSTANT(wxSYS_, CAN_ICONIZE_FRAME)
	WXJS_CONSTANT(wxSYS_, TABLET_PRESENT)
	// wxSystemScreenType
	WXJS_CONSTANT(wxSYS_, SCREEN_NONE)
	WXJS_CONSTANT(wxSYS_, SCREEN_TINY)
	WXJS_CONSTANT(wxSYS_, SCREEN_PDA)
	WXJS_CONSTANT(wxSYS_, SCREEN_SMALL)
	WXJS_CONSTANT(wxSYS_, SCREEN_DESKTOP)
WXJS_END_CONSTANT_MAP()
