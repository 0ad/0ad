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

#include <type_traits>

#include "gui/CGUI.h"
#include "gui/CGUISetting.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectTypes/CText.h"

/**
 * Convenient struct to get info on a [const] function pointer.
 */
template<typename ptr>
struct args_info;

template<typename C, typename R, typename ...Types>
struct args_info<R(C::*)(Types ...)>
{
	static const size_t nb_args = sizeof...(Types);
	using return_type = R;
	using object_type = C;
	using args = std::tuple<typename std::remove_const<typename std::remove_reference<Types>::type>::type...>;
 };
 
// TODO: would be nice to find a way around the duplication here.
template<typename C, typename R, typename ...Types>
struct args_info<R(C::*)(Types ...) const>
 {
	static const size_t nb_args = sizeof...(Types);
	using return_type = R;
	using object_type = C;
	using args = std::tuple<typename std::remove_const<typename std::remove_reference<Types>::type>::type...>;
 };
 
// Convenience wrapper since the code is a little verbose.
// TODO: I think c++14 makes this clean enough, with type deduction, that it could be removed.
#define SetupHandler(funcPtr, JSName) \
	m_FunctionHandlers[JSName].init( \
		scriptInterface.GetContext(), \
		JS_NewFunction(scriptInterface.GetContext(), &(scriptMethod<cppType, decltype(funcPtr), funcPtr>), args_info<decltype(funcPtr)>::nb_args, 0, JSName) \
	);

JSI_GUI::GUIObjectFactory::GUIObjectFactory(ScriptInterface& scriptInterface)
{
	CX_IN_REALM(cx, &scriptInterface);
	SetupHandler(&IGUIObject::toString, "toString");
	SetupHandler(&IGUIObject::toString, "toSource");
	SetupHandler(&IGUIObject::focus, "focus");
	SetupHandler(&IGUIObject::blur, "blur");
	SetupHandler(&IGUIObject::getComputedSize, "getComputedSize");
}


JSI_GUI::TextObjectFactory::TextObjectFactory(ScriptInterface& scriptInterface) : JSI_GUI::GUIObjectFactory(scriptInterface)
 {
	CX_IN_REALM(cx, &scriptInterface);
    SetupHandler(&CText::GetTextSize, "getTextSize");
 }
 
#undef SetupHandler

/**
 * Based on https://stackoverflow.com/a/32223343
 * make_index_sequence is not defined in C++11... Only C++14.
 */
template <size_t... Ints>
struct index_sequence
{
	using type = index_sequence;
	using value_type = size_t;
	static constexpr std::size_t size() noexcept { return sizeof...(Ints); }
};
template <class Sequence1, class Sequence2>
struct _merge_and_renumber;
template <size_t... I1, size_t... I2>
struct _merge_and_renumber<index_sequence<I1...>, index_sequence<I2...>>
: index_sequence<I1..., (sizeof...(I1)+I2)...> { };
template <size_t N>
struct make_index_sequence
: _merge_and_renumber<typename make_index_sequence<N/2>::type,
typename make_index_sequence<N - N/2>::type> { };
template<> struct make_index_sequence<0> : index_sequence<> { };
template<> struct make_index_sequence<1> : index_sequence<0> { };


/**
 * This series of templates is a setup to transparently call a c++ function
 * from a JS function (with CallArgs) and returning that value.
 * The C++ code can have arbitrary arguments and arbitrary return types, so long
 * as they can be converted to/from JS.
 */

/**
 * This helper is a recursive template call that converts the N-1th argument
 * of the function from a JS value to its proper C++ type.
 */
template<int N, typename tuple>
struct convertFromJS
{
	bool operator()(ScriptInterface& interface, JSContext* cx_, JS::CallArgs& val, tuple& outs)
	{
	    CX_IN_REALM(cx, &interface);
		if (!interface.FromJSVal(cx, val[N-1], std::get<N-1>(outs)))
			return false;
		return convertFromJS<N-1, tuple>()(interface, cx, val, outs);
	}
};
// Specialization for the base case "no arguments".
template<typename tuple>
struct convertFromJS<0, tuple>
{
	bool operator()(ScriptInterface& UNUSED(interface), JSContext* UNUSED(cx), JS::CallArgs& UNUSED(val), tuple& UNUSED(outs))
	{
		return true;
	}
};

/**
 * These two templates take a function pointer, its arguments, call it,
 * and set the return value of the CallArgs to whatever it returned, if anything.
 * It's tag-dispatched for the "returns_void" and the regular return case.
 */
template <typename funcPtr, funcPtr callable, typename T, size_t... Is, typename... types>
void call(T* object, ScriptInterface* scriptInterface, JS::CallArgs& callArgs, std::tuple<types...>& args, std::false_type, index_sequence<Is...>)
{
	CX_IN_REALM(cx, scriptInterface);
	// This is perfectly readable, what are you talking about.
	auto ret = ((*object).* callable)(std::get<Is>(args)...);
	scriptInterface->ToJSVal(cx, callArgs.rval(), ret);
}

