/*
 * wxJavaScript - statbar.h
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
 * $Id: statbar.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSStatusBar_H
#define _WXJSStatusBar_H

/////////////////////////////////////////////////////////////////////////////
// Name:        statbar.h
// Purpose:     StatusBar ports wxStatusBar to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     31-01-2003
// Copyright:   (c) 2001-2003 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////
namespace wxjs
{
    namespace gui
    {
        class StatusBar : public wxStatusBar
                            , public ApiWrapper<StatusBar, wxStatusBar>
                            , public Object
        {
        public:

            StatusBar(JSContext *cx, JSObject *obj);
            virtual ~StatusBar();

            /**
             * Callback for retrieving properties of wxStatusBar
             */
            static bool GetProperty(wxStatusBar *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxStatusBar *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxStatusBar object is created
             */
            static wxStatusBar* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            /**
             * Callback for when a wxStatusBar object is destroyed
             */
            static void Destruct(JSContext *cx, wxStatusBar *p);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_CONSTANT_MAP()

            WXJS_DECLARE_METHOD_MAP()
            static JSBool getFieldRect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setFieldsCount(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setStatusText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getStatusText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            /**
             * Property Ids.
             */
            enum
            {
                P_FIELDS_COUNT
                , P_STATUS_WIDTHS
            };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSStatusBar_H
