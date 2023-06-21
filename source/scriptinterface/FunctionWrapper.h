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

#ifndef INCLUDED_FUNCTIONWRAPPER
#define INCLUDED_FUNCTIONWRAPPER

#include "Object.h"
#include "ScriptConversions.h"
#include "ScriptExceptions.h"
#include "ScriptRequest.h"

#include <tuple>
#include <type_traits>

class ScriptInterface;

/**
 * This class introduces templates to conveniently wrap C++ functions in JSNative functions.
 * This _is_ rather template heavy, so compilation times beware.
 * The C++ code can have arbitrary arguments and arbitrary return types, so long
 * as they can be converted to/from JS using Script::ToJSVal (FromJSVal respectively),
 * and they are default-constructible (TODO: that can probably changed).
 * (This could be a namespace, but I like being able to specify public/private).
 */
class ScriptFunction {
private:
	ScriptFunction() = delete;
	ScriptFunction(const ScriptFunction&) = delete;
	ScriptFunction(ScriptFunction&&) = delete;

	/**
	 * In JS->C++ calls, types are converted using FromJSVal,
	 * and this requires them to be default-constructible (as that function takes an out parameter)
	 * thus constref needs to be removed when defining the tuple.
	 * Exceptions are:
	 *  - const ScriptRequest& (as the first argument only, for implementation simplicity).
	 *  - const ScriptInterface& (as the first argument only, for implementation simplicity).
	 *  - JS::HandleValue
	 */
	template<typename T>
	using type_transform = std::conditional_t<
		std::is_same_v<const ScriptRequest&, T> || std::is_same_v<const ScriptInterface&, T>,
		T,
		std::remove_const_t<typename std::remove_reference_t<T>>
	>;

	/**
	 * Convenient struct to get info on a [class] [const] function pointer.
	 * TODO VS19: I ran into a really weird bug with an auto specialisation on this taking function pointers.
	 * It'd be good to add it back once we upgrade.
	 */
	template <class T> struct args_info;

	template<typename R, typename ...Types>
	struct args_info<R(*)(Types ...)>
	{
		static constexpr const size_t nb_args = sizeof...(Types);
		using return_type = R;
		using object_type = void;
		using arg_types = std::tuple<type_transform<Types>...>;
	};

	template<typename C, typename R, typename ...Types>
	struct args_info<R(C::*)(Types ...)> : public args_info<R(*)(Types ...)> { using object_type = C; };
	template<typename C, typename R, typename ...Types>
	struct args_info<R(C::*)(Types ...) const> : public args_info<R(C::*)(Types ...)> {};

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	/**
	 * DoConvertFromJS takes a type, a JS argument, and converts.
	 * The type T must be default constructible (except for HandleValue, which is handled specially).
	 * (possible) TODO: this could probably be changed if FromJSVal had a different signature.
	 * @param went_ok - true if the conversion succeeded and went_ok was true before, false otherwise.
	 */
	template<size_t idx, typename T>
	static std::tuple<T> DoConvertFromJS(const ScriptRequest& rq, JS::CallArgs& args, bool& went_ok)
	{
		// No need to convert JS values.
		if constexpr (std::is_same_v<T, JS::HandleValue>)
		{
			// Default-construct values that aren't passed by JS.
			// TODO: this should perhaps be removed, as it's distinct from C++ default values and kind of tricky.
			if (idx >= args.length())
				return std::forward_as_tuple(JS::UndefinedHandleValue);
			else
			{
				// GCC (at least < 9) & VS17 prints warnings if arguments are not used in some constexpr branch.
				UNUSED2(rq); UNUSED2(args); UNUSED2(went_ok);
				return std::forward_as_tuple(args[idx]); // This passes the null handle value if idx is beyond the length of args.
			}
		}
		else
		{
			// Default-construct values that aren't passed by JS.
			// TODO: this should perhaps be removed, as it's distinct from C++ default values and kind of tricky.
			if (idx >= args.length())
				return std::forward_as_tuple(T{});
			else
			{
				T ret;
				went_ok &= Script::FromJSVal<T>(rq, args[idx], ret);
				return std::forward_as_tuple(ret);
			}
		}
	}

