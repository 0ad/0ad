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

#include "JSInterface_IGUIObject.h"
#include "JSInterface_GUITypes.h"

#include "gui/IGUIObject.h"
#include "gui/CGUI.h"
#include "gui/IGUIScrollBar.h"
#include "gui/CList.h"
#include "gui/GUIManager.h"

#include "ps/CLogger.h"

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptExtraHeaders.h"

JSClass JSI_IGUIObject::JSI_class = {
	"GUIObject", JSCLASS_HAS_PRIVATE,
	nullptr, nullptr,
	JSI_IGUIObject::getProperty, JSI_IGUIObject::setProperty,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, JSI_IGUIObject::construct, nullptr
};

JSFunctionSpec JSI_IGUIObject::JSI_methods[] =
{
	JS_FN("toString", JSI_IGUIObject::toString, 0, 0),
	JS_FN("focus", JSI_IGUIObject::focus, 0, 0),
	JS_FN("blur", JSI_IGUIObject::blur, 0, 0),
	JS_FN("getComputedSize", JSI_IGUIObject::getComputedSize, 0, 0),
	JS_FN("getTextSize", JSI_IGUIObject::getTextSize, 0, 0),
	JS_FS_END
};

bool JSI_IGUIObject::getProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
	JSAutoRequest rq(cx);
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, obj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return false;

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return false;

	// Skip some things which are known to be functions rather than properties.
	// ("constructor" *must* be here, else it'll try to GetSettingType before
	// the private IGUIObject* has been set (and thus crash). The others are
	// partly for efficiency, and also to allow correct reporting of attempts to
	// access nonexistent properties.)
	if (propName == "constructor" ||
		propName == "prototype"   ||
		propName == "toString"    ||
		propName == "toJSON"      ||
		propName == "focus"       ||
		propName == "blur"        ||
		propName == "getTextSize" ||
		propName == "getComputedSize"
	   )
		return true;

	// Use onWhatever to access event handlers
	if (propName.substr(0, 2) == "on")
	{
		CStr eventName(CStr(propName.substr(2)).LowerCase());
		std::map<CStr, JS::Heap<JSObject*>>::iterator it = e->m_ScriptHandlers.find(eventName);
		if (it == e->m_ScriptHandlers.end())
			vp.setNull();
		else
			vp.setObject(*it->second.get());
		return true;
	}

	if (propName == "parent")
	{
		IGUIObject* parent = e->GetParent();

		if (parent)
			vp.set(JS::ObjectValue(*parent->GetJSObject()));
		else
			vp.set(JS::NullValue());

		return true;
	}
	else if (propName == "children")
	{
		JS::RootedObject obj(cx, JS_NewArrayObject(cx, JS::HandleValueArray::empty()));
		vp.setObject(*obj);

		for (size_t i = 0; i < e->m_Children.size(); ++i)
		{
			JS::RootedValue val(cx);
			ScriptInterface::ToJSVal(cx, &val, e->m_Children[i]);
			JS_SetElement(cx, obj, i, val);
		}

		return true;
	}
	else if (propName == "name")
	{
		vp.set(JS::StringValue(JS_NewStringCopyZ(cx, e->GetName().c_str())));
		return true;
	}
	else
	{
		// Retrieve the setting's type (and make sure it actually exists)
		EGUISettingType Type;
		if (e->GetSettingType(propName, Type) != PSRETURN_OK)
		{
			JS_ReportError(cx, "Invalid GUIObject property '%s'", propName.c_str());
			return false;
		}

		// (All the cases are in {...} to avoid scoping problems)
		switch (Type)
		{
		case GUIST_bool:
		{
			bool value;
			GUI<bool>::GetSetting(e, propName, value);
			vp.set(JS::BooleanValue(value));
			break;
		}

		case GUIST_int:
		{
			int value;
			GUI<int>::GetSetting(e, propName, value);
			vp.set(JS::Int32Value(value));
			break;
		}

		case GUIST_uint:
		{
			u32 value;
			GUI<u32>::GetSetting(e, propName, value);
			if (value >= std::numeric_limits<u32>::max())
				LOGERROR("Integer overflow on converting to GUIST_uint");
			else
				vp.set(JS::Int32Value(value));
			break;
		}

		case GUIST_float:
		{
			float value;
			GUI<float>::GetSetting(e, propName, value);
			// Create a garbage-collectable double
			vp.set(JS::NumberValue(value));
			return !vp.isNull();
		}

		case GUIST_CColor:
		{
			CColor color;
			GUI<CColor>::GetSetting(e, propName, color);
			JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUIColor"));
			vp.setObject(*obj);
			JS::RootedValue c(cx);
			// Attempt to minimise ugliness through macrosity
#define P(x) \
	c = JS::NumberValue(color.x); \
	if (c.isNull()) \
		return false; \
	JS_SetProperty(cx, obj, #x, c)

			P(r);
			P(g);
			P(b);
			P(a);
#undef P
			break;
		}

		case GUIST_CClientArea:
		{
			CClientArea area;
			GUI<CClientArea>::GetSetting(e, propName, area);

			JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUISize"));
			vp.setObject(*obj);
			try
			{
#define P(x, y, z) pScriptInterface->SetProperty(vp, #z, area.x.y, false, true)
				P(pixel,	left,	left);
				P(pixel,	top,	top);
				P(pixel,	right,	right);
				P(pixel,	bottom,	bottom);
				P(percent,	left,	rleft);
				P(percent,	top,	rtop);
				P(percent,	right,	rright);
				P(percent,	bottom,	rbottom);
#undef P
			}
			catch (PSERROR_Scripting_ConversionFailed&)
			{
				debug_warn(L"Error creating size object!");
				break;
			}

			break;
		}

		case GUIST_CGUIString:
		{
			CGUIString value;
			GUI<CGUIString>::GetSetting(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value.GetOriginalString());
			break;
		}

		case GUIST_CStr:
		{
			CStr value;
			GUI<CStr>::GetSetting(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value);
			break;
		}

		case GUIST_CStrW:
		{
			CStrW value;
			GUI<CStrW>::GetSetting(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value);
			break;
		}

		case GUIST_CGUISpriteInstance:
		{
			CGUISpriteInstance *value;
			GUI<CGUISpriteInstance>::GetSettingPointer(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value->GetName());
			break;
		}

		case GUIST_EAlign:
		{
			EAlign value;
			GUI<EAlign>::GetSetting(e, propName, value);
			CStr word;
			switch (value)
			{
			case EAlign_Left: word = "left"; break;
			case EAlign_Right: word = "right"; break;
			case EAlign_Center: word = "center"; break;
			default: debug_warn(L"Invalid EAlign!"); word = "error"; break;
			}
			ScriptInterface::ToJSVal(cx, vp, word);
			break;
		}

		case GUIST_EVAlign:
		{
			EVAlign value;
			GUI<EVAlign>::GetSetting(e, propName, value);
			CStr word;
			switch (value)
			{
			case EVAlign_Top: word = "top"; break;
			case EVAlign_Bottom: word = "bottom"; break;
			case EVAlign_Center: word = "center"; break;
			default: debug_warn(L"Invalid EVAlign!"); word = "error"; break;
			}
			ScriptInterface::ToJSVal(cx, vp, word);
			break;
		}

		case GUIST_CGUIList:
		{
			CGUIList value;
			GUI<CGUIList>::GetSetting(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value.m_Items);
			break;
		}

		case GUIST_CGUISeries:
		{
			CGUISeries value;
			GUI<CGUISeries>::GetSetting(e, propName, value);
			ScriptInterface::ToJSVal(cx, vp, value.m_Series);
			break;
		}

		default:
			JS_ReportError(cx, "Setting '%s' uses an unimplemented type", propName.c_str());
			DEBUG_WARN_ERR(ERR::LOGIC);
			return false;
		}

		return true;
	}
}

