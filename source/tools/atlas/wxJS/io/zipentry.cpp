#include "precompiled.h"

/*
 * wxJavaScript - zipentry.cpp
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
 * $Id: zipentry.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "zipentry.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>zipentry</file>
 * <module>io</module>
 * <class name="wxZipEntry" prototype="@wxArchiveEntry" version="0.8.3">
 *  Holds the meta-data for an entry in a zip.
 * </class>
 */

WXJS_INIT_CLASS(ZipEntry, "wxZipEntry", 0)

/***
 * <constants>
 *  <type name="wxZipMethod">
 *   <constant name="STORE" />
 *   <constant name="SHRINK" />
 *   <constant name="REDUCE1" />
 *   <constant name="REDUCE2" />
 *   <constant name="REDUCE3" />
 *   <constant name="REDUCE4" />
 *   <constant name="IMPLODE" />
 *   <constant name="TOKENIZE" />
 *   <constant name="DEFLATE" />
 *   <constant name="DEFLATE64" />
 *   <constant name="BZIP2" />
 *   <constant name="DEFAULT" />
 *   <desc>
 *    Compression Mode. wxZipMethod is ported as a separate JavaScript class.
 *   </desc>
 *  </type>
 *  <type name="wxZipSystem">
 *   <constant name="MSDOS" />
 *   <constant name="AMIGA" />
 *   <constant name="OPENVMS" />
 *   <constant name="UNIX" />
 *   <constant name="VM_CMS" />
 *   <constant name="ATARI_ST" />
 *   <constant name="OS2_HPFS" />
 *   <constant name="MACINTOSH" />
 *   <constant name="Z_SYSTEM" />
 *   <constant name="CPM" />
 *   <constant name="WINDOWS_NTFS" />
 *   <constant name="MVS" />
 *   <constant name="VSE" />
 *   <constant name="ACORN_RISC" />
 *   <constant name="VFAT" />
 *   <constant name="ALTERNATE_MVS" />
 *   <constant name="BEOS" />
 *   <constant name="TANDEM" />
 *   <constant name="OS_400" />
 *   <desc>
 *    Originating File-System. wxZipSystem is ported as a separate JavaScript object.
 *   </desc>
 *  </type>
 *  <type name="wxZipAttributes">
 *   <constant name="RDONLY" />
 *   <constant name="HIDDEN" />
 *   <constant name="SYSTEM" />
 *   <constant name="SUBDIR" />
 *   <constant name="ARCH" />
 *   <constant name="MASK" />
 *   <desc>
 *    wxZipAttributes is ported as a separate JavaScript object
 *   </desc>
 *  </type>
 *  <type name="wxZipFlags">
 *   <constant name="ENCRYPTED" />
 *   <constant name="DEFLATE_NORMAL" />
 *   <constant name="DEFLATE_EXTRA" />
 *   <constant name="DEFLATE_FAST" />
 *   <constant name="DEFLATE_SUPERFAST" />
 *   <constant name="DEFLATE_MASK" />
 *   <constant name="SUMS_FOLLOW" />
 *   <constant name="ENHANCED" />
 *   <constant name="PATCH" />
 *   <constant name="STRONG_ENC" />
 *   <constant name="UNUSED" />
 *   <constant name="RESERVED" />
 *   <desc>
 *    wxZipFlags is ported as a separate JavaScript object.
 *   </desc>
 *  </type>
 *
 * </constants>
 */
