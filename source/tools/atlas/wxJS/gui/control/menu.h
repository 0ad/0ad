/*
 * wxJavaScript - menu.h
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
 * $Id: menu.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSMENU_H
#define _WXJSMENU_H

/////////////////////////////////////////////////////////////////////////////
// Name:        menu.h
// Purpose:		Menu ports wxMenu to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     16.12.01
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class Menu : public wxMenu
                       , public ApiWrapper<Menu, wxMenu>
			           , public Object
        {
        public:

	        /**
	         * Constructors
	         */
            Menu(JSContext *cx, JSObject *obj, const wxString& title, long style = 0);
            Menu(JSContext *cx, JSObject *obj, long style = 0);

	        virtual ~Menu();

	        static bool GetProperty(wxMenu *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxMenu *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        /**
	         * JSConstructor - Callback for when a wxMenu object is created
	         */
	        static wxMenu* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        static void Destruct(JSContext *cx, wxMenu *p);

	        ////////////////////////
	        // JavaScript methods
	        ////////////////////////

	        static JSBool append(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool append_separator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool new_column(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool check(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool delete_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool enable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool find_item(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool getHelpString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool getItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool getLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool destroy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool insert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool isChecked(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isEnabled(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setHelpString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        enum
	        {
		        P_MENU_ITEM_COUNT
		        , P_MENU_ITEMS
		        , P_TITLE
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSMENU_H

