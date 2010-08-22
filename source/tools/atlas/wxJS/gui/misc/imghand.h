/*
 * wxJavaScript - imghand.h
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
 * $Id: imghand.h 734 2007-06-06 20:09:13Z fbraem $
 */
#ifndef _WXJSImageHandler_H
#define _WXJSImageHandler_H

#include <wx/image.h>

namespace wxjs
{
    namespace gui
    {
        // A class that handles the destruction of an imagehandler.
        // Ounce the handler is added to the image handler list, wxWindows destroys
        // the handler for us. So when the handler is not in a list, wxJS must destroy 
        // the handler.
        class ImageHandlerPrivate
        {
        public:
            ImageHandlerPrivate(wxImageHandler *handler, bool ours) : m_handler(handler), m_ours(ours)
            {
            }

            ~ImageHandlerPrivate()
            {
                if ( IsOurs() )
                    delete m_handler;
            }

            bool IsOurs() const
            {
                return m_ours;
            }

            void SetOurs(bool ours)
            {
                m_ours = ours;
            }

            wxImageHandler* GetHandler() const
            {
                return m_handler;
            }

        private:
            wxImageHandler *m_handler;
            bool m_ours;
        };

        class ImageHandler : public ApiWrapper<ImageHandler, ImageHandlerPrivate>
        {
        public:

            /**
             * Callback for retrieving properties of wxImageHandler
             */
            static bool GetProperty(ImageHandlerPrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(ImageHandlerPrivate *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_NAME
                , P_EXTENSION
                , P_TYPE
                , P_MIME_TYPE
            };

            WXJS_DECLARE_METHOD_MAP()
            static JSBool getImageCount(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool loadFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool saveFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };

        class BMPHandler : public ApiWrapper<BMPHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class ICOHandler : public ApiWrapper<ICOHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class GIFHandler : public ApiWrapper<GIFHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class JPEGHandler : public ApiWrapper<JPEGHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class PCXHandler : public ApiWrapper<PCXHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class PNGHandler : public ApiWrapper<PNGHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

        class PNMHandler : public ApiWrapper<PNMHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };

#if wxUSE_LIBTIFF
        class TIFFHandler : public ApiWrapper<TIFFHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };
#endif

        class XPMHandler : public ApiWrapper<XPMHandler, ImageHandlerPrivate>
        {
        public:
            static ImageHandlerPrivate* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
        };
    }; // namespace gui
}; // namespace wxjs
#endif //_WXJSImageHandler_H
