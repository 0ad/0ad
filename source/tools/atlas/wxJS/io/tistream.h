/*
 * wxJavaScript - tistream.h
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
 * $Id: tistream.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSTextInputStream_H
#define _WXJSTextInputStream_H

#include <wx/txtstrm.h>

namespace wxjs
{
    namespace io
    {
        class TextInputStream: public ApiWrapper<TextInputStream, TextInputStream>,
                               public wxTextInputStream
        {
        public:
            TextInputStream(wxInputStream &stream, const wxString &sep, wxMBConv& conv);
            /**
             * Callback for when a wxTextInputStream object is created
             */
            static TextInputStream* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            static bool GetProperty(TextInputStream *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            static bool SetProperty(TextInputStream *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_STRING_SEPARATORS
            };

            WXJS_DECLARE_METHOD_MAP()
            static JSBool read32(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool read16(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool read8(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool readDouble(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool readLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool readWord(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSTextInputStream_H
