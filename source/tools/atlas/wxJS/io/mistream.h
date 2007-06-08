/*
 * wxJavaScript - mistream.h
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
 * $Id: mistream.h 715 2007-05-18 20:38:04Z fbraem $
 */
#ifndef _WXJSMemoryInputStream_H
#define _WXJSMemoryInputStream_H

/////////////////////////////////////////////////////////////////////////////
// Name:        .h
// Purpose:     Ports the memory streams
//              wxJSMemoryInputStream ports wxMemoryInputStream to JavaScript
//              wxJSMemoryOutputStream ports wxMemoryOutputStream to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     22-10-2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/mstream.h>

namespace wxjs
{
    namespace io
    {
        class MemoryInputStream : public wxMemoryInputStream
                                , public ApiWrapper<MemoryInputStream, Stream>
                                , public wxClientDataContainer
        {
        public:
            MemoryInputStream(char *data, size_t len);
            virtual ~MemoryInputStream();

            /**
             * Callback for when a wxMemoryInputStream object is created
             */
            static Stream* Construct(JSContext *cx,
                                     JSObject *obj,
                                     uintN argc,
                                     jsval *argv,
                                     bool constructing);

        private:
            char *m_data;
        };
    }; // namespace io
}; // namespace wxjs
#endif //_WXJSMemoryInputStream_H
