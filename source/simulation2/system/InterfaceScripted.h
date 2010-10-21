/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_INTERFACE_SCRIPTED
#define INCLUDED_INTERFACE_SCRIPTED

#include "scriptinterface/ScriptInterface.h"
#include "js/jsapi.h"

#define BEGIN_INTERFACE_WRAPPER(iname) \
	JSClass class_ICmp##iname = { \
		"ICmp" #iname, JSCLASS_HAS_PRIVATE, \
		JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, \
		JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub, \
		JSCLASS_NO_OPTIONAL_MEMBERS \
	}; \
	static JSFunctionSpec methods_ICmp##iname[] = {

#define END_INTERFACE_WRAPPER(iname) \
		{ NULL } \
	}; \
	void ICmp##iname::InterfaceInit(ScriptInterface& scriptInterface) { \
		JSContext* cx = scriptInterface.GetContext(); \
		JSObject* global = JS_GetGlobalObject(cx); \
		JS_InitClass(cx, global, NULL, &class_ICmp##iname, NULL, 0, NULL, methods_ICmp##iname, NULL, NULL); \
	} \
	JSClass* ICmp##iname::GetJSClass() const { return &class_ICmp##iname; } \
	void RegisterComponentInterface_##iname(ScriptInterface& scriptInterface) { \
		ICmp##iname::InterfaceInit(scriptInterface); \
	}

#define DEFINE_INTERFACE_METHOD_0(scriptname, rettype, classname, methodname) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, &class_##classname, classname, &classname::methodname>)), \
		0, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#define DEFINE_INTERFACE_METHOD_1(scriptname, rettype, classname, methodname, arg1) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, arg1, &class_##classname, classname, &classname::methodname>)), \
		1, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#define DEFINE_INTERFACE_METHOD_2(scriptname, rettype, classname, methodname, arg1, arg2) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, arg1, arg2, &class_##classname, classname, &classname::methodname>)), \
		2, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#define DEFINE_INTERFACE_METHOD_3(scriptname, rettype, classname, methodname, arg1, arg2, arg3) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, arg1, arg2, arg3, &class_##classname, classname, &classname::methodname>)), \
		3, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#define DEFINE_INTERFACE_METHOD_4(scriptname, rettype, classname, methodname, arg1, arg2, arg3, arg4) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, arg1, arg2, arg3, arg4, &class_##classname, classname, &classname::methodname>)), \
		4, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#define DEFINE_INTERFACE_METHOD_5(scriptname, rettype, classname, methodname, arg1, arg2, arg3, arg4, arg5) \
	{ scriptname, \
		reinterpret_cast<JSNative>(static_cast<JSFastNative>(ScriptInterface::callMethod<rettype, arg1, arg2, arg3, arg4, arg5, &class_##classname, classname, &classname::methodname>)), \
		5, \
		JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSFUN_FAST_NATIVE, 0 },

#endif // INCLUDED_INTERFACE_SCRIPTED
