/* Copyright (C) 2023 Wildfire Games.
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

// This file is included directly into actual implementation files.

#include "JSInterface_GUIProxy.h"

#include "gui/CGUI.h"
#include "gui/CGUISetting.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "ps/CLogger.h"
#include "scriptinterface/FunctionWrapper.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/ScriptRequest.h"

#include <string>
#include <string_view>

template <typename T>
JSI_GUIProxy<T>& JSI_GUIProxy<T>::Singleton()
{
	static JSI_GUIProxy<T> s;
	return s;
}

// Call this for every specialised type. You will need to override IGUIObject::CreateJSObject() in your class interface.
#define DECLARE_GUIPROXY(Type) \
void Type::CreateJSObject() \
{ \
	ScriptRequest rq(m_pGUI.GetScriptInterface()); \
	using ProxyHandler = JSI_GUIProxy<std::remove_pointer_t<decltype(this)>>; \
	m_JSObject = ProxyHandler::CreateJSObject(rq, this, GetGUI().GetProxyData(&ProxyHandler::Singleton())); \
} \
template class JSI_GUIProxy<Type>;

// Use a common namespace to avoid duplicating the symbols un-necessarily.
namespace JSInterface_GUIProxy
{
// All proxy objects share a class definition.
JSClass& ClassDefinition()
{
	static JSClass c = PROXY_CLASS_DEF("GUIObjectProxy", JSCLASS_HAS_CACHED_PROTO(JSProto_Proxy) | JSCLASS_HAS_RESERVED_SLOTS(1));
	return c;
}

// Default implementation of the cache via unordered_map
class MapCache : public GUIProxyProps
{
public:
	virtual ~MapCache() {};

	virtual bool has(const std::string& name) const override
	{
		return m_Functions.find(name) != m_Functions.end();
	}

	virtual JSObject* get(const std::string& name) const override
	{
		return m_Functions.at(name).get();
	}

	virtual bool setFunction(const ScriptRequest& rq, const std::string& name, JSFunction* function) override
	{
		m_Functions[name].init(rq.cx, JS_GetFunctionObject(function));
		return true;
	}

protected:
	std::unordered_map<std::string, JS::PersistentRootedObject> m_Functions;
};
}

template<>
IGUIObject* IGUIProxyObject::FromPrivateSlot<IGUIObject>(JSObject* obj)
{
	if (!obj)
		return nullptr;
	if (JS::GetClass(obj) != &JSInterface_GUIProxy::ClassDefinition())
		return nullptr;
	return UnsafeFromPrivateSlot<IGUIObject>(obj);
}

// The default propcache is a MapCache.
template<typename T>
struct JSI_GUIProxy<T>::PropCache
{
	using type = JSInterface_GUIProxy::MapCache;
};

template <typename T>
T* JSI_GUIProxy<T>::FromPrivateSlot(const ScriptRequest&, JS::CallArgs& args)
{
	// Call the unsafe version - this is only ever called from actual proxy objects.
	return IGUIProxyObject::UnsafeFromPrivateSlot<T>(args.thisv().toObjectOrNull());
}

template <typename T>
bool JSI_GUIProxy<T>::PropGetter(JS::HandleObject proxy, const std::string& propName, JS::MutableHandleValue vp) const
{
	using PropertyCache = typename PropCache::type;
	// Since we know at compile time what the type actually is, avoid the virtual call.
	const PropertyCache* data = static_cast<const PropertyCache*>(static_cast<const GUIProxyProps*>(js::GetProxyReservedSlot(proxy, 0).toPrivate()));
	if (data->has(propName))
	{
		vp.setObjectOrNull(data->get(propName));
		return true;
	}
	return false;
}

template <typename T>
std::pair<const js::BaseProxyHandler*, GUIProxyProps*> JSI_GUIProxy<T>::CreateData(ScriptInterface& scriptInterface)
{
	using PropertyCache = typename PropCache::type;
	PropertyCache* data = new PropertyCache();
	ScriptRequest rq(scriptInterface);

	// Functions common to all children of IGUIObject.
	JSI_GUIProxy<IGUIObject>::CreateFunctions(rq, data);

	// Let derived classes register their own interface.
	if constexpr (!std::is_same_v<T, IGUIObject>)
		CreateFunctions(rq, data);
	return { &Singleton(), data };
}

template<typename T>
template<auto callable>
void JSI_GUIProxy<T>::CreateFunction(const ScriptRequest& rq, GUIProxyProps* cache, const std::string& name)
{
	cache->setFunction(rq, name, ScriptFunction::Create<callable, FromPrivateSlot>(rq, name.c_str()));
}

template<typename T>
std::unique_ptr<IGUIProxyObject> JSI_GUIProxy<T>::CreateJSObject(const ScriptRequest& rq, T* ptr, GUIProxyProps* dataPtr)
{
	js::ProxyOptions options;
	options.setClass(&JSInterface_GUIProxy::ClassDefinition());

	auto ret = std::make_unique<IGUIProxyObject>();
	ret->m_Ptr = static_cast<IGUIObject*>(ptr);

	JS::RootedValue cppObj(rq.cx), data(rq.cx);
	cppObj.get().setPrivate(ret->m_Ptr);
	data.get().setPrivate(static_cast<void*>(dataPtr));
	ret->m_Object.init(rq.cx, js::NewProxyObject(rq.cx, &Singleton(), cppObj, nullptr, options));
	js::SetProxyReservedSlot(ret->m_Object, 0, data);
	return ret;
}

template <typename T>
bool JSI_GUIProxy<T>::get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue UNUSED(receiver), JS::HandleId id, JS::MutableHandleValue vp) const
{
	ScriptRequest rq(cx);

	T* e = IGUIProxyObject::FromPrivateSlot<T>(proxy.get());
	if (!e)
		return false;

	JS::RootedValue idval(rq.cx);
	if (!JS_IdToValue(rq.cx, id, &idval))
		return false;

	std::string propName;
	if (!Script::FromJSVal(rq, idval, propName))
		return false;

	// Return function properties. Specializable.
	if (PropGetter(proxy, propName, vp))
		return true;

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
		Script::CreateArray(rq, vp);

		for (size_t i = 0; i < e->m_Children.size(); ++i)
			Script::SetPropertyInt(rq, vp, i, e->m_Children[i]);

		return true;
	}
	else if (propName == "name")
	{
		Script::ToJSVal(rq, vp, e->GetName());
		return true;
	}
	else if (e->SettingExists(propName))
	{
		e->m_Settings[propName]->ToJSVal(rq, vp);
		return true;
	}

	LOGERROR("Property '%s' does not exist!", propName.c_str());
	return false;
}


template <typename T>
bool JSI_GUIProxy<T>::set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue vp,
							JS::HandleValue UNUSED(receiver), JS::ObjectOpResult& result) const
{
	T* e = IGUIProxyObject::FromPrivateSlot<T>(proxy.get());
	if (!e)
	{
		LOGERROR("C++ GUI Object could not be found");
		return result.fail(JSMSG_OBJECT_REQUIRED);
	}

	ScriptRequest rq(cx);

	JS::RootedValue idval(rq.cx);
	if (!JS_IdToValue(rq.cx, id, &idval))
		return result.fail(JSMSG_BAD_PROP_ID);

	std::string propName;
	if (!Script::FromJSVal(rq, idval, propName))
		return result.fail(JSMSG_BAD_PROP_ID);

	if (propName == "name")
	{
		std::string value;
		if (!Script::FromJSVal(rq, vp, value))
			return result.fail(JSMSG_BAD_PROP_ID);
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
			LOGERROR("on- event-handlers must be functions");
			return result.fail(JSMSG_NOT_FUNCTION);
		}

		CStr eventName(propName.substr(2));
		e->SetScriptHandler(eventName, vpObj);

		return result.succeed();
	}

	if (e->SettingExists(propName))
		return e->m_Settings[propName]->FromJSVal(rq, vp, true) ? result.succeed() : result.fail(JSMSG_USER_DEFINED_ERROR);

	LOGERROR("Property '%s' does not exist!", propName.c_str());
	return result.fail(JSMSG_BAD_PROP_ID);
}

template<typename T>
bool JSI_GUIProxy<T>::delete_(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const
{
	T* e = IGUIProxyObject::FromPrivateSlot<T>(proxy.get());
	if (!e)
	{
		LOGERROR("C++ GUI Object could not be found");
		return result.fail(JSMSG_OBJECT_REQUIRED);
	}

	ScriptRequest rq(cx);

	JS::RootedValue idval(rq.cx);
	if (!JS_IdToValue(rq.cx, id, &idval))
		return result.fail(JSMSG_BAD_PROP_ID);

	std::string propName;
	if (!Script::FromJSVal(rq, idval, propName))
		return result.fail(JSMSG_BAD_PROP_ID);

	// event handlers
	if (std::string_view{propName}.substr(0, 2) == "on")
	{
		CStr eventName(propName.substr(2));
		e->UnsetScriptHandler(eventName);
		return result.succeed();
	}

	LOGERROR("Only event handlers can be deleted from GUI objects!");
	return result.fail(JSMSG_BAD_PROP_ID);
}
