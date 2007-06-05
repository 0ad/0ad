/*
 * wxJavaScript - menuitem.h
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
 * $Id: menuitem.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSMENUITEM_H
#define _WXJSMENUITEM_H

/////////////////////////////////////////////////////////////////////////////
// Name:        menuitem.h
// Purpose:		MenuItem ports wxMenuItem to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     19.12.01
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    namespace gui
    {
        class MenuItem : public ApiWrapper<MenuItem, wxMenuItem>
        {
        public:
	        static bool GetProperty(wxMenuItem *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxMenuItem *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static wxMenuItem* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            static void Destruct(JSContext *cx, wxMenuItem *p);

	        static JSBool is_checkable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool is_separator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setBitmaps(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()

	        enum
	        {
		        P_LABEL = WXJS_START_PROPERTY_ID
		        , P_TEXT
		        , P_CHECK
		        , P_CHECKABLE
		        , P_ENABLE
		        , P_HELP
		        , P_ID
		        , P_MARGIN_WIDTH
		        , P_SUB_MENU
		        , P_ACCEL
		        , P_BG_COLOUR
		        , P_MENU
		        , P_FONT
		        , P_BITMAP
		        , P_TEXT_COLOUR
		        , P_SEPARATOR
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSMENUITEM_H
