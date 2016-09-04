/* Copyright (C) 2016 Wildfire Games.
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

#include "precompiled.h"

#include "scriptinterface/ScriptInterface.h"

#include "gui/IGUIObject.h"
#include "lib/external_libraries/libsdl.h"
#include "ps/Hotkey.h"

#define SET(obj, name, value) STMT(JS::RootedValue v_(cx); AssignOrToJSVal(cx, &v_, (value)); JS_SetProperty(cx, obj, (name), v_))
	// ignore JS_SetProperty return value, because errors should be impossible
	// and we can't do anything useful in the case of errors anyway

template<> void ScriptInterface::ToJSVal<SDL_Event_>(JSContext* cx, JS::MutableHandleValue ret, SDL_Event_ const& val)
{
	JSAutoRequest rq(cx);
	const char* typeName;

	switch (val.ev.type)
	{
	case SDL_WINDOWEVENT: typeName = "windowevent"; break;
	case SDL_KEYDOWN: typeName = "keydown"; break;
	case SDL_KEYUP: typeName = "keyup"; break;
	case SDL_MOUSEMOTION: typeName = "mousemotion"; break;
	case SDL_MOUSEBUTTONDOWN: typeName = "mousebuttondown"; break;
	case SDL_MOUSEBUTTONUP: typeName = "mousebuttonup"; break;
	case SDL_QUIT: typeName = "quit"; break;
	case SDL_HOTKEYDOWN: typeName = "hotkeydown"; break;
	case SDL_HOTKEYUP: typeName = "hotkeyup"; break;
	default: typeName = "(unknown)"; break;
	}

	JS::RootedObject obj(cx, JS_NewPlainObject(cx));
	if (!obj)
	{
		ret.setUndefined();
		return;
	}

	SET(obj, "type", typeName);

	switch (val.ev.type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
		// SET(obj, "which", (int)val.ev.key.which); // (not in wsdl.h)
		// SET(obj, "state", (int)val.ev.key.state); // (not in wsdl.h)

		JS::RootedObject keysym(cx, JS_NewPlainObject(cx));
		if (!keysym)
		{
			ret.setUndefined();
			return;
		}
		JS::RootedValue keysymVal(cx, JS::ObjectValue(*keysym));
		JS_SetProperty(cx, obj, "keysym", keysymVal);

		// SET(keysym, "scancode", (int)val.ev.key.keysym.scancode); // (not in wsdl.h)
		SET(keysym, "sym", (int)val.ev.key.keysym.sym);
		// SET(keysym, "mod", (int)val.ev.key.keysym.mod); // (not in wsdl.h)
		{
			SET(keysym, "unicode", JS::UndefinedHandleValue);
		}
		// TODO: scripts have no idea what all the key/mod enum values are;
		// we should probably expose them as constants if we expect scripts to use them

		break;
	}
	case SDL_MOUSEMOTION:
	{
		// SET(obj, "which", (int)val.ev.motion.which); // (not in wsdl.h)
		// SET(obj, "state", (int)val.ev.motion.state); // (not in wsdl.h)
		SET(obj, "x", (int)val.ev.motion.x);
		SET(obj, "y", (int)val.ev.motion.y);
		// SET(obj, "xrel", (int)val.ev.motion.xrel); // (not in wsdl.h)
		// SET(obj, "yrel", (int)val.ev.motion.yrel); // (not in wsdl.h)
		break;
	}
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	{
		// SET(obj, "which", (int)val.ev.button.which); // (not in wsdl.h)
		SET(obj, "button", (int)val.ev.button.button);
		SET(obj, "state", (int)val.ev.button.state);
		SET(obj, "x", (int)val.ev.button.x);
		SET(obj, "y", (int)val.ev.button.y);
		break;
	}
	case SDL_HOTKEYDOWN:
	case SDL_HOTKEYUP:
	{
		SET(obj, "hotkey", static_cast<const char*>(val.ev.user.data1));
		break;
	}
	}

	ret.setObject(*obj);
}

template<> void ScriptInterface::ToJSVal<IGUIObject*>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, IGUIObject* const& val)
{
	if (val == NULL)
		ret.setNull();
	else
		ret.setObject(*val->GetJSObject());
}
