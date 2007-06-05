/*
 * wxJavaScript - zostream.h
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
 * $Id: zostream.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef wxjs_io_zostream_h
#define wxjs_io_zostream_h

#include <wx/zipstrm.h>

namespace wxjs
{
    namespace io
    {
        class ZipOutputStream : public wxZipOutputStream,
						            public ApiWrapper<ZipOutputStream, Stream>
        {
        public:

	        ZipOutputStream(wxOutputStream &str, int level);

	        static Stream* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void Destruct(JSContext *cx, Stream *p);

	        /**
             * Callback for retrieving properties of wxInputStream
             */
            static bool GetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_LEVEL
            };

            WXJS_DECLARE_METHOD_MAP()
            static JSBool setComment(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

	        // Keep a reference to the stream to avoid deletion.
            Stream m_refStream;
        };
    }; // namespace io
}; // namespace wxjs
#endif // wxjs_io_zostream_h