bool JSI_IGUIObject::setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool UNUSED(strict), JS::MutableHandleValue vp)
{
	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, obj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	JSAutoRequest rq(cx);
	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return false;

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return false;

	if (propName == "name")
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;
		e->SetName(value);
		return true;
	}

	JS::RootedObject vpObj(cx);
	if (vp.isObject())
		vpObj = &vp.toObject();

	// Use onWhatever to set event handlers
	if (propName.substr(0, 2) == "on")
	{
		if (vp.isPrimitive() || vp.isNull() || !JS_ObjectIsFunction(cx, &vp.toObject()))
		{
			JS_ReportError(cx, "on- event-handlers must be functions");
			return false;
		}

		CStr eventName(CStr(propName.substr(2)).LowerCase());
		e->SetScriptHandler(eventName, vpObj);

		return true;
	}

	// Retrieve the setting's type (and make sure it actually exists)
	EGUISettingType Type;
	if (e->GetSettingType(propName, Type) != PSRETURN_OK)
	{
		JS_ReportError(cx, "Invalid setting '%s'", propName.c_str());
		return true;
	}

	switch (Type)
	{
	case GUIST_CStr:
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		GUI<CStr>::SetSetting(e, propName, value);
		break;
	}

	case GUIST_CStrW:
	{
		std::wstring value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		GUI<CStrW>::SetSetting(e, propName, value);
		break;
	}

	case GUIST_CGUISpriteInstance:
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		GUI<CGUISpriteInstance>::SetSetting(e, propName, CGUISpriteInstance(value));
		break;
	}

	case GUIST_CGUIString:
	{
		std::wstring value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		CGUIString str;
		str.SetValue(value);
		GUI<CGUIString>::SetSetting(e, propName, str);
		break;
	}

	case GUIST_EAlign:
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		EAlign a;
		if (value == "left") a = EAlign_Left;
		else if (value == "right") a = EAlign_Right;
		else if (value == "center" || value == "centre") a = EAlign_Center;
		else
		{
			JS_ReportError(cx, "Invalid alignment (should be 'left', 'right' or 'center')");
			return false;
		}
		GUI<EAlign>::SetSetting(e, propName, a);
		break;
	}

	case GUIST_EVAlign:
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return false;

		EVAlign a;
		if (value == "top") a = EVAlign_Top;
		else if (value == "bottom") a = EVAlign_Bottom;
		else if (value == "center" || value == "centre") a = EVAlign_Center;
		else
		{
			JS_ReportError(cx, "Invalid alignment (should be 'top', 'bottom' or 'center')");
			return false;
		}
		GUI<EVAlign>::SetSetting(e, propName, a);
		break;
	}

	case GUIST_int:
	{
		int value;
		if (ScriptInterface::FromJSVal(cx, vp, value))
			GUI<int>::SetSetting(e, propName, value);
		else
		{
			JS_ReportError(cx, "Cannot convert value to int");
			return false;
		}
		break;
	}

	case GUIST_uint:
	{
		u32 value;
		if (ScriptInterface::FromJSVal(cx, vp, value))
			GUI<u32>::SetSetting(e, propName, value);
		else
		{
			JS_ReportError(cx, "Cannot convert value to u32");
			return false;
		}
		break;
	}

	case GUIST_float:
	{
		double value;
		if (JS::ToNumber(cx, vp, &value) == true)
			GUI<float>::SetSetting(e, propName, (float)value);
		else
		{
			JS_ReportError(cx, "Cannot convert value to float");
			return false;
		}
		break;
	}

	case GUIST_bool:
	{
		bool value = JS::ToBoolean(vp);
		GUI<bool>::SetSetting(e, propName, value);
		break;
	}

	case GUIST_CClientArea:
	{
		if (vp.isString())
		{
			std::wstring value;
			if (!ScriptInterface::FromJSVal(cx, vp, value))
				return false;

			if (e->SetSetting(propName, value) != PSRETURN_OK)
			{
				JS_ReportError(cx, "Invalid value for setting '%s'", propName.c_str());
				return false;
			}
		}
		else if (vp.isObject() && JS_InstanceOf(cx, vpObj, &JSI_GUISize::JSI_class, NULL))
		{
			CClientArea area;
			GUI<CClientArea>::GetSetting(e, propName, area);

			ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
#define P(x, y, z) pScriptInterface->GetProperty(vp, #z, area.x.y)
			P(pixel,	left,	left);
			P(pixel,	top,	top);
			P(pixel,	right,	right);
			P(pixel,	bottom,	bottom);
			P(percent,	left,	rleft);
			P(percent,	top,	rtop);
			P(percent,	right,	rright);
			P(percent,	bottom,	rbottom);
#undef P

			GUI<CClientArea>::SetSetting(e, propName, area);
		}
		else
		{
			JS_ReportError(cx, "Size only accepts strings or GUISize objects");
			return false;
		}
		break;
	}

	case GUIST_CColor:
	{
		if (vp.isString())
		{
			std::wstring value;
			if (!ScriptInterface::FromJSVal(cx, vp, value))
				return false;

			if (e->SetSetting(propName, value) != PSRETURN_OK)
			{
				JS_ReportError(cx, "Invalid value for setting '%s'", propName.c_str());
				return false;
			}
		}
		else if (vp.isObject() && JS_InstanceOf(cx, vpObj, &JSI_GUIColor::JSI_class, NULL))
		{
			CColor color;
			JS::RootedValue t(cx);
			double s;
#define PROP(x) \
	JS_GetProperty(cx, vpObj, #x, &t); \
	s = t.toDouble(); \
	color.x = (float)s

			PROP(r);
			PROP(g);
			PROP(b);
			PROP(a);
#undef PROP
			GUI<CColor>::SetSetting(e, propName, color);
		}
		else
		{
			JS_ReportError(cx, "Color only accepts strings or GUIColor objects");
			return false;
		}
		break;
	}

	case GUIST_CGUIList:
	{
		CGUIList list;
		if (ScriptInterface::FromJSVal(cx, vp, list.m_Items))
			GUI<CGUIList>::SetSetting(e, propName, list);
		else
		{
			JS_ReportError(cx, "Failed to get list '%s'", propName.c_str());
			return false;
		}
		break;
	}

	case GUIST_CGUISeries:
	{
		CGUISeries series;
		if (ScriptInterface::FromJSVal(cx, vp, series.m_Series))
			GUI<CGUISeries>::SetSetting(e, propName, series);
		else
		{
			JS_ReportError(cx, "Invalid value for chart series '%s'", propName.c_str());
			return false;
		}
		break;
	}

	default:
		JS_ReportError(cx, "Setting '%s' uses an unimplemented type", propName.c_str());
		break;
	}

	return !JS_IsExceptionPending(cx);
}