	/**
	 * Recursive wrapper: calls DoConvertFromJS for type T and recurses.
	 */
	template<size_t idx, typename T, typename V, typename ...Types>
	static std::tuple<T, V, Types...> DoConvertFromJS(const ScriptRequest& rq, JS::CallArgs& args, bool& went_ok)
	{
		return std::tuple_cat(DoConvertFromJS<idx, T>(rq, args, went_ok), DoConvertFromJS<idx + 1, V, Types...>(rq, args, went_ok));
	}

	/**
	 * ConvertFromJS is a wrapper around DoConvertFromJS, and serves to:
	 *  - unwrap the tuple types as a parameter pack
	 *  - handle specific cases for the first argument (ScriptRequest, ...).
	 *
	 * Trick: to unpack the types of the tuple as a parameter pack, we deduce them from the function signature.
	 * To do that, we want the tuple in the arguments, but we don't want to actually have to default-instantiate,
	 * so we'll pass a nullptr that's static_cast to what we want.
	 */
	template<typename ...Types>
	static std::tuple<Types...> ConvertFromJS(const ScriptRequest& rq, JS::CallArgs& args, bool& went_ok, std::tuple<Types...>*)
	{
		if constexpr (sizeof...(Types) == 0)
		{
			// GCC (at least < 9) & VS17 prints warnings if arguments are not used in some constexpr branch.
			UNUSED2(rq); UNUSED2(args); UNUSED2(went_ok);
			return {};
		}
		else
			return DoConvertFromJS<0, Types...>(rq, args, went_ok);
	}

	// Overloads for ScriptRequest& first argument.
	template<typename ...Types>
	static std::tuple<const ScriptRequest&, Types...> ConvertFromJS(const ScriptRequest& rq, JS::CallArgs& args, bool& went_ok, std::tuple<const ScriptRequest&, Types...>*)
	{
		if constexpr (sizeof...(Types) == 0)
		{
			// GCC (at least < 9) & VS17 prints warnings if arguments are not used in some constexpr branch.
			UNUSED2(args); UNUSED2(went_ok);
			return std::forward_as_tuple(rq);
		}
		else
			return std::tuple_cat(std::forward_as_tuple(rq), DoConvertFromJS<0, Types...>(rq, args, went_ok));
	}

