/*
 * wxJavaScript - tostream.h
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
 * $Id: tostream.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSTextOutputStream_H
#define _WXJSTextOutputStream_H

#include <wx/txtstrm.h>

namespace wxjs
{
    namespace io
    {
        class TextOutputStream : public ApiWrapper<TextOutputStream, wxTextOutputStream>
        {
        public:

            /**
             * Callback for when a wxTextOutputStream object is created
             */
            static wxTextOutputStream* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

            static bool GetProperty(wxTextOutputStream *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            static bool SetProperty(wxTextOutputStream *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_MODE
            };
            WXJS_DECLARE_METHOD_MAP()
            static JSBool write32(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool write16(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool write8(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool writeDouble(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool writeString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSTextOutputStream_H
