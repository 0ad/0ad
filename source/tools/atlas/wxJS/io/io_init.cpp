#include "precompiled.h"

/*
 * wxJavaScript - init.cpp
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
 * $Id: init.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// main.cpp
#include <wx/setup.h>

#include "../common/main.h"
#include "init.h"
#include "../common/index.h"
#include "../common/jsutil.h"

#include "constant.h"
#include "file.h"
#include "ffile.h"
#include "dir.h"
#include "dirtrav.h"
#include "stream.h"
#include "istream.h"
#include "ostream.h"
#include "fistream.h"
#include "fostream.h"
#include "ffistream.h"
#include "ffostream.h"
#include "costream.h"
#include "mistream.h"
#include "mostream.h"
#include "bostream.h"
#include "bistream.h"
#include "tistream.h"
#include "tostream.h"
#include "filename.h"
#include "aistream.h"
#include "archentry.h"
#include "zipentry.h"
#include "zistream.h"
#include "aostream.h"
#include "zostream.h"
#include "tempfile.h"
#include "textfile.h"
#include "textline.h"
#include "distream.h"
#include "dostream.h"
#include "sockaddr.h"
#include "sockbase.h"
#include "sockclient.h"
#include "socksrv.h"
#include "sostream.h"
#include "sistream.h"
#include "ipaddr.h"
#include "ipv4addr.h"
#include "protocol.h"
#include "uri.h"
#include "url.h"
#include "http.h"
#include "httphdr.h"
#include "sound.h"
#include "fn.h"
#include "process.h"

using namespace wxjs;

bool io::InitClass(JSContext *cx, JSObject *global)
{
    InitConstants(cx, global);

    JSObject *obj = File::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxFile prototype creation failed"));
	if (! obj )
		return false;

	obj = FFile::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxFFile prototype creation failed"));
	if (! obj )
		return false;

    obj = TempFile::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxTempFile prototype creation failed"));
	if (! obj )
		return false;

	obj = Dir::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxDir prototype creation failed"));
	if (! obj )
		return false;

	obj = DirTraverser::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxDirTraverser prototype creation failed"));
	if (! obj )
		return false;

	obj = StreamBase::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxStreamBase prototype creation failed"));
	if (! obj )
		return false;

	obj = InputStream::JSInit(cx, global, StreamBase::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = BufferedInputStream::JSInit(cx, global, InputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxBufferedInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = OutputStream::JSInit(cx, global, StreamBase::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = BufferedOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxBufferedOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = CountingOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxCountingOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = FileInputStream::JSInit(cx, global, InputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFileInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = FileOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxFileOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = MemoryInputStream::JSInit(cx, global, InputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMemoryInputStream prototype creation failed"));
	if (! obj )
		return false;

    obj = MemoryOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxMemoryOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = TextInputStream::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTextInputStream prototype creation failed"));
	if (! obj )
		return false;

    obj = TextOutputStream::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTextOutputStream prototype creation failed"));
	if (! obj )
		return false;

    obj = FileName::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxFileName prototype creation failed"));
	if (! obj )
		return false;

	obj = ArchiveInputStream::JSInit(cx, global, InputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxArchiveInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = ArchiveOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxArchiveOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = ArchiveEntry::JSInit(cx, global, NULL);
	wxASSERT_MSG(obj != NULL, wxT("wxArchiveEntry prototype creation failed"));
	if (! obj )
		return false;

	obj = ZipEntry::JSInit(cx, global, ArchiveEntry::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxZipEntry prototype creation failed"));
	if (! obj )
		return false;

	obj = ZipInputStream::JSInit(cx, global, ArchiveInputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxZipInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = ZipOutputStream::JSInit(cx, global, ArchiveOutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxZipOutputStream"));
	if (! obj )
		return false;

	obj = TextFile::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTextFile"));
	if (! obj )
		return false;

	obj = TextLine::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxTextLine"));
	if (! obj )
		return false;

	obj = DataInputStream::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxDataInputStream prototype creation failed"));
	if (! obj )
		return false;

    obj = DataOutputStream::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxDataOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = SocketBase::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxSocketBase prototype creation failed"));
	if (! obj )
		return false;

	obj = SockAddress::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxSocketAddr prototype creation failed"));
	if (! obj )
		return false;

	obj = SocketClient::JSInit(cx, global, SocketBase::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSocketClient prototype creation failed"));
	if (! obj )
		return false;

	obj = SocketServer::JSInit(cx, global, SocketBase::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSocketServer prototype creation failed"));
	if (! obj )
		return false;

	obj = SocketInputStream::JSInit(cx, global, InputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSocketInputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = SocketOutputStream::JSInit(cx, global, OutputStream::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxSocketOutputStream prototype creation failed"));
	if (! obj )
		return false;

	obj = IPaddress::JSInit(cx, global, SockAddress::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxIPaddress prototype creation failed"));
	if (! obj )
		return false;

	obj = IPV4address::JSInit(cx, global, IPaddress::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxIPV4address prototype creation failed"));
	if (! obj )
		return false;

	obj = Protocol::JSInit(cx, global, SocketClient::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxProtocol prototype creation failed"));
	if (! obj )
		return false;

	obj = URI::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxURI prototype creation failed"));
	if (! obj )
		return false;

	obj = HTTP::JSInit(cx, global, Protocol::GetClassPrototype());
	wxASSERT_MSG(obj != NULL, wxT("wxHTTP prototype creation failed"));
	if (! obj )
		return false;

	obj = HTTPHeader::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxHTTPHeader prototype creation failed"));
	if (! obj )
		return false;

	obj = URL::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxURL prototype creation failed"));
	if (! obj )
		return false;

	obj = Sound::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxSound prototype creation failed"));
	if (! obj )
		return false;

    obj = Process::JSInit(cx, global);
	wxASSERT_MSG(obj != NULL, wxT("wxProcess prototype creation failed"));
	if (! obj )
		return false;

	JS_DefineFunction(cx, global, "wxConcatFiles", methodInterfaceWrapper<concatFiles>, 3, 0);
	JS_DefineFunction(cx, global, "wxCopyFile", methodInterfaceWrapper<copyFile>, 2, 0);
	JS_DefineFunction(cx, global, "wxFileExists", methodInterfaceWrapper<fileExists>, 1, 0);
	JS_DefineFunction(cx, global, "wxRenameFile", methodInterfaceWrapper<renameFile>, 2, 0);
	JS_DefineFunction(cx, global, "wxGetCwd", methodInterfaceWrapper<getCwd>, 0, 0);
	JS_DefineFunction(cx, global, "wxGetFreeDiskSpace", methodInterfaceWrapper<getFreeDiskSpace>, 1, 0);
	JS_DefineFunction(cx, global, "wxGetTotalDiskSpace", methodInterfaceWrapper<getTotalDiskSpace>, 1, 0);
	JS_DefineFunction(cx, global, "wxGetOSDirectory", methodInterfaceWrapper<getOSDirectory>, 0, 0);
	JS_DefineFunction(cx, global, "wxIsAbsolutePath", methodInterfaceWrapper<isAbsolutePath>, 1, 0);
	JS_DefineFunction(cx, global, "wxIsWild", methodInterfaceWrapper<isWild>, 1, 0);
	JS_DefineFunction(cx, global, "wxDirExists", methodInterfaceWrapper<dirExists>, 1, 0);
	JS_DefineFunction(cx, global, "wxMatchWild", methodInterfaceWrapper<matchWild>, 3, 0);
	JS_DefineFunction(cx, global, "wxMkDir", methodInterfaceWrapper<mkDir>, 2, 0);
	JS_DefineFunction(cx, global, "wxRemoveFile", methodInterfaceWrapper<removeFile>, 1, 0);
	JS_DefineFunction(cx, global, "wxRmDir", methodInterfaceWrapper<rmDir>, 1, 0);
	JS_DefineFunction(cx, global, "wxSetWorkingDirectory", methodInterfaceWrapper<setWorkingDirectory>, 1, 0);
    JS_DefineFunction(cx, global, "wxExecute", methodInterfaceWrapper<execute>, 1, 0);
    JS_DefineFunction(cx, global, "wxShell", methodInterfaceWrapper<shell>, 1, 0);

	return true;
}

bool io::InitObject(JSContext *cx, JSObject *obj)
{
    return true;
}

void io::Destroy()
{
}
