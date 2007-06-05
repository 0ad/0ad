/*
 * wxJavaScript - zipentry.h
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
 * $Id: zipentry.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef wxjs_io_zipentry_h
#define wxjs_io_zipentry_h

#include <wx/zipstrm.h>

namespace wxjs
{
    namespace io
    {
        class ZipEntry : public ApiWrapper<ZipEntry, wxZipEntry>
        {
        public:

	        static wxZipEntry* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

	        /**
             * Callback for retrieving properties
             */
            static bool GetProperty(wxZipEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp);
            static bool SetProperty(wxZipEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

	        static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
		        P_COMMENT
		        , P_COMPRESSED_SIZE
		        , P_CRC
		        , P_EXTERNAL_ATTR
		        , P_EXTRA
		        , P_FLAGS
		        , P_LOCAL_EXTRA
		        , P_MODE
		        , P_METHOD
		        , P_SYSTEM_MADE_BY
		        , P_MADE_BY_UNIX
		        , P_TEXT
            };
        };
    }; // namespace io
}; // namespace wxjs
#endif // wxjs_io_zipentry_h
