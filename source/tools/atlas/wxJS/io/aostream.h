/*
 * wxJavaScript - aostream.h
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
 * $Id: aostream.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef wxjs_io_aostream_h
#define wxjs_io_aostream_h

namespace wxjs
{
    namespace io
    {
        class ArchiveOutputStream : public ApiWrapper<ArchiveOutputStream, Stream>
        {
        public:

            WXJS_DECLARE_METHOD_MAP()
            static JSBool closeentry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool putNextDirEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool copyArchiveMetaData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool putNextEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool copyEntry(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
        };
    }; // namespace io
}; // namespace wxjs
#endif // wxjs_io_aostream_h
