#include "precompiled.h"

/*
 * wxJavaScript - ipaddr.cpp
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
 * $Id: ipaddr.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "ipaddr.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>ipaddr</file>
 * <module>io</module>
 * <class name="wxIPaddress" prototype="@wxSockAddress" version="0.8.4">
 *  wxIPaddress is an prototype class for all internet protocol address objects. 
 *  Currently, only @wxIPV4address is implemented.
 * </class>
 */
WXJS_INIT_CLASS(IPaddress, "wxIPaddress", 0)

/***
 * <properties>
 *  <property name="hostname" type="String">
 *   Get/Set the hostname.
 *  </property>
 *  <property name="IPAddress" type="String" readonly="Y">
 *   Returns a string containing the IP address.
 *  </property>
 *  <property name="localhost" type="Boolean" readonly="Y">
 *   Returns true when the hostname is localhost.
 *  </property>
 *  <property name="service" type="Integer">
 *   Get/Set the current service.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(IPaddress)
	WXJS_PROPERTY(P_HOSTNAME, "hostname")
	WXJS_READONLY_PROPERTY(P_IPADDRESS, "IPAddress")
	WXJS_READONLY_PROPERTY(P_LOCALHOST, "localhost")
	WXJS_PROPERTY(P_SERVICE, "service")
WXJS_END_PROPERTY_MAP()

bool IPaddress::GetProperty(wxIPaddress *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_HOSTNAME:
		*vp = ToJS(cx, p->Hostname());
		break;
	case P_IPADDRESS:
		*vp = ToJS(cx, p->IPAddress());
		break;
	case P_LOCALHOST:
		*vp = ToJS(cx, p->IsLocalHost());
		break;
	case P_SERVICE:
		*vp = ToJS<int>(cx, p->Service());
		break;
	default:
		*vp = JSVAL_VOID;
	}
    return true;
}

bool IPaddress::SetProperty(wxIPaddress *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_HOSTNAME:
		{
			wxString hostname;
			FromJS(cx, *vp, hostname);
			p->Hostname(hostname);
			break;
		}
	case P_SERVICE:
		{
			int service;
			if ( FromJS(cx, *vp, service) )
				p->Service(service);
			break;
		}
	}
    return true;
}

WXJS_BEGIN_METHOD_MAP(IPaddress)
	WXJS_METHOD("anyAddress", anyAddress, 0)
	WXJS_METHOD("localhost", localhost, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="anyAddress">
 *  <function returns="Boolean" />
 *  <desc>
 *   Internally, this is the same as setting the IP address to INADDR_ANY.
 *   Returns true on success, false if something went wrong.
 *  </desc>
 * </method>
 */
JSBool IPaddress::anyAddress(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxIPaddress *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxIPaddress"));

	*rval = ToJS(cx, p->AnyAddress());

	return JS_TRUE;
}

/***
 * <method name="localhost">
 *  <function returns="Boolean" />
 *  <desc>
 *   Set the address to localhost.
 *   Returns true on success, false if something went wrong.
 *  </desc>
 * </method>
 */
JSBool IPaddress::localhost(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxIPaddress *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxIPaddress"));

	*rval = ToJS(cx, p->LocalHost());

	return JS_TRUE;
}
