/*
 * wxJavaScript - dir.h
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
 * $Id: dir.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSDir_H
#define _WXJSDir_H

/////////////////////////////////////////////////////////////////////////////
// Name:        dir.h
// Purpose:     wxJSDir ports wxDir to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     27.09.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/dir.h>

namespace wxjs
{
    namespace io
    {

        class Dir : public ApiWrapper<Dir, wxDir>
        {
        public:
            /**
             * Callback for retrieving properties of wxDir
             */
            static bool GetProperty(wxDir *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxDir object is created
             */
            static wxDir* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            static JSBool open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getFirst(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool hasFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool hasSubDirs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool traverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getAllFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_CONSTANT_MAP()
            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_STATIC_METHOD_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_OPENED,
		        P_NAME
            };
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSDir_H

