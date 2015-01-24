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

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

private:

/**
 * In our interface code (the CONVERT_ARG macro specifically) we require types to be default-constructible.
 * This is a workaround to make the current design work with JS::HandleValue types, which have a private constructor.
 * JS::HandleValue objects are meant to be implicitly created only from JS::RootedValue objects.
 * Generally handles should not be used this way, but in this case we can be sure that the handle will not live longer than its root,
 * so it should be OK.
 * This solution involves some overhead, but it should be quite small and shouldn't affect performance in practice.
 * HandleValue types are just structs with one pointer and fit into a single register.
 */
class HandleWrapper
{
public:
	HandleWrapper() : m_Handle(JS::NullHandleValue) {};
	void set(JS::HandleValue handle) { m_Handle.repoint(handle); }
	operator JS::HandleValue()
	{
		return m_Handle;
	}

private:
	JS::HandleValue m_Handle;
};

// WrapperIfHandle<T>::Type has the type HandleWrapper for T == JS::HandleValue and
// T for all other types.
// Allows to use default-constructible HandleWrapper types in templates instead of the
// HandleValue type that isn't default-constructible without code duplication.
template <typename T> struct WrapperIfHandle;

public:

// Define lots of useful macros:

// Varieties of comma-separated list to fit on the head/tail/whole of another comma-separated list
#define NUMBERED_LIST_HEAD(z, i, data) data##i,
#define NUMBERED_LIST_TAIL(z, i, data) ,data##i
#define NUMBERED_LIST_BALANCED(z, i, data) BOOST_PP_COMMA_IF(i) data##i
// Some other things
#define TYPED_ARGS(z, i, data) , T##i a##i
#define TYPED_ARGS_CONST_REF(z, i, data) const T##i& a##i,

// TODO: We allow optional parameters when the C++ type can be converted from JS::UndefinedValue.
// FromJSVal is expected to either set a##i or return false (otherwise we could get undefined
// behaviour because some types have undefined values when not being initialized).
// This is not very clear and also a bit fragile. Another problem is that the error reporting lacks
// a bit. SpiderMonkey will throw a JS exception and abort the execution of the current function when
// we return false here (without printing a callstack or additional detail telling that an argument
// conversion failed). So we have two TODOs here:
// 1. On the conceptual side: How to consistently work with optional parameters (or drop them completely?)
// 2. On the technical side: Improve error handling, find a better way to ensure parameters are initialized
#define CONVERT_ARG(z, i, data) \
	typename WrapperIfHandle<T##i>::Type a##i; \
	if (! ScriptInterface::FromJSVal<typename WrapperIfHandle<T##i>::Type>(cx, i < args.length() ? args[i] : JS::UndefinedHandleValue, a##i)) return false;

// List-generating macros, named roughly after their first list item
#define TYPENAME_T0_HEAD(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_HEAD, typename T) // "typename T0, typename T1, "
#define TYPENAME_T0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, typename T) // ", typename T0, typename T1"
#define T0(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_BALANCED, T) // "T0, T1"
#define T0_HEAD(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_HEAD, T) // "T0, T1, "
#define T0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, T) // ", T0, T1"
#define T0_A0(z, i) BOOST_PP_REPEAT_##z (i, TYPED_ARGS, ~) // ",T0 a0, T1 a1"
#define T0_A0_CONST_REF(z, i) BOOST_PP_REPEAT_##z (i, TYPED_ARGS_CONST_REF, ~) // ", const T0 a0, const T1 a1, "
#define A0(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_BALANCED, a) // "a0, a1"
#define A0_TAIL(z, i) BOOST_PP_REPEAT_##z (i, NUMBERED_LIST_TAIL, a) // ", a0, a1"

// Define RegisterFunction<TR, T0..., f>
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  R (*fptr) ( ScriptInterface::CxPrivate* T0_TAIL(z,i) )> \
	void RegisterFunction(const char* name) { \
		Register(name, call<R, T0_HEAD(z,i)  fptr>, nargs<0 T0_TAIL(z,i)>()); \
	}
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// JSFastNative-compatible function that wraps the function identified in the template argument list
// (Definition comes later, since it depends on some things we haven't defined yet)
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  R (*fptr) ( ScriptInterface::CxPrivate* T0_TAIL(z,i) )> \
	static bool call(JSContext* cx, uint argc, jsval* vp);
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Similar, for class methods
#define OVERLOADS(z, i, data) \
	template <typename R, TYPENAME_T0_HEAD(z,i)  JSClass* CLS, typename TC, R (TC::*fptr) ( T0(z,i) )> \
	static bool callMethod(JSContext* cx, uint argc, jsval* vp);
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Argument-number counter
#define OVERLOADS(z, i, data) \
	template <int dummy TYPENAME_T0_TAIL(z,i)> /* add a dummy parameter so we still compile with 0 template args */ \
	static size_t nargs() { return i; }
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Call the named property on the given object
#define OVERLOADS(z, i, data) \
	template <typename R TYPENAME_T0_TAIL(z, i)> \
	bool CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) R& ret);
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// Implicit conversion from JS::Rooted<R>* to JS::MutableHandle<R> does not work with template argument deduction
// (only exact type matches allowed). We need this overload to allow passing Rooted<R>* using the & operator
// (as people would expect it to work based on the SpiderMonkey rooting guide).
#define OVERLOADS(z, i, data) \
	template <typename R TYPENAME_T0_TAIL(z, i)> \
	bool CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) JS::Rooted<R>* ret);
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

// This overload is for the case when a JS::MutableHandle<R> type gets passed into CallFunction directly and
// without requiring implicit conversion.
#define OVERLOADS(z, i, data) \
	template <typename R TYPENAME_T0_TAIL(z, i)> \
	bool CallFunction(JS::HandleValue val, const char* name, T0_A0_CONST_REF(z,i) JS::MutableHandle<R> ret);
BOOST_PP_REPEAT(SCRIPT_INTERFACE_MAX_ARGS, OVERLOADS, ~)
#undef OVERLOADS

