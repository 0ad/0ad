/*
 * wxJavaScript - imagelst.h
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
 * $Id: imagelst.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSImageList_H
#define _WXJSImageList_H

/////////////////////////////////////////////////////////////////////////////
// Name:        imagelst.h
// Purpose:     ImageList ports wxImageList to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     06-10-2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/imaglist.h>

namespace wxjs
{
    namespace gui
    {
        class ImageList : public wxImageList
                            , public ApiWrapper<ImageList, wxImageList>
                            , public Object
        {
        public:

            ImageList(JSContext *cx, JSObject *obj);

            virtual ~ImageList()
            {
            }

            /**
             * Callback for retrieving properties of wxImageList
             */
            static bool GetProperty(wxImageList *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxImageList object is created
             */
            static wxImageList* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            WXJS_DECLARE_PROPERTY_MAP()
            WXJS_DECLARE_METHOD_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_IMAGE_COUNT
            };

            static JSBool add(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool draw(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getSize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool removeAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSImageList_H

