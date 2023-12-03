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

#include "precompiled.h"

#include "gui/ObjectBases/IGUIObject.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/CGUIList.h"
#include "gui/SettingTypes/CGUISeries.h"
#include "gui/SettingTypes/CGUISize.h"
#include "gui/Scripting/JSInterface_GUIProxy.h"
#include "lib/external_libraries/libsdl.h"
#include "maths/Size2D.h"
#include "maths/Vector2D.h"
#include "ps/Hotkey.h"
#include "ps/CLogger.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptConversions.h"

#include <string>

#define SET(obj, name, value) STMT(JS::RootedValue v_(rq.cx); Script::ToJSVal(rq, &v_, (value)); JS_SetProperty(rq.cx, obj, (name), v_))
	// ignore JS_SetProperty return value, because errors should be impossible
	// and we can't do anything useful in the case of errors anyway

template<> void Script::ToJSVal<SDL_Event_>(const ScriptRequest& rq, JS::MutableHandleValue ret, SDL_Event_ const& val)
{
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
	case SDL_HOTKEYPRESS: typeName = "hotkeypress"; break;
	case SDL_HOTKEYDOWN: typeName = "hotkeydown"; break;
	case SDL_HOTKEYUP: typeName = "hotkeyup"; break;
	case SDL_HOTKEYPRESS_SILENT: typeName = "hotkeypresssilent"; break;
	case SDL_HOTKEYUP_SILENT: typeName = "hotkeyupsilent"; break;
	default: typeName = "(unknown)"; break;
	}

	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));
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

		JS::RootedObject keysym(rq.cx, JS_NewPlainObject(rq.cx));
		if (!keysym)
		{
			ret.setUndefined();
			return;
		}
		JS::RootedValue keysymVal(rq.cx, JS::ObjectValue(*keysym));
		JS_SetProperty(rq.cx, obj, "keysym", keysymVal);

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
	case SDL_HOTKEYPRESS:
	case SDL_HOTKEYDOWN:
	case SDL_HOTKEYUP:
	case SDL_HOTKEYPRESS_SILENT:
	case SDL_HOTKEYUP_SILENT:
	{
		SET(obj, "hotkey", static_cast<const char*>(val.ev.user.data1));
		break;
	}
	}

	ret.setObject(*obj);
}

template<> void Script::ToJSVal<IGUIObject*>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, IGUIObject* const& val)
{
	if (val == nullptr)
		ret.setNull();
	else
		ret.setObject(*val->GetJSObject());
}

template<> bool Script::FromJSVal<IGUIObject*>(const ScriptRequest& rq, JS::HandleValue v, IGUIObject*& out)
{
	if (!v.isObject())
	{
		ScriptException::Raise(rq, "Value is not an IGUIObject.");
		return false;
	}
	out = IGUIProxyObject::FromPrivateSlot<IGUIObject>(v.toObjectOrNull());
	if (!out)
	{
		ScriptException::Raise(rq, "Value is not an IGUIObject.");
		return false;
	}
	return true;
}

template<> void Script::ToJSVal<CGUIString>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUIString& val)
{
	Script::ToJSVal(rq, ret, val.GetOriginalString());
}

template<> bool Script::FromJSVal<CGUIString>(const ScriptRequest& rq, JS::HandleValue v, CGUIString& out)
{
	std::wstring val;
	if (!FromJSVal(rq, v, val))
		return false;
	out.SetValue(val);
	return true;
}

JSVAL_VECTOR(CVector2D)
JSVAL_VECTOR(std::vector<CVector2D>)
JSVAL_VECTOR(CGUIString)

template<> void Script::ToJSVal<CGUIColor>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUIColor& val)
{
	ToJSVal<CColor>(rq, ret, val);
}

/**
 * The color depends on the predefined color database stored in the current GUI page.
 */
template<> bool Script::FromJSVal<CGUIColor>(const ScriptRequest& rq, JS::HandleValue v, CGUIColor& out) = delete;

template<> void Script::ToJSVal<CRect>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CRect& val)
{
	Script::CreateObject(
		rq,
		ret,
		"left", val.left,
		"right", val.right,
		"top", val.top,
		"bottom", val.bottom);
}

template<> void Script::ToJSVal<CGUISize>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUISize& val)
{
	val.ToJSVal(rq, ret);
}

template<> bool Script::FromJSVal<CGUISize>(const ScriptRequest& rq, JS::HandleValue v, CGUISize& out)
{
	return out.FromJSVal(rq, v);
}

template<> void Script::ToJSVal<CGUIList>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUIList& val)
{
	ToJSVal(rq, ret, val.m_Items);
}

