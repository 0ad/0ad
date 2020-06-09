/* Copyright (C) 2020 Wildfire Games.
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

#include "gui/CGUI.h"
#include "gui/CGUISetting.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/ScriptInterface.h"

JSClassOps JSI_IGUIObject::classOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

js::ObjectOps JSI_IGUIObject::objOps = {
	nullptr, nullptr, nullptr,
	JSI_IGUIObject::getProperty,
	JSI_IGUIObject::setProperty,
	nullptr, 
    JSI_IGUIObject::deleteProperty,
    nullptr, nullptr};

JSClass JSI_IGUIObject::JSI_class = {
	"GUIObject", JSCLASS_HAS_PRIVATE,
    &JSI_IGUIObject::classOps,
    nullptr, nullptr,
    &JSI_IGUIObject::objOps
};

JSFunctionSpec JSI_IGUIObject::JSI_methods[] =
{
	JS_FN("toString", JSI_IGUIObject::toString, 0, 0),
	JS_FN("focus", JSI_IGUIObject::focus, 0, 0),
	JS_FN("blur", JSI_IGUIObject::blur, 0, 0),
	JS_FN("getComputedSize", JSI_IGUIObject::getComputedSize, 0, 0),
	JS_FS_END
};

void JSI_IGUIObject::RegisterScriptClass(ScriptInterface& scriptInterface)
{
	scriptInterface.DefineCustomObjectType(&JSI_class, nullptr, 0, nullptr, JSI_methods, nullptr, nullptr);
}

bool JSI_IGUIObject::getProperty(JSContext* cx, JS::HandleObject obj, JS::Handle<JS::Value>, JS::HandleId id, JS::MutableHandleValue vp)
{
	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;

	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, obj, &JSI_IGUIObject::JSI_class);
	if (!e)
		return false;

	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return false;

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return false;

	// TODO: This is slightly inefficient (we're going through the GUI via a string lookup).
	// We could do better by templating the proxy with function names to look for in the Factory.
	// (the find can't fail)
	const std::map<std::string, JS::PersistentRootedFunction>& factory = e->m_pGUI.m_ObjectTypes.find(e->GetObjectType())->second.guiObjectFactory->m_FunctionHandlers;
	std::map<std::string, JS::PersistentRootedFunction>::const_iterator it = factory.find(propName);
	if (it != factory.end())
	{
		JSObject* obj = JS_GetFunctionObject(it->second.get());
		vp.setObjectOrNull(obj);
		return true;
	}

	// Use onWhatever to access event handlers
	if (propName.substr(0, 2) == "on")
	{
		CStr eventName(propName.substr(2));
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
		ScriptInterface::CreateArray(cx, vp);

		for (size_t i = 0; i < e->m_Children.size(); ++i)
			pScriptInterface->SetPropertyInt(vp, i, e->m_Children[i]);

		return true;
	}
	else if (propName == "name")
	{
		ScriptInterface::ToJSVal(cx, vp, e->GetName());
		return true;
	}
	else if (e->SettingExists(propName))
	{
		e->m_Settings[propName]->ToJSVal(cx, vp);
		return true;
	}

	JS_ReportErrorASCII(cx, "Property '%s' does not exist!", propName.c_str());
	return false;
}

bool JSI_IGUIObject::setProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::HandleValue vp, JS::HandleValue,  JS::ObjectOpResult& result)
{
	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, obj, &JSI_IGUIObject::JSI_class);
	if (!e)
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return result.fail(JSMSG_UNDEFINED_PROP);

	if (propName == "name")
	{
		std::string value;
		if (!ScriptInterface::FromJSVal(cx, vp, value))
			return result.fail(JSMSG_UNDEFINED_PROP);
		e->SetName(value);
		return result.succeed();
	}

	JS::RootedObject vpObj(cx);
	if (vp.isObject())
		vpObj = &vp.toObject();

	// Use onWhatever to set event handlers
	if (propName.substr(0, 2) == "on")
	{
		if (vp.isPrimitive() || vp.isNull() || !JS_ObjectIsFunction(&vp.toObject()))
		{
			JS_ReportErrorASCII(cx, "on- event-handlers must be functions");
			return result.fail(JSMSG_NOT_FUNCTION);
		}

		CStr eventName(propName.substr(2));
		e->SetScriptHandler(eventName, vpObj);

		return result.succeed();
	}

	if (e->SettingExists(propName))
		return e->m_Settings[propName]->FromJSVal(cx, vp, true) ? result.succeed() : result.fail(JSMSG_TYPEDOBJECT_BAD_ARGS);

	JS_ReportErrorASCII(cx, "Property '%s' does not exist!", propName.c_str());
	return result.fail(JSMSG_UNDEFINED_PROP);
}

bool JSI_IGUIObject::deleteProperty(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::ObjectOpResult& result)
{
	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, obj, &JSI_IGUIObject::JSI_class);
	if (!e)
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	JS::RootedValue idval(cx);
	if (!JS_IdToValue(cx, id, &idval))
		return result.fail(JSMSG_NOT_NONNULL_OBJECT);

	std::string propName;
	if (!ScriptInterface::FromJSVal(cx, idval, propName))
		return result.fail(JSMSG_UNDEFINED_PROP);

	// event handlers
	if (propName.substr(0, 2) == "on")
	{
		CStr eventName(propName.substr(2));
		e->UnsetScriptHandler(eventName);
		return result.succeed();
	}

	JS_ReportErrorASCII(cx, "Only event handlers can be deleted from GUI objects!");
	return result.fail(JSMSG_UNDEFINED_PROP);
}

bool JSI_IGUIObject::toString(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, args, &JSI_IGUIObject::JSI_class);
	if (!e)
		return false;

	ScriptInterface::ToJSVal(cx, args.rval(), "[GUIObject: " + e->GetName() + "]");
	return true;
}

bool JSI_IGUIObject::focus(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, args, &JSI_IGUIObject::JSI_class);
	if (!e)
		return false;

	e->GetGUI().SetFocusedObject(e);
	args.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::blur(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, args, &JSI_IGUIObject::JSI_class);
	if (!e)
		return false;

	e->GetGUI().SetFocusedObject(nullptr);
	args.rval().setUndefined();
	return true;
}

bool JSI_IGUIObject::getComputedSize(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	IGUIObject* e = ScriptInterface::GetPrivate<IGUIObject>(cx, args, &JSI_IGUIObject::JSI_class);
	if (!e)
		return false;

	e->UpdateCachedSize();
	ScriptInterface::ToJSVal(cx, args.rval(), e->m_CachedActualSize);

	return true;
}
