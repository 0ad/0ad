#include "precompiled.h"

/*
 * wxJavaScript - zostream.cpp
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
 * $Id: zostream.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../ext/wxjs_ext.h"


#include "stream.h"
#include "ostream.h"
#include "zostream.h"
#include "zistream.h"
#include "zipentry.h"

using namespace wxjs;
using namespace wxjs::io;

ZipOutputStream::ZipOutputStream(wxOutputStream &str, int level) : wxZipOutputStream(str, level)
{
}

/***
 * <file>zostream</file>
 * <module>io</module>
 * <class name="wxZipOutputStream" protoype="@wxArchiveOutputStream" version="0.8.3">
 *  Output stream for writing zip files. The following sample shows how easy it is 
 *  to create a zip archive with files from one directory:
 *  <pre><code class="whjs">
 *   var zos = new wxZipOutputStream(new wxFileOutputStream("temp.zip"));
 *   zos.setComment("This archive is created as test");
 *   var dir = new wxDir("c:\\temp");
 *
 *   var trav = new wxDirTraverser();
 *   trav.onFile = function(filename)
 *   {
 *     var entry = new wxZipEntry(filename);
 *     zos.putNextEntry(entry);
 *     var fis = new wxFileInputStream(filename);
 *     var bfs = new wxBufferedOutputStream(zos);
 *     fis.read(bfs);
 *
 *     // Don't forget to sync (flush), otherwise you loose content
 *     bfs.sync();
 *
 *     return wxDirTraverser.CONTINUE;
 *   }
 *   dir.traverse(trav);
 *  </code></pre>
 *</class>
 */
WXJS_INIT_CLASS(ZipOutputStream, "wxZipOutputStream", 1)

/***
 * <ctor>
 *  <function>
 *   <arg name="Stream" type="@wxOutputStream">An Output stream</arg>
 *   <arg name="Level" type="Integer" default="-1">
 *    Level is the compression level to use. It can be a value between 0 and 9 
 *    or -1 to use the default value which currently is equivalent to 6.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxZipOutputStream object.
 *  </desc>
 * </ctor>
 */
Stream* ZipOutputStream::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( OutputStream::HasPrototype(cx, argv[0]) )
    {
        Stream *out = OutputStream::GetPrivate(cx, argv[0], false);

		int level = -1;
		if ( argc > 1 )
		{
			if ( ! FromJS(cx, argv[1], level) )
				return NULL;
		}

		// This is needed, because otherwise the stream can be garbage collected.
		// Another method could be to root the stream, but how are we going to unroot it?
		JS_DefineProperty(cx, obj, "__stream__", argv[0], NULL, NULL, JSPROP_READONLY);

		ZipOutputStream *stream = new ZipOutputStream(*(wxOutputStream*) out->GetStream(), level);
	    stream->m_refStream = *out;
		return new Stream(stream);
    }
    return NULL;
}

void ZipOutputStream::Destruct(JSContext *cx, Stream *p)
{
    if ( p != NULL )
    {
		ZipOutputStream *stream = (ZipOutputStream*) p->GetStream();
        stream->Close();

        // Keep stream alive for a moment, so that the base class
        // doesn't crash when it flushes the stream.
        Stream tempRefStream(stream->m_refStream);

        delete p;
        p = NULL;
    }
}

/***
 * <properties>
 *  <property name="comment" type="String" readonly="Y">
 *    The comment of the zip file
 *  </property>
 *  <property name="nextEntry" type="@wxZipEntry" readonly="Y">
 *   Closes the current entry if one is open, then reads the meta-data for the next 
 *   entry and returns it in a @wxZipEntry object. The stream is then open and can be read.
 *  </property>
 *  <property name="totalEntries" type="Integer" readonly="Y">
 *    The number of entries in the zip file
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ZipOutputStream)
	WXJS_PROPERTY(P_LEVEL, "level")
WXJS_END_PROPERTY_MAP()

bool ZipOutputStream::GetProperty(Stream *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	wxZipOutputStream *out = (wxZipOutputStream*) p->GetStream();
	switch(id)
	{
	case P_LEVEL:
		*vp = ToJS(cx, out->GetLevel());
		break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(ZipOutputStream)
	WXJS_METHOD("setComment", setComment, 1)
WXJS_END_METHOD_MAP()


/***
 * <method name="setComment">
 *  <function>
 *   <arg name="Comment" type="String" />
 *  </function>
 *  <desc>
 *   Sets the comment on the archive
 *  </desc>
 * </method>
 */
JSBool ZipOutputStream::setComment(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Stream *p = ZipOutputStream::GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	wxZipOutputStream *zos = (wxZipOutputStream *) p->GetStream();

	wxString comment;
	FromJS(cx, argv[0], comment);
	zos->SetComment(comment);

	return JS_TRUE;
}
