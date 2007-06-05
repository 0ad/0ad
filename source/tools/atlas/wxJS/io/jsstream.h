/*
 * wxJavaScript - jsstream.h
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
 * $Id: jsstream.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxjs_io_jsstream_h
#define _wxjs_io_jsstream_h

#include <wx/stream.h>

namespace wxjs
{
    namespace io
    {

        // Reference counting for stream classes.
        // Why? because some streams are members of other streams. When
        // these streams are gc'ed by SpiderMonkey before the stream that owns a stream,
        // access violations can be the result. This is the case in wxBufferedOutputStream
        // where the stream is flushed in the destructor.
        class StreamRef : public wxObjectRefData
        {
        public:
            StreamRef(wxStreamBase *stream) : wxObjectRefData(), m_stream(stream)
            {
            }
            virtual ~StreamRef()
            {
                delete m_stream;
            }

            wxStreamBase *GetStream()
            {
                return m_stream;
            }
        private:
            wxStreamBase *m_stream;
        };

        // This class will be the private data for each stream JavaScript object.
        class Stream : public wxObject
        {
        public:
	        inline Stream(bool owner = true) : m_owner(owner), m_obj(NULL)
            {
            }

	        inline Stream(wxStreamBase *stream, bool owner = true) : m_owner(owner), m_obj(NULL)
            {
                m_refData = new StreamRef(stream);
            }
            
            inline Stream(const Stream& stream) : wxObject(stream)
            {
                Ref(stream);
		        m_owner = stream.m_owner;
		        m_obj = stream.m_obj;
            }

            inline bool operator == (const Stream& stream) const 
            { 
                return m_refData == stream.m_refData; 
            }
            inline bool operator != (const Stream& stream) const 
            { 
                return m_refData != stream.m_refData; 
            }

            inline Stream& operator = (const Stream& stream)
            {
                if ( *this == stream )
                    return (*this);
                else
                {
                    Ref(stream);
                    return *this;
                }
            }

	        inline void SetObject(JSObject *obj)
	        {
		        m_obj = obj;
	        }

	        inline JSObject *GetObject()
	        {
		        return m_obj;
	        }

            virtual ~Stream()
            {
            }

            wxStreamBase* GetStream()
            {
                return ((StreamRef *) m_refData)->GetStream();
            }

	        // Do we own the stream (=default)? Or is another class
	        // responsible for destroying me? (like wxHTTP for example)
	        bool IsOwner() const { return m_owner; }

        private:
	        bool m_owner;

	        JSObject *m_obj;
        };
    }; // namespace io
}; // namespace wxjs
#endif // _wxjs_io_jsstream_h