void ZipEntry::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxZipMethodMap[] = 
    {
		WXJS_CONSTANT(wxZIP_METHOD_, STORE)
		WXJS_CONSTANT(wxZIP_METHOD_, SHRINK)
		WXJS_CONSTANT(wxZIP_METHOD_, REDUCE1)
		WXJS_CONSTANT(wxZIP_METHOD_, REDUCE2)
		WXJS_CONSTANT(wxZIP_METHOD_, REDUCE3)
		WXJS_CONSTANT(wxZIP_METHOD_, REDUCE4)
		WXJS_CONSTANT(wxZIP_METHOD_, IMPLODE)
		WXJS_CONSTANT(wxZIP_METHOD_, TOKENIZE)
		WXJS_CONSTANT(wxZIP_METHOD_, DEFLATE)
		WXJS_CONSTANT(wxZIP_METHOD_, DEFLATE64)
		WXJS_CONSTANT(wxZIP_METHOD_, BZIP2)
		WXJS_CONSTANT(wxZIP_METHOD_, DEFAULT)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxZipMethod", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxZipMethodMap);

    JSConstDoubleSpec wxZipSystemMap[] = 
    {
		WXJS_CONSTANT(wxZIP_SYSTEM_, MSDOS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, AMIGA)
		WXJS_CONSTANT(wxZIP_SYSTEM_, OPENVMS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, UNIX)
		WXJS_CONSTANT(wxZIP_SYSTEM_, VM_CMS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, ATARI_ST)
		WXJS_CONSTANT(wxZIP_SYSTEM_, OS2_HPFS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, MACINTOSH)
		WXJS_CONSTANT(wxZIP_SYSTEM_, Z_SYSTEM)
		WXJS_CONSTANT(wxZIP_SYSTEM_, CPM)
		WXJS_CONSTANT(wxZIP_SYSTEM_, WINDOWS_NTFS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, MVS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, VSE)
		WXJS_CONSTANT(wxZIP_SYSTEM_, ACORN_RISC)
		WXJS_CONSTANT(wxZIP_SYSTEM_, VFAT)
		WXJS_CONSTANT(wxZIP_SYSTEM_, ALTERNATE_MVS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, BEOS)
		WXJS_CONSTANT(wxZIP_SYSTEM_, TANDEM)
		WXJS_CONSTANT(wxZIP_SYSTEM_, OS_400)
	    { 0 }
    };

    constObj = JS_DefineObject(cx, obj, "wxZipSystem", 
						 	   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxZipSystemMap);

	JSConstDoubleSpec wxZipAttributes[] = 
    {
		WXJS_CONSTANT(wxZIP_A_, RDONLY)
		WXJS_CONSTANT(wxZIP_A_, HIDDEN)
		WXJS_CONSTANT(wxZIP_A_, SYSTEM)
		WXJS_CONSTANT(wxZIP_A_, SUBDIR)
		WXJS_CONSTANT(wxZIP_A_, ARCH)
		WXJS_CONSTANT(wxZIP_A_, MASK)
		{ 0 }
	};
    constObj = JS_DefineObject(cx, obj, "wxZipAttributes", 
						 	   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxZipAttributes);

	JSConstDoubleSpec wxZipFlags[] = 
    {
		WXJS_CONSTANT(wxZIP_, ENCRYPTED)
		WXJS_CONSTANT(wxZIP_, DEFLATE_NORMAL)
		WXJS_CONSTANT(wxZIP_, DEFLATE_EXTRA)
		WXJS_CONSTANT(wxZIP_, DEFLATE_FAST)
		WXJS_CONSTANT(wxZIP_, DEFLATE_SUPERFAST)
		WXJS_CONSTANT(wxZIP_, DEFLATE_MASK)
		WXJS_CONSTANT(wxZIP_, SUMS_FOLLOW)
		WXJS_CONSTANT(wxZIP_, ENHANCED)
		WXJS_CONSTANT(wxZIP_, PATCH)
		WXJS_CONSTANT(wxZIP_, STRONG_ENC)
		WXJS_CONSTANT(wxZIP_, UNUSED)
		WXJS_CONSTANT(wxZIP_, RESERVED)
		{ 0 }
	};
    constObj = JS_DefineObject(cx, obj, "wxZipFlags", 
						 	   NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxZipFlags);
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Name" type="String" default="" />
 *   <arg name="Date" type="Date" default="Now()" />
 *   <arg name="Offset" type="Integer" default="-1" />
 *  </function>
 *  <desc>
 *   Creates a new wxZipEntry
 *  </desc>
 * </ctor>
 */
wxZipEntry* ZipEntry::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	off_t size = wxInvalidOffset;
	wxDateTime dt = wxDateTime::Now();
	wxString name = wxEmptyString;

	switch(argc)
	{
	case 3:
		if ( ! FromJS(cx, argv[2], size) )
			return NULL;
		// Fall through
	case 2:
		if ( ! FromJS(cx, argv[1], dt) )
			return NULL;
		// Fall through
	case 1:
		FromJS(cx, argv[0], name);
		break;
	}

	wxZipEntry* entry = new wxZipEntry(name, dt, size);
	entry->SetMethod(wxZIP_METHOD_DEFAULT);
	return entry;
}

