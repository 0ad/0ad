/* Copyright (C) 2009 Wildfire Games.
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

// (NativeWrapperDecls.h set up a lot of the macros we use here)


// ScriptInterface_NativeWrapper<T>::call(cx, rval, fptr, args...) will call fptr(cbdata, args...),
// and if T != void then it will store the result in rval:

// Templated on the return type so void can be handled separately
template <typename R>
struct ScriptInterface_NativeWrapper {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, jsval& rval, F fptr  T0_A0(z,i)) { \
			rval = ScriptInterface::ToJSVal<R>(cx, fptr(ScriptInterface::GetCallbackData(cx)  A0_TAIL(z,i))); \
		}

	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Overloaded to ignore the return value from void functions
template <>
struct ScriptInterface_NativeWrapper<void> {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, jsval& /*rval*/, F fptr  T0_A0(z,i)) { \
			fptr(ScriptInterface::GetCallbackData(cx)  A0_TAIL(z,i)); \
		}
	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

// Same idea but for method calls:

template <typename R, typename TC>
struct ScriptInterface_NativeMethodWrapper {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* cx, jsval& rval, TC* c, F fptr  T0_A0(z,i)) { \
			rval = ScriptInterface::ToJSVal<R>(cx, (c->*fptr)( A0(z,i) )); \
		}

	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};

template <typename TC>
struct ScriptInterface_NativeMethodWrapper<void, TC> {
	#define OVERLOADS(z, i, data) \
		template<TYPENAME_T0_HEAD(z,i)  typename F> \
		static void call(JSContext* /*cx*/, jsval& /*rval*/, TC* c, F fptr  T0_A0(z,i)) { \
			(c->*fptr)( A0(z,i) ); \
		}
	BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
	#undef OVERLOADS
};



// JSNative-compatible function that wraps the function identified in the template argument list
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  R (*fptr) ( void* T0_TAIL(z,i) )> \
	JSBool ScriptInterface::call(JSContext* cx, JSObject* /*obj*/, uintN /*argc*/, jsval* argv, jsval* rval) { \
		(void)argv; /* avoid 'unused parameter' warnings */ \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		ScriptInterface_NativeWrapper<R>::call(cx, *rval, fptr  A0_TAIL(z,i)); \
		return (ScriptInterface::IsExceptionPending(cx) ? JS_FALSE : JS_TRUE); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Same idea but for methods
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  JSClass* CLS, typename TC, R (TC::*fptr) ( T0(z,i) )> \
	JSBool ScriptInterface::callMethod(JSContext* cx, JSObject* obj, uintN /*argc*/, jsval* argv, jsval* rval) { \
		(void)argv; /* avoid 'unused parameter' warnings */ \
		if (ScriptInterface::GetClass(cx, obj) != CLS) return JS_FALSE; \
		TC* c = static_cast<TC*>(ScriptInterface::GetPrivate(cx, obj)); \
		if (! c) return JS_FALSE; \
		BOOST_PP_REPEAT_##z (i, CONVERT_ARG, ~) \
		ScriptInterface_NativeMethodWrapper<R, TC>::call(cx, *rval, c, fptr  A0_TAIL(z,i)); \
		return (ScriptInterface::IsExceptionPending(cx) ? JS_FALSE : JS_TRUE); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Clean up our mess
#undef NUMBERED_LIST_HEAD
#undef NUMBERED_LIST_TAIL
#undef NUMBERED_LIST_BALANCED
#undef TYPED_ARGS
#undef CONVERT_ARG
#undef TYPENAME_T0_HEAD
#undef TYPENAME_T0_TAIL
#undef T0
#undef T0_HEAD
#undef T0_TAIL
#undef T0_A0
#undef A0
#undef A0_TAIL
