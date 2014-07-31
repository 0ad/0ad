/* Copyright (C) 2014 Wildfire Games.
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
 #include "ps/GameSetup/Config.h"

// (NativeWrapperDecls.h set up a lot of the macros we use here)

// ScriptInterface_NativeWrapper<T>::call(cx, rval, fptr, args...) will call fptr(cbdata, args...),
// and if T != void then it will store the result in rval:

// Templated on the return type so void can be handled separately
template <typename R>
struct ScriptInterface_NativeWrapper {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, JS::MutableHandleValue rval, F fptr  T0_A0(z,i)) { \
			ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface; \
			pScriptInterface->AssignOrToJSVal<R>(rval, fptr(ScriptInterface::GetScriptInterfaceAndCBData(cx) A0_TAIL(z,i))); \
		}

	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Overloaded to ignore the return value from void functions
template <>
struct ScriptInterface_NativeWrapper<void> {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, JS::MutableHandleValue /*rval*/, F fptr  T0_A0(z,i)) { \
			fptr(ScriptInterface::GetScriptInterfaceAndCBData(cx) A0_TAIL(z,i)); \
		}
	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Same idea but for method calls:

template <typename R, typename TC>
struct ScriptInterface_NativeMethodWrapper {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, JS::MutableHandleValue rval, TC* c, F fptr  T0_A0(z,i)) { \
			ScriptInterface* pScriptInterface = ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface; \
			pScriptInterface->AssignOrToJSVal<R>(rval, (c->*fptr)( A0(z,i) )); \
		}

	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

template <typename TC>
struct ScriptInterface_NativeMethodWrapper<void, TC> {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* /*cx*/, JS::MutableHandleValue /*rval*/, TC* c, F fptr  T0_A0(z,i)) { \
			(c->*fptr)( A0(z,i) ); \
		}
	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Fast natives don't trigger the hook we use for profiling, so explicitly
// notify the profiler when these functions are being called.
// ScriptInterface_impl::Register stores the name in a reserved slot.
// (TODO: this doesn't work for functions registered via InterfaceScripted.h.
// Maybe we should do some interned JS_GetFunctionId thing.)
#define SCRIPT_PROFILE \
	if (g_ScriptProfilingEnabled) \
	{ \
		ENSURE(JS_CALLEE(cx, vp).isObject() && JS_ObjectIsFunction(cx, &JS_CALLEE(cx, vp).toObject())); \
		const char* name = "(unknown)"; \
		jsval nameval; \
		nameval = JS_GetReservedSlot( &JS_CALLEE(cx, vp).toObject(), 0); \
		if (!nameval.isUndefined()) \
			name = static_cast<const char*>(JSVAL_TO_PRIVATE(nameval)); \
		CProfileSampleScript profile(name); \
	}

// JSFastNative-compatible function that wraps the function identified in the template argument list
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  R (*fptr) ( ScriptInterface::CxPrivate* T0_TAIL(z,i) )> \
	JSBool ScriptInterface::call(JSContext* cx, uint argc, jsval* vp) { \
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
		JSAutoRequest rq(cx); \
		SCRIPT_PROFILE \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		JS::RootedValue rval(cx); \
		ScriptInterface_NativeWrapper<R>::call(cx, &rval, fptr  A0_TAIL(z,i)); \
		args.rval().set(rval); \
		return !ScriptInterface::IsExceptionPending(cx); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Same idea but for methods
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  JSClass* CLS, typename TC, R (TC::*fptr) ( T0(z,i) )> \
	JSBool ScriptInterface::callMethod(JSContext* cx, uint argc, jsval* vp) { \
		JS::CallArgs args = JS::CallArgsFromVp(argc, vp); \
		JSAutoRequest rq(cx); \
		SCRIPT_PROFILE \
		if (ScriptInterface::GetClass(JS_THIS_OBJECT(cx, vp)) != CLS) return false; \
		TC* c = static_cast<TC*>(ScriptInterface::GetPrivate(JS_THIS_OBJECT(cx, vp))); \
		if (! c) return false; \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		JS::RootedValue rval(cx); \
		ScriptInterface_NativeMethodWrapper<R, TC>::call(cx, &rval, c, fptr  A0_TAIL(z,i)); \
		args.rval().set(rval); \
		return !ScriptInterface::IsExceptionPending(cx); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

#define ASSIGN_OR_TO_JS_VAL(z, i, data) AssignOrToJSVal(argv.handleAt(i), a##i);

#define OVERLOADS(z, i, data) \
template<typename R TYPENAME_T0_TAIL(z, i)> \
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) R& ret) \
{ \
	JSContext* cx = GetContext(); \
	JSAutoRequest rq(cx); \
	JS::RootedValue jsRet(cx); \
	JS::AutoValueVector argv(cx); \
	argv.resize(i); \
	BOOST_PP_REPEAT_##z (i, ASSIGN_OR_TO_JS_VAL, ~) \
	bool ok = CallFunction_(val, name, argv.length(), argv.begin(), &jsRet); \
	if (!ok) \
		return false; \
	return FromJSVal(cx, jsRet, ret); \
}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

#define OVERLOADS(z, i, data) \
template<typename R TYPENAME_T0_TAIL(z, i)> \
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) JS::Rooted<R>* ret) \
{ \
	JSContext* cx = GetContext(); \
	JSAutoRequest rq(cx); \
	JS::MutableHandle<R> jsRet(ret); \
	JS::AutoValueVector argv(cx); \
	argv.resize(i); \
	BOOST_PP_REPEAT_##z (i, ASSIGN_OR_TO_JS_VAL, ~) \
	bool ok = CallFunction_(val, name, argv.length(), argv.begin(), jsRet); \
	if (!ok) \
		return false; \
	return true; \
}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

#define OVERLOADS(z, i, data) \
template<typename R TYPENAME_T0_TAIL(z, i)> \
bool ScriptInterface::CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) JS::MutableHandle<R> ret) \
{ \
	JSContext* cx = GetContext(); \
	JSAutoRequest rq(cx); \
	JS::AutoValueVector argv(cx); \
	argv.resize(i); \
	BOOST_PP_REPEAT_##z (i, ASSIGN_OR_TO_JS_VAL, ~) \
	bool ok = CallFunction_(val, name, argv.length(), argv.begin(), ret); \
	if (!ok) \
		return false; \
	return true; \
}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS
#undef ASSIGN_OR_TO_JS_VAL

// Clean up our mess
#undef SCRIPT_PROFILE
#undef NUMBERED_LIST_HEAD
#undef NUMBERED_LIST_TAIL
#undef NUMBERED_LIST_BALANCED
#undef TYPED_ARGS_CONST_REF
#undef TYPED_ARGS
#undef CONVERT_ARG
#undef TYPENAME_T0_HEAD
#undef TYPENAME_T0_TAIL
#undef T0
#undef T0_HEAD
#undef T0_TAIL
#undef T0_A0_CONST_REF
#undef T0_A0
#undef A0
#undef A0_TAIL