/***
 * <properties>
 *  <property name="comment" type="String">
 *   Get/Set a short comment for this entry.
 *  </property>
 *  <property name="compressedSize" type="Integer" readonly="Y">
 *   The compressed size of this entry in bytes.
 *  </property>
 *  <property name="crc" type="Integer" readonly="Y">
 *   CRC32 for this entry's data.
 *  </property>
 *  <property name="externalAttributes" type="Integer" readonly="Y">
 *   Attributes of the entry. See @wxZipEntry#wxZipAttributes.
 *  </property>
 *  <property name="extra" type="String">
 *   The extra field is used to store platform or application specific data. 
 *   See Pkware's document 'appnote.txt' for information on its format.
 *  </property>
 *  <property name="flags" type="Integer" readonly="Y">
 *   see @wxZipEntry#wxZipFlags
 *  </property>
 *  <property name="localExtra" type="String">
 *   The extra field is used to store platform or application specific data. 
 *   See Pkware's document 'appnote.txt' for information on its format. 
 *  </property>
 *  <property name="method" type="Integer">
 *   The compression method. See @wxZipEntry#wxZipMethod.
 *  </property>
 *  <property name="mode" type="Integer" />
 *  <property name="systemMadeBy" type="Integer">
 *   The originating file-system. The default constructor sets this to 
 *   wxZipSystem.MSDOS. Set it to wxZipSystem.UNIX in order to be able
 *   to store unix permissions using @wxZipEntry#mode.
 *  </property>
 *  <property name="madeByUnix" type="Boolean" readonly="Y">
 *   Returns true if @wxZipEntry#systemMadeBy is a flavour of unix.
 *  </property>
 *  <property name="text" type="Boolean">
 *   Indicates that this entry's data is text in an 8-bit encoding.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ZipEntry)
	WXJS_PROPERTY(P_COMMENT, "comment")
	WXJS_READONLY_PROPERTY(P_COMPRESSED_SIZE, "compressedSize")
	WXJS_READONLY_PROPERTY(P_CRC, "crc")
	WXJS_READONLY_PROPERTY(P_EXTERNAL_ATTR, "externalAttributes")
	WXJS_PROPERTY(P_EXTRA, "extra")
	WXJS_READONLY_PROPERTY(P_FLAGS, "flags")
	WXJS_PROPERTY(P_LOCAL_EXTRA, "localExtra")
	WXJS_PROPERTY(P_MODE, "mode")
	WXJS_PROPERTY(P_METHOD, "method")
	WXJS_PROPERTY(P_SYSTEM_MADE_BY, "systemMadeBy")
	WXJS_READONLY_PROPERTY(P_MADE_BY_UNIX, "madeByUnix")
	WXJS_PROPERTY(P_TEXT, "text")
WXJS_END_PROPERTY_MAP()

bool ZipEntry::GetProperty(wxZipEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
	case P_COMMENT:
		*vp = ToJS(cx, p->GetComment());
		break;
	case P_COMPRESSED_SIZE:
		*vp = ToJS<long>(cx, p->GetCompressedSize());
		break;
	case P_CRC:
		*vp = ToJS(cx, p->GetCrc());
		break;
	case P_EXTERNAL_ATTR:
		*vp = ToJS(cx, p->GetExternalAttributes());
		break;
	case P_EXTRA:
		*vp = ToJS(cx, wxString::FromAscii(p->GetExtra()));
		break;
	case P_FLAGS:
		*vp = ToJS(cx, p->GetFlags());
		break;
	case P_LOCAL_EXTRA:
		*vp = ToJS(cx, wxString::FromAscii(p->GetLocalExtra()));
		break;
	case P_METHOD:
		*vp = ToJS(cx, p->GetMethod());
		break;
	case P_MODE:
		*vp = ToJS(cx, p->GetMode());
		break;
	case P_SYSTEM_MADE_BY:
		*vp = ToJS(cx, p->GetSystemMadeBy());
		break;
	case P_MADE_BY_UNIX:
		*vp = ToJS(cx, p->IsMadeByUnix());
		break;
	case P_TEXT:
		*vp = ToJS(cx, p->IsText());
		break;
    }
    return true;
}

bool ZipEntry::SetProperty(wxZipEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
	case P_EXTRA:
		{
			wxString extra;
			FromJS(cx, *vp, extra);
			p->SetExtra(extra.ToAscii(), extra.Length());
		}
		break;
	case P_LOCAL_EXTRA:
		{
			wxString extra;
			FromJS(cx, *vp, extra);
			p->SetLocalExtra(extra.ToAscii(), extra.Length());
		}
		break;
	case P_MODE:
		{
			int mode;
			if ( FromJS(cx, *vp, mode) )
			{
				p->SetMode(mode);
			}
			break;
		}
	case P_METHOD:
		{
			int method;
			if ( FromJS(cx, *vp, method) )
			{
				p->SetMethod(method);
			}
		}
	case P_SYSTEM_MADE_BY:
		{
			int madeBy;
			if ( FromJS(cx, *vp, madeBy) )
			{
				p->SetSystemMadeBy(madeBy);
			}
			break;
		}
	case P_TEXT:
		{
			bool text;
			if ( FromJS(cx, *vp, text) )
			{
				p->SetIsText(text);
			}
			break;
		}
    }
	return true;
}
