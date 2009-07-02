#include "precompiled.h"

/*
 * wxJavaScript - globfun.cpp
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
 * $Id: globfun.cpp 810 2007-07-13 20:07:05Z fbraem $
 */
// globfun.cpp
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/image.h>

#include "../../common/main.h"
#include "../../ext/wxjs_ext.h"

#include "globfun.h"
#include "colour.h"
#include "size.h"
#include "fontlist.h"
#include "colourdb.h"

/***
 * <file>misc/globfun</file>
 * <module>gui</module>
 * <class name="Global Functions">
 *  On this page you can find all the functions that are defined on the global object.
 * </class>
 */
static JSFunctionSpec Functions[] =
{
    { "wxMessageBox", wxjs::gui::MessageBox, 1 },
    { "wxInitAllImageHandlers", wxjs::gui::InitAllImageHandlers, 0 },
    { "wxGetKeyState", wxjs::gui::GetKeyState, 1 },
	{ 0 }
};

/***
 * <method name="wxMessageBox">
 *  <function>
 *   <arg name="Text" type="String" />
 *  </function>
 *  <desc>
 *   Shows a modal message box with the given text.
 *  </desc>
 * </method>
 */
JSBool wxjs::gui::MessageBox(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if ( argc == 1 )
	{
        wxString msg;
        FromJS(cx, argv[0], msg);
		wxMessageBox(msg);
		return JS_TRUE;
	}
	return JS_FALSE;
}

/***
 * <method name="wxInitAllImageHandlers">
 *  <function />
 *  <desc>
 *   Initializes all available image handlers. When wxJS is started,
 *   only @wxBMPHandler is instantiated. When you only need one
 *   image handler you can also use @wxImage#addHandler.
 *   <br /><br />
 *   See @wxImage, @wxBMPHandler, @wxGIFHandler, @wxICOHandler, 
 *   @wxJPEGHandler, @wxPCXHandler, @wxPNGHandler, @wxPNMHandler, 
 *   @wxTIFFHandler, @wxXPMHandler
 *  </desc>
 * </method>
 */
JSBool wxjs::gui::InitAllImageHandlers(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxInitAllImageHandlers();
    return JS_TRUE;
}

JSBool wxjs::gui::GetKeyState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int key;
	if (! FromJS(cx, argv[0], key))
		return JS_FALSE;
	*rval = (wxGetKeyState((wxKeyCode)key) ? JSVAL_TRUE : JSVAL_FALSE);
	return JS_TRUE;
}

bool wxjs::gui::InitFunctions(JSContext *cx, JSObject *global)
{
	JS_DefineFunctions(cx, global, Functions);
	return true;
}

/***
 * <properties>
 *  <property name="wxDefaultPosition" type="@wxPoint">
 *   The default position
 *  </property>
 *  <property name="wxDefaultSize" type="@wxSize">
 *   The default size
 *  </property>
 *  <property name="wxTheFontList" type="@wxFontList">
 *   The one and only font list object.
 *  </property>
 *  <property name="wxTheColourDatabase" type="@wxColourDatabase">
 *   The one and only colour database.
 *  </property>
 * </properties>
 */
void wxjs::gui::DefineGlobals(JSContext *cx, JSObject *global)
{
    wxjs::gui::Size::DefineObject(cx, global, "wxDefaultSize", new wxSize(wxDefaultSize));
    wxjs::gui::FontList::DefineObject(cx, global, "wxTheFontList", wxTheFontList);
    wxjs::gui::ColourDatabase::DefineObject(cx, global, "wxTheColourDatabase", wxTheColourDatabase);

    wxjs::gui::DefineGlobalColours(cx, global);
}
