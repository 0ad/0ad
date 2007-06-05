#include "precompiled.h"

/*
 * wxJavaScript - bostream.cpp
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
 * $Id: bostream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "stream.h"
#include "ostream.h"
#include "bostream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>bostream</file>
 * <module>io</module>
 * <class name="wxBufferedOutputStream" prototype="@wxOutputStream" version="0.8.2">
 *  This stream acts as a cache. It caches the bytes to be written
 *  to the specified output stream. 
 * </class>
 */
BufferedOutputStream::BufferedOutputStream(JSContext *cx
												   , JSObject *obj
                                                   , wxOutputStream &s
                                                   , wxStreamBuffer *buffer)
                            : wxBufferedOutputStream(s, buffer)
                            , Object(obj, cx)
{
}
 
WXJS_INIT_CLASS(BufferedOutputStream, "wxBufferedOutputStream", 1)

/***
 * <ctor>
 *  <function>
 *   <arg name="Output" type="@wxOutputStream" />
 *  </function>
 *  <desc>
 *   Creates a new wxBufferedOutputStream object.
 *  </desc>
 * </ctor>
 */
Stream* BufferedOutputStream::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if (    argc == 0 
         || argc  > 2 )
        return NULL;

    Stream *out = OutputStream::GetPrivate(cx, argv[0]);
    if ( out == NULL )
        return NULL;

	// This is needed, because otherwise the stream can be garbage collected.
	// Another method could be to root the stream, but how are we going to unroot it?
	JS_DefineProperty(cx, obj, "__stream__", argv[0], NULL, NULL, JSPROP_READONLY);

    wxStreamBuffer *buffer = NULL;

	BufferedOutputStream *stream = new BufferedOutputStream(cx, obj, *((wxOutputStream*)(out->GetStream())), buffer);
    stream->m_refStream = *out;

    return new Stream(stream);
}

void BufferedOutputStream::Destruct(JSContext *cx, Stream *p)
{
    if ( p != NULL )
    {
        BufferedOutputStream *stream = (BufferedOutputStream*) p->GetStream();

        // Keep stream alive for a moment, so that the base class wxBufferedOutputStream 
        // doesn't crash when it flushes the stream.
        Stream tempRefStream(stream->m_refStream);

        delete p;
        p = NULL;
    }
}

