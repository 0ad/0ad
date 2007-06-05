/*
 * wxJavaScript - control.h
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
 * $Id: control.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSControl_H
#define _WXJSControl_H

/////////////////////////////////////////////////////////////////////////////
// Name:        control.h
// Purpose:     Control ports wxControl to JavaScript
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
        class Control : public ApiWrapper<Control, wxControl>
        {
        public:
            /**
             * Callback for retrieving properties of wxControl
             */
            static bool GetProperty(wxControl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxControl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxControl object is destroyed
             */
            static void Destruct(JSContext *cx, wxControl *p);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_METHOD_MAP()
            static JSBool command(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            /**
             * Property Ids.
             */
            enum
            {
                P_LABEL
            };
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSControl_H
