/* Copyright (C) 2021 Wildfire Games.
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

#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptInterface.h"

#define BEGIN_INTERFACE_WRAPPER(iname) \
	JSClass class_ICmp##iname = { \
		"ICmp" #iname, JSCLASS_HAS_PRIVATE \
	}; \
	static JSFunctionSpec methods_ICmp##iname[] = {

#define END_INTERFACE_WRAPPER(iname) \
		JS_FS_END \
	}; \
	void ICmp##iname::InterfaceInit(ScriptInterface& scriptInterface) { \
		scriptInterface.DefineCustomObjectType(&class_ICmp##iname, NULL, 0, NULL, methods_ICmp##iname, NULL, NULL); \
	} \
	bool ICmp##iname::NewJSObject(const ScriptInterface& scriptInterface, JS::MutableHandleObject out) const\
	{ \
		out.set(scriptInterface.CreateCustomObject("ICmp" #iname)); \
		return true; \
	} \
	void RegisterComponentInterface_##iname(ScriptInterface& scriptInterface) { \
		ICmp##iname::InterfaceInit(scriptInterface); \
	}

template <typename T, JSClass* jsClass>
inline T* ComponentGetter(const ScriptRequest& rq, JS::CallArgs& args)
{
	return ScriptInterface::GetPrivate<T>(rq, args, jsClass);
}

#define DEFINE_INTERFACE_METHOD(scriptname, classname, methodname) \
	ScriptFunction::Wrap<&classname::methodname, ComponentGetter<classname, &class_##classname>>(scriptname),

#endif // INCLUDED_INTERFACE_SCRIPTED
