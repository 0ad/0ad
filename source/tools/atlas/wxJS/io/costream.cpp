#include "precompiled.h"

/*
 * wxJavaScript - costream.cpp
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
 * $Id: costream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// stream.cpp

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"

#include "stream.h"
#include "costream.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>costream</file>
 * <module>io</module>
 * <class name="wxCountingOutputStream" prototype="@wxOutputStream" version="0.8.2">
 *  wxCountingOutputStream is a specialized output stream which does not write any data anyway,
 *  instead it counts how many bytes would get written if this were a normal stream. 
 *  This can sometimes be useful or required if some data gets serialized to a stream or 
 *  compressed by using stream compression and thus the final size of the stream cannot be 
 *  known other than pretending to write the stream. One case where the resulting size would 
 *  have to be known is if the data has to be written to a piece of memory and the memory has 
 *  to be allocated before writing to it (which is probably always the case when writing to a 
 *  memory stream).
 * </class>
 */

WXJS_INIT_CLASS(CountingOutputStream, "wxCountingOutputStream", 0)

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Creates a new wxCountingOutputStream object.
 *  </desc>
 * </ctor>
 */
Stream* CountingOutputStream::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new Stream(new wxCountingOutputStream());
}
