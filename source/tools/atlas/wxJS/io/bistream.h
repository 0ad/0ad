/*
 * wxJavaScript - bistream.h
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
 * $Id: bistream.h 715 2007-05-18 20:38:04Z fbraem $
 */
#ifndef wxjs_io_bistream_h
#define wxjs_io_bistream_h

namespace wxjs
{
    namespace io
    {
        class BufferedInputStream : public wxBufferedInputStream
                                  , public ApiWrapper<BufferedInputStream, Stream>
                                  , public wxClientDataContainer
        {
        public:
            BufferedInputStream(wxInputStream &s, wxStreamBuffer *buffer = NULL);
            virtual ~BufferedInputStream()
            {
            }
            static Stream* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            static void Destruct(JSContext *cx, Stream *p);

            Stream m_refStream;
        };
    }; // namespace io
}; // namespace wxjs
#endif // wxjs_io_bistream_h