template <typename funcPtr, funcPtr callable, typename T, size_t... Is, typename... types>
void call(T* object, ScriptInterface* UNUSED(scriptInterface), JS::CallArgs& UNUSED(callArgs), std::tuple<types...>& args, std::true_type, index_sequence<Is...>)
{
	// Void return specialization, just call the function.
	((*object).* callable)(std::get<Is>(args)...);
}

template <typename funcPtr, funcPtr callable, typename T, size_t N = args_info<funcPtr>::nb_args, typename tuple = typename args_info<funcPtr>::args>
bool JSToCppCall(T* object, JSContext* cx, JS::CallArgs& args)
{
	ScriptInterface* scriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
	// This is where the magic happens: instantiate a tuple to store the converted JS arguments,
	// then 'unpack' the tuple to call the C++ function, convert & store the return value.
	tuple outs;
	if (!convertFromJS<N, tuple>()(*scriptInterface, cx, args, outs))
		return false;
	// TODO: We have no failure handling here. It's non trivial in a generic sense since we may return a value.
	// We could either try-catch and throw exceptions,
	// or come C++17 return an std::optional/maybe or some kind of [bool, val] structured binding.
	using returns_void = std::is_same<typename args_info<funcPtr>::return_type, void>;
	call<funcPtr, callable, T>(object, scriptInterface, args, outs, returns_void{}, make_index_sequence<N>{});
	return true;
}

// TODO: this can get rewritten as <auto> and deduced come c++14
template <typename objType, typename funcPtr, funcPtr callable>
bool JSI_GUI::GUIObjectFactory::scriptMethod(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	static_assert(std::is_same<objType, typename args_info<funcPtr>::object_type>::value,
				  "The called method is not defined on the factory's cppType. You most likely forgot to define 'using cppType = ...'");

    void* ptr = js::GetProxyReservedSlot(args.thisv().toObjectOrNull(), 0).toPrivate();
	objType* thisObj = static_cast<objType*>(ptr);
	if (!thisObj)
		return false;

	if (!JSToCppCall<decltype(callable), callable>(thisObj, cx, args))
		return false;

	return true;
}

js::Class JSI_GUI::GUIObjectFactory::m_ProxyObjectClass = \
	PROXY_CLASS_DEF("GUIObjectProxy", JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Proxy));

JSObject* JSI_GUI::GUIObjectFactory::CreateObject(JSContext* cx)
{
	js::ProxyOptions options;
	options.setClass(&m_ProxyObjectClass);
	JS::RootedObject proxy(cx, js::NewProxyObject(cx, &JSI_GUI::GUIProxy::singleton, JS::NullHandleValue, nullptr, options));
	return proxy;
}

JSI_GUI::GUIProxy JSI_GUI::GUIProxy::singleton;

// The family can't be nullptr because that's used for some DOM object and it crashes.
JSI_GUI::GUIProxy::GUIProxy() : BaseProxyHandler(this, false, false) {};

bool JSI_GUI::GUIProxy::get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue UNUSED(receiver), JS::HandleId id, JS::MutableHandleValue vp) const
 {
 	ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface;
 
    void* ptr = js::GetProxyReservedSlot(proxy.get(), 0).toPrivate();
	IGUIObject* e = static_cast<IGUIObject*>(ptr);
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
		std::map<CStr, JS::Heap<JSFunction*>>::iterator it = e->m_ScriptHandlers.find(eventName);
		if (it == e->m_ScriptHandlers.end())
			vp.setNull();
		else
			vp.setObject(*JS_GetFunctionObject(it->second.get()));
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

bool JSI_GUI::GUIProxy::set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue vp,
							 JS::HandleValue UNUSED(receiver), JS::ObjectOpResult& result) const
 {
    void* ptr = js::GetProxyReservedSlot(proxy.get(), 0).toPrivate();
	IGUIObject* e = static_cast<IGUIObject*>(ptr);

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
		e->SetScriptHandler(eventName, JS_ValueToFunction(cx, vp));

		return result.succeed();
	}

	if (e->SettingExists(propName))
		return e->m_Settings[propName]->FromJSVal(cx, vp, true) ? result.succeed() : result.fail(JSMSG_TYPEDOBJECT_BAD_ARGS);

	JS_ReportErrorASCII(cx, "Property '%s' does not exist!", propName.c_str());
	return result.fail(JSMSG_UNDEFINED_PROP);
}

bool JSI_GUI::GUIProxy::delete_(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const
{	
    IGUIObject* e = static_cast<IGUIObject*>(js::GetProxyReservedSlot(proxy.get(), 0).toPrivate());

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