bool JSI_IGUIObject::construct(JSContext* cx, uint argc, JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;

	if (args.length() == 0)
	{
		JS_ReportError(cx, "GUIObject has no default constructor");
		return false;
	}

	JS::RootedObject obj(cx, pScriptInterface->CreateCustomObject("GUIObject"));

	// Store the IGUIObject in the JS object's 'private' area
	IGUIObject* guiObject = (IGUIObject*)args[0].get().toPrivate();
	JS_SetPrivate(obj, guiObject);

	args.rval().setObject(*obj);
	return true;
}

void JSI_IGUIObject::init(ScriptInterface& scriptInterface)
{
	scriptInterface.DefineCustomObjectType(&JSI_class, construct, 1, nullptr, JSI_methods, nullptr, nullptr);
}

bool JSI_IGUIObject::toString(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	char buffer[256];
	snprintf(buffer, 256, "[GUIObject: %s]", e->GetName().c_str());
	buffer[255] = 0;
	rec.rval().setString(JS_NewStringCopyZ(cx, buffer));
	return true;
}

bool JSI_IGUIObject::focus(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->GetGUI()->SetFocusedObject(e);

	rec.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::blur(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->GetGUI()->SetFocusedObject(NULL);

	rec.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::getTextSize(JSContext* cx, uint argc, JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	JS::RootedObject thisObj(cx, &args.thisv().toObject());

	IGUIObject* obj = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);

	if (!obj || !obj->SettingExists("caption"))
		return false;

	CStrW font;
	if (GUI<CStrW>::GetSetting(obj, "font", font) != PSRETURN_OK || font.empty())
		font = L"default";

	CGUIString caption;
	EGUISettingType Type;
	obj->GetSettingType("caption", Type);
	if (Type == GUIST_CGUIString)
		// CText, CButton, CCheckBox, CRadioButton
		GUI<CGUIString>::GetSetting(obj, "caption", caption);
	else if (Type == GUIST_CStrW)
	{
		// CInput
		CStrW captionStr;
		GUI<CStrW>::GetSetting(obj, "caption", captionStr);
		caption.SetValue(captionStr);
	}
	else
		return false;

	obj->UpdateCachedSize();
	float width = obj->m_CachedActualSize.GetWidth();

	if (obj->SettingExists("scrollbar"))
	{
		bool scrollbar;
		GUI<bool>::GetSetting(obj, "scrollbar", scrollbar);
		if (scrollbar)
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(obj, "scrollbar_style", scrollbar_style);
			const SGUIScrollBarStyle* scrollbar_style_object = obj->GetGUI()->GetScrollBarStyle(scrollbar_style);
			if (scrollbar_style_object)
				width -= scrollbar_style_object->m_Width;
		}
	}

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(obj, "buffer_zone", buffer_zone);
	SGUIText text = obj->GetGUI()->GenerateText(caption, font, width, buffer_zone, obj);

	JS::RootedValue objVal(cx, JS::ObjectValue(*JS_NewPlainObject(cx)));
	try
	{
		ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
		pScriptInterface->SetProperty(objVal, "width", text.m_Size.cx, false, true);
		pScriptInterface->SetProperty(objVal, "height", text.m_Size.cy, false, true);
	}
	catch (PSERROR_Scripting_ConversionFailed&)
	{
		debug_warn(L"Error creating size object!");
		return false;
	}

	rec.rval().set(objVal);
	return true;
}

bool JSI_IGUIObject::getComputedSize(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JSAutoRequest rq(cx);
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);

	JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp));

	IGUIObject* e = (IGUIObject*)JS_GetInstancePrivate(cx, thisObj, &JSI_IGUIObject::JSI_class, NULL);
	if (!e)
		return false;

	e->UpdateCachedSize();
	CRect size = e->m_CachedActualSize;

	JS::RootedValue objVal(cx, JS::ObjectValue(*JS_NewPlainObject(cx)));
	try
	{
		ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
		pScriptInterface->SetProperty(objVal, "left", size.left, false, true);
		pScriptInterface->SetProperty(objVal, "right", size.right, false, true);
		pScriptInterface->SetProperty(objVal, "top", size.top, false, true);
		pScriptInterface->SetProperty(objVal, "bottom", size.bottom, false, true);
	}
	catch (PSERROR_Scripting_ConversionFailed&)
	{
		debug_warn(L"Error creating size object!");
		return false;
	}

	rec.rval().set(objVal);
	return true;
}
