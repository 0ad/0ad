#include "precompiled.h"

/*
 * wxJavaScript - accentry.cpp
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
 * $Id: accentry.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// accentry.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"

#include "accentry.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/accentry</file>
 * <module>gui</module>
 * <class name="wxAcceleratorEntry">
 *  An object used by an application wishing to create an accelerator table.
 *  See @wxAcceleratorTable, wxMenuItem @wxMenuItem#accel property.
 * </class>
 */
WXJS_INIT_CLASS(AcceleratorEntry, "wxAcceleratorEntry", 0)

/***
 * <properties>
 *  <property name="flags" type="Integer">See @wxAcceleratorEntry#flag</property>
 *  <property name="keyCode" type="Integer">
 *   See @wxKeyCode
 *  </property>
 *  <property name="command" type="Integer">
 *   The menu or control command identifier.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(AcceleratorEntry)
  WXJS_PROPERTY(P_FLAG, "flags")
  WXJS_PROPERTY(P_KEYCODE, "keyCode")
  WXJS_PROPERTY(P_COMMAND, "command")
WXJS_END_PROPERTY_MAP()

/***
 * <constants>
 *  <type name="flag">
 *   <constant name="NORMAL" />
 *   <constant name="ALT" />
 *   <constant name="CTRL" />
 *   <constant name="SHIFT" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(AcceleratorEntry)
  WXJS_CONSTANT(wxACCEL_, NORMAL)
  WXJS_CONSTANT(wxACCEL_, ALT)
  WXJS_CONSTANT(wxACCEL_, CTRL)  
  WXJS_CONSTANT(wxACCEL_, SHIFT) 
WXJS_END_CONSTANT_MAP()

bool AcceleratorEntry::GetProperty(wxAcceleratorEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id) 
	{
	case P_FLAG:
		*vp = ToJS(cx, p->GetFlags());
		break;
	case P_KEYCODE:
		*vp = ToJS(cx, p->GetKeyCode());
		break;
	case P_COMMAND:
		*vp = ToJS(cx, p->GetCommand());
		break;
    }
    return true;
}

bool AcceleratorEntry::SetProperty(wxAcceleratorEntry *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id) 
	{
	case P_FLAG:
		{
			int flag;
			if ( FromJS(cx, *vp, flag) )
				p->Set(flag, p->GetKeyCode(), p->GetCommand());
			break;
		}
	case P_KEYCODE:
		{
			int keycode;
			if ( FromJS(cx, *vp, keycode) )
				p->Set(p->GetFlags(), keycode, p->GetCommand());
			break;
		}
	case P_COMMAND:
		{
			int command;
			if ( FromJS(cx, *vp, command) )
				p->Set(p->GetFlags(), p->GetKeyCode(), command);
			break;
		}
	}
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Flags" type="Integer" default="0">
 *    Indicates which modifier key is pressed. See @wxAcceleratorEntry#constants
 *    for the possible values.
 *   </arg>
 *   <arg name="Keycode" type="Integer" default="0">
 *    The keycode. See @wxKeyCode
 *   </arg>
 *   <arg name="Command" type="Integer" default="0">
 *    The menu or command id.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxAcceleratorEntry object.
 *  </desc>
 * </ctor>
 */
wxAcceleratorEntry* AcceleratorEntry::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	int flag = 0;
	int key = 0;
	int id = 0;

	if ( argc == 0 )
		return new wxAcceleratorEntry();

	if ( argc > 3 )
        argc = 3;

	switch(argc)
	{
	case 3:
		if ( ! FromJS(cx, argv[2], id) )
			break;
		// Walk through
	case 2:
		if ( ! FromJS(cx, argv[1], key) )
			break;
		// Walk through
	case 1:
		if ( ! FromJS(cx, argv[0], flag) )
			break;
		// Walk through
	default:
		{
			return new wxAcceleratorEntry(flag, key, id);
		}
	}

	return NULL;
}

WXJS_BEGIN_METHOD_MAP(AcceleratorEntry)
	WXJS_METHOD("set", set, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="set">
 *  <function>
 *   <arg name="Flags" type="Integer">
 *    Indicates which modifier key is pressed. See @wxAcceleratorEntry#constants
 *    for the possible values.
 *   </arg>
 *   <arg name="Keycode" type="Integer" default="0">
 *    The keycode. See @wxKeyCode
 *   </arg>
 *   <arg name="Command" type="Integer" default="0">
 *    The menu or command id.
 *   </arg>
 *  </function>
 *  <desc>
 *   See @wxAcceleratorEntry#ctor for the explanation of the arguments.
 *  </desc>
 * </method>
 */
JSBool AcceleratorEntry::set(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if ( argc > 3 )
        argc = 3;

	wxAcceleratorEntry *p = GetPrivate(cx, obj);

	int flag = p->GetFlags();
	int key = p->GetKeyCode();
	int id = p->GetCommand();

	switch(argc)
	{
	case 3:
		if ( ! FromJS(cx, argv[2], id) )
			break;
		// Walk through
	case 2:
		if ( ! FromJS(cx, argv[1], key) )
			break;
		// Walk through
	case 1:
		if ( ! FromJS(cx, argv[0], flag) )
			break;
		// Walk through
	default:
		{
			p->Set(flag, key, id);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}
