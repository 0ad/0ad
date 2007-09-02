#include "precompiled.h"

/*
 * wxJavaScript - mistream.cpp
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
 * $Id: mistream.cpp 759 2007-06-12 21:13:52Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"

#include "stream.h"
#include "mistream.h"

#include "../ext/wxjs_ext.h"
#include "../ext/jsmembuf.h"

using namespace wxjs;
using namespace wxjs::io;

MemoryInputStream::MemoryInputStream(char *data
									 , size_t len) : wxMemoryInputStream(data, len)
                                                   , m_data(data)
{
}

MemoryInputStream::~MemoryInputStream()
{
    delete[] m_data;
}

/***
 * <file>mistream</file>
 * <module>io</module>
 * <class name="wxMemoryInputStream" version="0.8.3" prototype="@wxInputStream">
 *  wxMemoryInputStream allows an application to create an input stream
 *  in which the bytes read are supplied by a memory buffer or a string.
 *  See also @wxMemoryOutputStream.
 * </class>
 */
WXJS_INIT_CLASS(MemoryInputStream, "wxMemoryInputStream", 1)

/***
 * <ctor>
 *  <function>
 *   <arg name="Data" type="String" />
 *  </function>
 *  <function>
 *   <arg name="Buffer" type="@wxMemoryBuffer" />
 *  </function>
 *  <desc>
 *   Constructs a new wxMemoryInputStream object. A copy is created
 *   of the data and passed to the stream.
 *   <br /><br />
 *   When a String is used, the string is stored as UTF-16!
 *  </desc>
 * </ctor>
 */
Stream* MemoryInputStream::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	if ( JSVAL_IS_OBJECT(argv[0]) )
	{
		wxMemoryBuffer* buffer = wxjs::ext::GetMemoryBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
		if ( buffer != NULL )
		{
			char *dataPtr = new char[buffer->GetDataLen()];
			memcpy(dataPtr, buffer->GetData(), buffer->GetDataLen());
            MemoryInputStream *mis = new MemoryInputStream(dataPtr, (size_t) buffer->GetDataLen());
            mis->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
			return new Stream(mis);
		}		
	}

	wxString data;
	FromJS(cx, argv[0], data);

    wxMBConvUTF16 utf16;
    int length = utf16.WC2MB(NULL, data, 0);
    if ( length > 0 )
    {
        char *buffer = new char[length + utf16.GetMBNulLen()];
        length = utf16.WC2MB(buffer, data, length + utf16.GetMBNulLen());
        MemoryInputStream *mis = new MemoryInputStream(buffer, length);
        mis->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
    	return new Stream(mis);
    }
    return NULL;
}
