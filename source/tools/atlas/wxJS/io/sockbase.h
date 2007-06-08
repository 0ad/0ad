/*
 * wxJavaScript - sockbase.h
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
 * $Id: sockbase.h 716 2007-05-20 17:57:22Z fbraem $
 */
#ifndef _wxjs_io_sockbase_h
#define _wxjs_io_sockbase_h

#include <wx/socket.h>

#include <vector>

#include "jsstream.h"

namespace wxjs
{
    namespace io
    {
        class SocketBasePrivate
        {
        public:
	        SocketBasePrivate(wxSocketBase *base) : m_base(base)
	        {
	        }

	        virtual ~SocketBasePrivate()
	        {
	        }

	        void DestroyStreams(JSContext *cx)
	        {
		        for(std::vector<Stream *>::iterator it = m_streams.begin(); it != m_streams.end(); it++)
		        {
			        JSObject *obj = (*it)->GetObject();
			        if ( obj != NULL )
				        JS_SetPrivate(cx, obj, NULL); // To avoid deletion
			        delete *it;
		        }
	        }

	        wxSocketBase* GetBase() { return m_base; }

	        void AddStream(Stream *stream) 
	        {
		        m_streams.push_back(stream);
	        }

        private:
	        wxSocketBase *m_base;
	        std::vector<Stream *> m_streams;
        };

        class SocketBase : public ApiWrapper<SocketBase, SocketBasePrivate>
        {
        public:
	        static bool GetProperty(SocketBasePrivate *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

	        static void InitClass(JSContext *cx,
                                  JSObject *obj,
                                  JSObject *proto);

	        WXJS_DECLARE_PROPERTY_MAP()
	        enum
	        {
		        P_ERROR = WXJS_START_PROPERTY_ID
		        , P_CONNECTED 
		        , P_DATA
		        , P_DISCONNECTED 
		        , P_LASTCOUNT
		        , P_LASTERROR
		        , P_OK
	        };
        	
	        WXJS_DECLARE_METHOD_MAP()

	        WXJS_DECLARE_METHOD(close)
	        WXJS_DECLARE_METHOD(discard)
	        WXJS_DECLARE_METHOD(interruptWait)
	        WXJS_DECLARE_METHOD(notify)
	        WXJS_DECLARE_METHOD(peek)
	        WXJS_DECLARE_METHOD(read)
	        WXJS_DECLARE_METHOD(readMsg)
	        WXJS_DECLARE_METHOD(restoreState)
	        WXJS_DECLARE_METHOD(saveState)
	        WXJS_DECLARE_METHOD(setTimeout)
	        WXJS_DECLARE_METHOD(unread)
	        WXJS_DECLARE_METHOD(wait)
	        WXJS_DECLARE_METHOD(waitForLost)
	        WXJS_DECLARE_METHOD(waitForRead)
	        WXJS_DECLARE_METHOD(waitForWrite)
	        WXJS_DECLARE_METHOD(write)
	        WXJS_DECLARE_METHOD(writeMsg)
        };
    }; // namespace io
}; // namespace wxjs
#endif // _wxjs_io_sockbase_h
