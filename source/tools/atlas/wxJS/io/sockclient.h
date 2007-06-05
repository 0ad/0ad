/*
 * wxJavaScript - sockclient.h
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
 * $Id: sockclient.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxjs_io_sockclient_h
#define _wxjs_io_sockclient_h

#include <wx/event.h>
#include <wx/socket.h>

#include "sockbase.h"

namespace wxjs
{
    namespace io
    {
        class SocketEventHandler;

        class SocketClient : public ApiWrapper<SocketClient, SocketBasePrivate>
					           , public wxSocketClient
        {
        public:
	        SocketClient(JSContext *cx, JSObject *obj, wxSocketFlags flags = wxSOCKET_NONE);
	        virtual ~SocketClient();

	        static SocketBasePrivate *Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
	        static void Destruct(JSContext *cx, SocketBasePrivate *p);

	        WXJS_DECLARE_METHOD_MAP()

	        static JSBool connect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
	        static JSBool waitOnConnect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

        private:
	        SocketEventHandler *m_evtHandler;
        };
    }; // namespace io
}; // namespace wxjs
#endif // _wxjs_io_sockclient_h
