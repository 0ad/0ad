/* Copyright (C) 2017 Wildfire Games.
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
#include "ps/Profile.h"

// Use the macro below to define types that will be passed by value to C++ functions.
// NOTE: References are used just to avoid superfluous copy constructor calls
// in the script wrapper code. They cannot be used as out-parameters.
// They are const T& by default to avoid confusion about this, especially
// because sometimes the function is not just exposed to scripts, but also
// called from C++ code.

template <typename T> struct ScriptInterface::MaybeRef
{
	typedef const T& Type;
};

#define PASS_BY_VALUE_IN_NATIVE_WRAPPER(T) \
template <> struct ScriptInterface::MaybeRef<T> \
{ \
	typedef T Type; \
}; \

PASS_BY_VALUE_IN_NATIVE_WRAPPER(JS::HandleValue)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(bool)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(int)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(uint8_t)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(uint16_t)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(uint32_t)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(fixed)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(float)
PASS_BY_VALUE_IN_NATIVE_WRAPPER(double)

#undef PASS_BY_VALUE_IN_NATIVE_WRAPPER

// This works around a bug in Visual Studio 2013 (error C2244 if ScriptInterface:: is included in the
// type specifier of MaybeRef<T>::Type for parameters inside the member function declaration).
// It's probably the bug described here, but I'm not quite sure (at least the example there still
// cause error C2244):
// https://connect.microsoft.com/VisualStudio/feedback/details/611863/vs2010-c-fails-with-error-c2244-gcc-4-3-4-compiles-ok
//
// TODO: When dropping support for VS 2013, check if this bug is still present in the supported
// Visual Studio versions (replace the macro definitions in NativeWrapperDecls.h with these ones,
// remove them from here and check if this causes error C2244 when compiling.
#undef NUMBERED_LIST_TAIL_MAYBE_REF
#undef NUMBERED_LIST_BALANCED_MAYBE_REF
#define NUMBERED_LIST_TAIL_MAYBE_REF(z, i, data) , typename ScriptInterface::MaybeRef<data##i>::Type
#define NUMBERED_LIST_BALANCED_MAYBE_REF(z, i, data) BOOST_PP_COMMA_IF(i) typename ScriptInterface::MaybeRef<data##i>::Type

// (NativeWrapperDecls.h set up a lot of the macros we use here)

// ScriptInterface_NativeWrapper<T>::call(cx, rval, fptr, args...) will call fptr(cbdata, args...),
// and if T != void then it will store the result in rval:

// Templated on the return type so void can be handled separately
template <typename R>
struct ScriptInterface_NativeWrapper
{
	template<typename F, typename... Ts>
	static void call(JSContext* cx, JS::MutableHandleValue rval, F fptr, Ts... params)
	{
		ScriptInterface::AssignOrToJSValUnrooted<R>(cx, rval, fptr(ScriptInterface::GetScriptInterfaceAndCBData(cx), params...));
	}
};

// Overloaded to ignore the return value from void functions
template <>
struct ScriptInterface_NativeWrapper<void>
{
	template<typename F, typename... Ts>
	static void call(JSContext* cx, JS::MutableHandleValue UNUSED(rval), F fptr, Ts... params)
	{
		fptr(ScriptInterface::GetScriptInterfaceAndCBData(cx), params...);
	}
};

// Same idea but for method calls:

template <typename R, typename TC>
struct ScriptInterface_NativeMethodWrapper
{
	template<typename F, typename... Ts>
	static void call(JSContext* cx, JS::MutableHandleValue rval, TC* c, F fptr, Ts... params)
	{
		ScriptInterface::AssignOrToJSValUnrooted<R>(cx, rval, (c->*fptr)(params...));
	}
};

template <typename TC>
struct ScriptInterface_NativeMethodWrapper<void, TC>
{
	template<typename F, typename... Ts>
	static void call(JSContext* UNUSED(cx), JS::MutableHandleValue UNUSED(rval), TC* c, F fptr, Ts... params)
	{
		(c->*fptr)(params...);
	}
};

// JSFastNative-compatible function that wraps the function identified in the template argument list
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  R (*fptr) ( ScriptInterface::CxPrivate* T0_TAIL_MAYBE_REF(z,i) )> \
	bool ScriptInterface::call(JSContext* cx, uint argc, JS::Value* vp) \
	{ \
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
		JSAutoRequest rq(cx); \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		JS::RootedValue rval(cx); \
		ScriptInterface_NativeWrapper<R>::template call<R( ScriptInterface::CxPrivate* T0_TAIL_MAYBE_REF(z,i))  T0_TAIL(z,i)>(cx, &rval, fptr  A0_TAIL(z,i)); \
		args.rval().set(rval); \
		return !ScriptInterface::IsExceptionPending(cx); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Same idea but for methods
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  JSClass* CLS, typename TC, R (TC::*fptr) ( T0_MAYBE_REF(z,i) )> \
	bool ScriptInterface::callMethod(JSContext* cx, uint argc, JS::Value* vp) \
	{ \
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
		JSAutoRequest rq(cx); \
		JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp)); \
		if (ScriptInterface::GetClass(thisObj) != CLS) return false; \
		TC* c = static_cast<TC*>(ScriptInterface::GetPrivate(thisObj)); \
		if (! c) return false; \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		JS::RootedValue rval(cx); \
		ScriptInterface_NativeMethodWrapper<R, TC>::template call<R (TC::*)(T0_MAYBE_REF(z,i))  T0_TAIL(z,i)>(cx, &rval, c, fptr A0_TAIL(z,i)); \
		args.rval().set(rval); \
		return !ScriptInterface::IsExceptionPending(cx); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// const methods
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  JSClass* CLS, typename TC, R (TC::*fptr) ( T0_MAYBE_REF(z,i) ) const> \
	bool ScriptInterface::callMethodConst(JSContext* cx, uint argc, JS::Value* vp) \
	{ \
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
		JSAutoRequest rq(cx); \
		JS::RootedObject thisObj(cx, JS_THIS_OBJECT(cx, vp)); \
		if (ScriptInterface::GetClass(thisObj) != CLS) return false; \
		TC* c = static_cast<TC*>(ScriptInterface::GetPrivate(thisObj)); \
		if (! c) return false; \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		JS::RootedValue rval(cx); \
		ScriptInterface_NativeMethodWrapper<R, TC>::template call<R (TC::*)(T0_MAYBE_REF(z,i)) const  T0_TAIL(z,i)>(cx, &rval, c, fptr A0_TAIL(z,i)); \
		args.rval().set(rval); \
		return !ScriptInterface::IsExceptionPending(cx); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

template<int i, typename T, typename... Ts>
static void AssignOrToJSValHelper(JSContext* cx, JS::AutoValueVector& argv, const T& a, const Ts&... params)
{
	ScriptInterface::AssignOrToJSVal(cx, argv[i], a);
	AssignOrToJSValHelper<i+1>(cx, argv, params...);
}

template<int i, typename... Ts>
static void AssignOrToJSValHelper(JSContext* UNUSED(cx), JS::AutoValueVector& UNUSED(argv))
{
	cassert(sizeof...(Ts) == 0);
	// Nop, for terminating the template recursion.
}

template<typename R, typename... Ts>
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, R& ret, const Ts&... params) const
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(sizeof...(Ts));
	AssignOrToJSValHelper<0>(cx, argv, params...);
	bool ok = CallFunction_(val, name, argv, &jsRet);
	if (!ok)
		return false;
	return FromJSVal(cx, jsRet, ret);
}

template<typename R, typename... Ts>
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, JS::Rooted<R>* ret, const Ts&... params) const
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::MutableHandle<R> jsRet(ret);
	JS::AutoValueVector argv(cx);
	argv.resize(sizeof...(Ts));
	AssignOrToJSValHelper<0>(cx, argv, params...);
	return CallFunction_(val, name, argv, jsRet);
}

template<typename R, typename... Ts>
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, JS::MutableHandle<R> ret, const Ts&... params) const
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(sizeof...(Ts));
	AssignOrToJSValHelper<0>(cx, argv, params...);
	return CallFunction_(val, name, argv, ret);
}

// Call the named property on the given object, with void return type
template<typename... Ts>
bool ScriptInterface::CallFunctionVoid(JS::HandleValue val, const char* name, const Ts&... params) const
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(sizeof...(Ts));
	AssignOrToJSValHelper<0>(cx, argv, params...);
	return CallFunction_(val, name, argv, &jsRet);
}

// Clean up our mess
#undef NUMBERED_LIST_HEAD
#undef NUMBERED_LIST_TAIL
#undef NUMBERED_LIST_TAIL_MAYBE_REF
#undef NUMBERED_LIST_BALANCED
#undef NUMBERED_LIST_BALANCED_MAYBE_REF
#undef CONVERT_ARG
#undef TYPENAME_T0_HEAD
#undef T0
#undef T0_MAYBE_REF
#undef T0_TAIL
#undef T0_TAIL_MAYBE_REF
#undef A0_TAIL
