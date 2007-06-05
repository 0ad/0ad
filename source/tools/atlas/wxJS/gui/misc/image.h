/*
 * wxJavaScript - image.h
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
 * $Id: image.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSImage_H
#define _WXJSImage_H

/////////////////////////////////////////////////////////////////////////////
// Name:        image.h
// Purpose:     Image ports wxImage to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     07-10-2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/image.h>

namespace wxjs
{
    namespace gui
    {
        class Image : public ApiWrapper<Image, wxImage>
        {
        public:
            /**
             * Callback for retrieving properties of wxImage
             */
            static bool GetProperty(wxImage *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxImage *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxImage object is created
             */
            static wxImage* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_OK
                , P_MASK_RED
                , P_MASK_GREEN
                , P_MASK_BLUE
                , P_WIDTH
                , P_HEIGHT
                , P_MASK
                , P_HAS_PALETTE
                , P_HAS_MASK
                , P_PALETTE
                , P_SIZE
                , P_HANDLERS
            };

            WXJS_DECLARE_STATIC_PROPERTY_MAP()
            bool GetStaticProperty(JSContext *cx, int id, jsval *vp);

            WXJS_DECLARE_METHOD_MAP()
            static JSBool create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool destroy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool copy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getSubImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool paste(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool scale(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool rescale(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool rotate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool rotate90(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool mirror(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool replace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool convertToMono(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setRGB(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getRed(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getGreen(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getBlue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findFirstUnusedColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setMaskFromImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setMaskColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getOptionInt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool hasOption(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_STATIC_METHOD_MAP()
            static JSBool canRead(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool cleanUpHandlers(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool removeHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findHandlerMime(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertHandler(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSImage_H
