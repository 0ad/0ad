/*
 * wxJavaScript - mouse.h
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
 * $Id: mouse.h 598 2007-03-07 20:13:28Z fbraem $
 */
/////////////////////////////////////////////////////////////////////////////
// Name:        mouse.h
// Purpose:     MouseEvent ports wxMouseEvent to JavaScript.
// Author:      Franky Braem
// Modified by:
// Created:     01.07.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXJSMouseEvent_H
#define _WXJSMouseEvent_H

namespace wxjs
{
    namespace gui
    {
        typedef JSEvent<wxMouseEvent> PrivMouseEvent;

        class MouseEvent : public ApiWrapper<MouseEvent, PrivMouseEvent>
        {
        public:
            virtual ~MouseEvent()
            {
            }

            static bool GetProperty(PrivMouseEvent *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            // Property ID's
            enum
            {
	            P_ALTDOWN
	            , P_CONTROLDOWN
	            , P_DRAGGING
	            , P_ENTERING
	            , P_POSITION
	            , P_LINES_PER_ACTION
	            , P_BUTTON
	            , P_METADOWN
	            , P_SHIFTDOWN
	            , P_LEFT_DOWN
	            , P_MIDDLE_DOWN
	            , P_RIGHT_DOWN
	            , P_LEFT_UP
	            , P_MIDDLE_UP
	            , P_RIGHT_UP
	            , P_LEFT_DCLICK
	            , P_MIDDLE_DCLICK
	            , P_RIGHT_DCLICK
	            , P_LEFT_IS_DOWN
	            , P_MIDDLE_IS_DOWN
	            , P_RIGHT_IS_DOWN
	            , P_MOVING
	            , P_LEAVING
	            , P_X
	            , P_Y
	            , P_WHEELROTATION
	            , P_WHEELDELTA
            };

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_METHOD_MAP()

            static JSBool button(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool buttonDClick(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool buttonDown(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool buttonUp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSMouseEvent_H