	// Overloads for ScriptInterface& first argument.
	template<typename ...Types>
	static std::tuple<const ScriptInterface&, Types...> ConvertFromJS(const ScriptRequest& rq, JS::CallArgs& args, bool& went_ok, std::tuple<const ScriptInterface&, Types...>*)
	{
		if constexpr (sizeof...(Types) == 0)
		{
			// GCC (at least < 9) & VS17 prints warnings if arguments are not used in some constexpr branch.
			UNUSED2(rq); UNUSED2(args); UNUSED2(went_ok);
			return std::forward_as_tuple(rq.GetScriptInterface());
		}
		else
			return std::tuple_cat(std::forward_as_tuple(rq.GetScriptInterface()), DoConvertFromJS<0, Types...>(rq, args, went_ok));
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	/**
	 * Wrap std::apply for the case where we have an object method or a regular function.
	 */
	template <auto callable, typename T, typename tuple>
	static typename args_info<decltype(callable)>::return_type call(T* object, tuple& args)
	{
		if constexpr(std::is_same_v<T, void>)
		{
			// GCC (at least < 9) & VS17 prints warnings if arguments are not used in some constexpr branch.
			UNUSED2(object);
			return std::apply(callable, args);
		}
		else
			return std::apply(callable, std::tuple_cat(std::forward_as_tuple(*object), args));
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	struct IgnoreResult_t {};
	static inline IgnoreResult_t IgnoreResult;

	/**
	 * Recursive helper to call AssignOrToJSVal
	 */
	template<int i, typename T, typename... Ts>
	static void AssignOrToJSValHelper(const ScriptRequest& rq, JS::MutableHandleValueVector argv, const T& a, const Ts&... params)
	{
		Script::ToJSVal(rq, argv[i], a);
		AssignOrToJSValHelper<i+1>(rq, argv, params...);
	}

	template<int i, typename... Ts>
	static void AssignOrToJSValHelper(const ScriptRequest& UNUSED(rq), JS::MutableHandleValueVector UNUSED(argv))
	{
		static_assert(sizeof...(Ts) == 0);
		// Nop, for terminating the template recursion.
	}

	/**
	 * Wrapper around calling a JS function from C++.
	 * Arguments are const& to avoid lvalue/rvalue issues, and so can't be used as out-parameters.
	 * In particular, the problem is that Rooted are deduced as Rooted, not Handle, and so can't be copied.
	 * This could be worked around with more templates, but it doesn't seem particularly worth doing.
	 */
	template<typename R, typename ...Args>
	static bool Call_(const ScriptRequest& rq, JS::HandleValue val, const char* name, R& ret, const Args&... args)
	{
		JS::RootedObject obj(rq.cx);
		if (!JS_ValueToObject(rq.cx, val, &obj) || !obj)
			return false;

		// Fetch the property explicitly - this avoids converting the arguments if it doesn't exist.
		JS::RootedValue func(rq.cx);
		if (!JS_GetProperty(rq.cx, obj, name, &func) || func.isUndefined())
			return false;

		JS::RootedValueVector argv(rq.cx);
		ignore_result(argv.resize(sizeof...(Args)));
		AssignOrToJSValHelper<0>(rq, &argv, args...);

		bool success;
		if constexpr (std::is_same_v<R, JS::MutableHandleValue>)
			success = JS_CallFunctionValue(rq.cx, obj, func, argv, ret);
		else
		{
			JS::RootedValue jsRet(rq.cx);
			success = JS_CallFunctionValue(rq.cx, obj, func, argv, &jsRet);
			if constexpr (!std::is_same_v<R, IgnoreResult_t>)
			{
				if (success)
					Script::FromJSVal(rq, jsRet, ret);
			}
			else
				UNUSED2(ret); // VS2017 complains.
		}
		// Even if everything succeeded, there could be pending exceptions
		return !ScriptException::CatchPending(rq) && success;
	}

	///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////
public:
	template <typename T>
	using ObjectGetter = T*(*)(const ScriptRequest&, JS::CallArgs&);

	// TODO: the fact that this takes class and not auto is to work around an odd VS17 bug.
	// It can be removed with VS19.
	template <class callableType>
	using GetterFor = ObjectGetter<typename args_info<callableType>::object_type>;

	/**
	 * The meat of this file. This wraps a C++ function into a JSNative,
	 * so that it can be called from JS and manipulated in Spidermonkey.
	 * Most C++ functions can be directly wrapped, so long as their arguments are
	 * convertible from JS::Value and their return value is convertible to JS::Value (or void)
	 * The C++ function may optionally take const ScriptRequest& or ScriptInterface& as its first argument.
	 * The function may be an object method, in which case you need to pass an appropriate getter
	 *
	 * Optimisation note: the ScriptRequest object is created even without arguments,
	 * as it's necessary for IsExceptionPending.
	 *
	 * @param thisGetter to get the object, if necessary.
	 */
	template <auto callable, GetterFor<decltype(callable)> thisGetter = nullptr>
	static bool ToJSNative(JSContext* cx, unsigned argc, JS::Value* vp)
	{
		using ObjType = typename args_info<decltype(callable)>::object_type;

		JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
		ScriptRequest rq(cx);

		// If the callable is an object method, we must specify how to fetch the object.
		static_assert(std::is_same_v<typename args_info<decltype(callable)>::object_type, void> || thisGetter != nullptr,
					  "ScriptFunction::Register - No getter specified for object method");

// GCC 7 triggers spurious warnings
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
#endif
		ObjType* obj = nullptr;
		if constexpr (thisGetter != nullptr)
		{
			obj = thisGetter(rq, args);
			if (!obj)
				return false;
		}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

		bool went_ok = true;
		typename args_info<decltype(callable)>::arg_types outs = ConvertFromJS(rq, args, went_ok, static_cast<typename args_info<decltype(callable)>::arg_types*>(nullptr));
		if (!went_ok)
			return false;

		/**
		 * TODO: error handling isn't standard, and since this can call any C++ function,
		 * there's no simple obvious way to deal with it.
		 * For now we check for pending JS exceptions, but it would probably be nicer
		 * to standardise on something, or perhaps provide an "errorHandler" here.
		 */
		if constexpr (std::is_same_v<void, typename args_info<decltype(callable)>::return_type>)
			call<callable>(obj, outs);
		else if constexpr (std::is_same_v<JS::Value, typename args_info<decltype(callable)>::return_type>)
			args.rval().set(call<callable>(obj, outs));
		else
			Script::ToJSVal(rq, args.rval(), call<callable>(obj, outs));

		return !ScriptException::IsPending(rq);
	}

	/**
	 * Call a JS function @a name, property of object @a val, with the arguments @a args.
	 * @a ret will be updated with the return value, if any.
	 * @return the success (or failure) thereof.
	 */
	template<typename R, typename ...Args>
	static bool Call(const ScriptRequest& rq, JS::HandleValue val, const char* name, R& ret, const Args&... args)
	{
		return Call_(rq, val, name, ret, std::forward<const Args>(args)...);
	}

	// Specialisation for MutableHandleValue return.
	template<typename ...Args>
	static bool Call(const ScriptRequest& rq, JS::HandleValue val, const char* name, JS::MutableHandleValue ret, const Args&... args)
	{
		return Call_(rq, val, name, ret, std::forward<const Args>(args)...);
	}

	/**
	 * Call a JS function @a name, property of object @a val, with the arguments @a args.
	 * @return the success (or failure) thereof.
	 */
	template<typename ...Args>
	static bool CallVoid(const ScriptRequest& rq, JS::HandleValue val, const char* name, const Args&... args)
	{
		return Call(rq, val, name, IgnoreResult, std::forward<const Args>(args)...);
	}

	/**
	 * Return a function spec from a C++ function.
	 */
	template <auto callable, GetterFor<decltype(callable)> thisGetter = nullptr, u16 flags = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT>
	static JSFunctionSpec Wrap(const char* name)
	{
		return JS_FN(name, (&ToJSNative<callable, thisGetter>), args_info<decltype(callable)>::nb_args, flags);
	}

	/**
	 * Return a JSFunction from a C++ function.
	 */
	template <auto callable, GetterFor<decltype(callable)> thisGetter = nullptr, u16 flags = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT>
	static JSFunction* Create(const ScriptRequest& rq, const char* name)
	{
		return JS_NewFunction(rq.cx, &ToJSNative<callable, thisGetter>, args_info<decltype(callable)>::nb_args, flags, name);
	}

	/**
	 * Register a function on the native scope (usually 'Engine').
	 */
	template <auto callable, GetterFor<decltype(callable)> thisGetter = nullptr, u16 flags = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT>
	static void Register(const ScriptRequest& rq, const char* name)
	{
		JS_DefineFunction(rq.cx, rq.nativeScope, name, &ToJSNative<callable, thisGetter>, args_info<decltype(callable)>::nb_args, flags);
	}

	/**
	 * Register a function on @param scope.
	 * Prefer the version taking ScriptRequest unless you have a good reason not to.
	 * @see Register
	 */
	template <auto callable, GetterFor<decltype(callable)> thisGetter = nullptr, u16 flags = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT>
	static void Register(JSContext* cx, JS::HandleObject scope, const char* name)
	{
		JS_DefineFunction(cx, scope, name, &ToJSNative<callable, thisGetter>, args_info<decltype(callable)>::nb_args, flags);
	}
};

#endif // INCLUDED_FUNCTIONWRAPPER
