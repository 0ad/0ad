/*
 * wxJavaScript - window.h
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
 * $Id: window.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSWINDOW_H
#define _WXJSWINDOW_H

/////////////////////////////////////////////////////////////////////////////
// Name:        window.h
// Purpose:		Window ports wxWindow to JavaScript.
//              wxWindow is used as a prototype object
//              of several other wxWindow JavaScript objects.
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
        class Window : public ApiWrapper<Window, wxWindow>
        {
        public:

	        static bool GetProperty(wxWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
	        static bool SetProperty(wxWindow *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        /**
	         * Callback for retrieving static properties
	         */
	        static bool GetStaticProperty(JSContext *cx, int id, jsval *vp);

	        static JSBool captureMouse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool centre(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clearBackground(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool clientToScreen(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool move(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool convertDialogToPixels(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool convertPixelsToDialog(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool destroy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool releaseMouse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool layout(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool raise(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool lower(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool centreOnParent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool fit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setSizeHints(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool show(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool refresh(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool setFocus(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool findFocus(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool findWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool initDialog(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool transferDataToWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool transferDataFromWindow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool validate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool makeModal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool warpPointer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool update(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool freeze(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool thaw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        WXJS_DECLARE_PROPERTY_MAP()
	        WXJS_DECLARE_METHOD_MAP()
	        WXJS_DECLARE_STATIC_METHOD_MAP()
	        WXJS_DECLARE_STATIC_PROPERTY_MAP()
	        WXJS_DECLARE_CONSTANT_MAP()

	        enum
	        {
		        P_CLIENT_HEIGHT = -128
		        , P_AUTO_LAYOUT
		        , P_CLIENT_WIDTH
		        , P_CLIENT_ORIGIN
		        , P_ENABLE
		        , P_HEIGHT
		        , P_TITLE 
		        , P_VISIBLE
		        , P_WIDTH
		        , P_POSITION
		        , P_SIZE
		        , P_SIZER
		        , P_CLIENT_SIZE
		        , P_ID
		        , P_RECT
		        , P_CLIENT_RECT
		        , P_BEST_SIZE
		        , P_WINDOW_STYLE
		        , P_EXTRA_STYLE
		        , P_SHOWN
		        , P_TOP_LEVEL
		        , P_ACCEPTS_FOCUS
		        , P_ACCEPTS_FOCUS_KEYBOARD
		        , P_DEFAULT_ITEM
		        , P_CHILDREN
		        , P_PARENT
		        , P_VALIDATOR
		        , P_ACCELERATOR_TABLE
		        , P_CAPTURE
		        , P_HAS_CAPTURE
		        , P_UPDATE_REGION
		        , P_BACKGROUND_COLOUR
		        , P_FOREGROUND_COLOUR
		        , P_FONT
	        };
        };
    }; // namespace gui
}; // namespace wxjs

#endif // _WXJSWINDOW_H
