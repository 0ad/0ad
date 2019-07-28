/* Copyright (C) 2019 Wildfire Games.
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

#include "gui/CGUIColor.h"
#include "gui/CGUIList.h"
#include "gui/CGUISeries.h"
#include "gui/GUIbase.h"
#include "gui/IGUIObject.h"
#include "lib/external_libraries/libsdl.h"
#include "maths/Vector2D.h"
#include "ps/Hotkey.h"
#include "scriptinterface/ScriptConversions.h"

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
		SET(obj, "clicks", (int)val.ev.button.clicks);
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

template<> void ScriptInterface::ToJSVal<CGUIString>(JSContext* cx, JS::MutableHandleValue ret, const CGUIString& val)
{
	ScriptInterface::ToJSVal(cx, ret, val.GetOriginalString());
}

template<> bool ScriptInterface::FromJSVal<CGUIString>(JSContext* cx, JS::HandleValue v, CGUIString& out)
{
	std::wstring val;
	if (!FromJSVal(cx, v, val))
		return false;
	out.SetValue(val);
	return true;
}

JSVAL_VECTOR(CVector2D)
JSVAL_VECTOR(std::vector<CVector2D>)
JSVAL_VECTOR(CGUIString)

template<> void ScriptInterface::ToJSVal<CGUIColor>(JSContext* cx, JS::MutableHandleValue ret, const CGUIColor& val)
{
	ToJSVal<CColor>(cx, ret, val);
}

template<> bool ScriptInterface::FromJSVal<CGUIColor>(JSContext* cx, JS::HandleValue v, CGUIColor& out)
{
	if (v.isString())
	{
		CStr name;
		if (!FromJSVal(cx, v, name))
			return false;

		if (!out.ParseString(name))
		{
			JS_ReportError(cx, "Invalid color '%s'", name.c_str());
			return false;
		}
		return true;
	}

	// Parse as object
	return FromJSVal<CColor>(cx, v, out);
}

template<> void ScriptInterface::ToJSVal<CClientArea>(JSContext* cx, JS::MutableHandleValue ret, const CClientArea& val)
{
	val.ToJSVal(cx, ret);
}

template<> bool ScriptInterface::FromJSVal<CClientArea>(JSContext* cx, JS::HandleValue v, CClientArea& out)
{
	return out.FromJSVal(cx, v);
}

template<> void ScriptInterface::ToJSVal<CGUIList>(JSContext* cx, JS::MutableHandleValue ret, const CGUIList& val)
{
	ToJSVal(cx, ret, val.m_Items);
}

template<> bool ScriptInterface::FromJSVal<CGUIList>(JSContext* cx, JS::HandleValue v, CGUIList& out)
{
	return FromJSVal(cx, v, out.m_Items);
}

template<> void ScriptInterface::ToJSVal<CGUISeries>(JSContext* cx, JS::MutableHandleValue ret, const CGUISeries& val)
{
	ToJSVal(cx, ret, val.m_Series);
}

template<> bool ScriptInterface::FromJSVal<CGUISeries>(JSContext* cx, JS::HandleValue v, CGUISeries& out)
{
	return FromJSVal(cx, v, out.m_Series);
}

template<> void ScriptInterface::ToJSVal<EVAlign>(JSContext* cx, JS::MutableHandleValue ret, const EVAlign& val)
{
	std::string word;
	switch (val)
	{
	case EVAlign_Top:
		word = "top";
		break;

	case EVAlign_Bottom:
		word = "bottom";
		break;

	case EVAlign_Center:
		word = "center";
		break;

	default:
		word = "error";
		JS_ReportError(cx, "Invalid EVAlign");
		break;
	}
	ToJSVal(cx, ret, word);
}

template<> bool ScriptInterface::FromJSVal<EVAlign>(JSContext* cx, JS::HandleValue v, EVAlign& out)
{
	std::string word;
	FromJSVal(cx, v, word);

	if (word == "top")
		out = EVAlign_Top;
	else if (word == "bottom")
		out = EVAlign_Bottom;
	else if (word == "center")
		out = EVAlign_Center;
	else
	{
		out = EVAlign_Top;
		JS_ReportError(cx, "Invalid alignment (should be 'left', 'right' or 'center')");
		return false;
	}
	return true;
}

template<> void ScriptInterface::ToJSVal<EAlign>(JSContext* cx, JS::MutableHandleValue ret, const EAlign& val)
{
	std::string word;
	switch (val)
	{
	case EAlign_Left:
		word = "left";
		break;
	case EAlign_Right:
		word = "right";
		break;
	case EAlign_Center:
		word = "center";
		break;
	default:
		word = "error";
		JS_ReportError(cx, "Invalid alignment (should be 'left', 'right' or 'center')");
		break;
	}
	ToJSVal(cx, ret, word);
}

template<> bool ScriptInterface::FromJSVal<EAlign>(JSContext* cx, JS::HandleValue v, EAlign& out)
{
	std::string word;
	FromJSVal(cx, v, word);

	if (word == "left")
		out = EAlign_Left;
	else if (word == "right")
		out = EAlign_Right;
	else if (word == "center")
		out = EAlign_Center;
	else
	{
		out = EAlign_Left;
		JS_ReportError(cx, "Invalid alignment (should be 'left', 'right' or 'center')");
		return false;
	}
	return true;
}

template<> void ScriptInterface::ToJSVal<CGUISpriteInstance>(JSContext* cx, JS::MutableHandleValue ret, const CGUISpriteInstance& val)
{
	ToJSVal(cx, ret, val.GetName());
}

template<> bool ScriptInterface::FromJSVal<CGUISpriteInstance>(JSContext* cx, JS::HandleValue v, CGUISpriteInstance& out)
{
	std::string name;
	if (!FromJSVal(cx, v, name))
		return false;

	out.SetName(name);
	return true;
}