template<> bool Script::FromJSVal<CGUIList>(const ScriptRequest& rq, JS::HandleValue v, CGUIList& out)
{
	return FromJSVal(rq, v, out.m_Items);
}

template<> void Script::ToJSVal<CGUISeries>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUISeries& val)
{
	ToJSVal(rq, ret, val.m_Series);
}

template<> bool Script::FromJSVal<CGUISeries>(const ScriptRequest& rq, JS::HandleValue v, CGUISeries& out)
{
	return FromJSVal(rq, v, out.m_Series);
}

template<> void Script::ToJSVal<EVAlign>(const ScriptRequest& rq, JS::MutableHandleValue ret, const EVAlign& val)
{
	std::string word;
	switch (val)
	{
	case EVAlign::TOP:
		word = "top";
		break;

	case EVAlign::BOTTOM:
		word = "bottom";
		break;

	case EVAlign::CENTER:
		word = "center";
		break;

	default:
		word = "error";
		ScriptException::Raise(rq, "Invalid EVAlign");
		break;
	}
	ToJSVal(rq, ret, word);
}

template<> bool Script::FromJSVal<EVAlign>(const ScriptRequest& rq, JS::HandleValue v, EVAlign& out)
{
	std::string word;
	FromJSVal(rq, v, word);

	if (word == "top")
		out = EVAlign::TOP;
	else if (word == "bottom")
		out = EVAlign::BOTTOM;
	else if (word == "center")
		out = EVAlign::CENTER;
	else
	{
		out = EVAlign::TOP;
		LOGERROR("Invalid alignment (should be 'left', 'right' or 'center')");
		return false;
	}
	return true;
}

template<> void Script::ToJSVal<EAlign>(const ScriptRequest& rq, JS::MutableHandleValue ret, const EAlign& val)
{
	std::string word;
	switch (val)
	{
	case EAlign::LEFT:
		word = "left";
		break;
	case EAlign::RIGHT:
		word = "right";
		break;
	case EAlign::CENTER:
		word = "center";
		break;
	default:
		word = "error";
		ScriptException::Raise(rq, "Invalid alignment (should be 'left', 'right' or 'center')");
		break;
	}
	ToJSVal(rq, ret, word);
}

template<> bool Script::FromJSVal<EAlign>(const ScriptRequest& rq, JS::HandleValue v, EAlign& out)
{
	std::string word;
	FromJSVal(rq, v, word);

	if (word == "left")
		out = EAlign::LEFT;
	else if (word == "right")
		out = EAlign::RIGHT;
	else if (word == "center")
		out = EAlign::CENTER;
	else
	{
		out = EAlign::LEFT;
		LOGERROR("Invalid alignment (should be 'left', 'right' or 'center')");
		return false;
	}
	return true;
}

template<> void Script::ToJSVal<CGUISpriteInstance>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CGUISpriteInstance& val)
{
	ToJSVal(rq, ret, val.GetName());
}

template<> bool Script::FromJSVal<CGUISpriteInstance>(const ScriptRequest& rq, JS::HandleValue v, CGUISpriteInstance& out)
{
	std::string name;
	if (!FromJSVal(rq, v, name))
		return false;

	out.SetName(name);
	return true;
}

template<> void Script::ToJSVal<CSize2D>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CSize2D& val)
{
	Script::CreateObject(rq, ret, "width", val.Width, "height", val.Height);
}

template<> bool Script::FromJSVal<CSize2D>(const ScriptRequest& rq, JS::HandleValue v, CSize2D& out)
{
	if (!v.isObject())
	{
		LOGERROR("CSize2D value must be an object!");
		return false;
	}

	if (!FromJSProperty(rq, v, "width", out.Width))
	{
		LOGERROR("Failed to get CSize2D.Width property");
		return false;
	}

	if (!FromJSProperty(rq, v, "height", out.Height))
	{
		LOGERROR("Failed to get CSize2D.Height property");
		return false;
	}

	return true;
}

template<> void Script::ToJSVal<CVector2D>(const ScriptRequest& rq, JS::MutableHandleValue ret, const CVector2D& val)
{
	Script::CreateObject(rq, ret, "x", val.X, "y", val.Y);
}

template<> bool Script::FromJSVal<CVector2D>(const ScriptRequest& rq, JS::HandleValue v, CVector2D& out)
{
	if (!v.isObject())
	{
		LOGERROR("CVector2D value must be an object!");
		return false;
	}

	if (!FromJSProperty(rq, v, "x", out.X))
	{
		LOGERROR("Failed to get CVector2D.X property");
		return false;
	}

	if (!FromJSProperty(rq, v, "y", out.Y))
	{
		LOGERROR("Failed to get CVector2D.Y property");
		return false;
	}

	return true;
}

#undef SET
