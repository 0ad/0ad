/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTINTERFACE
#define INCLUDED_SCRIPTINTERFACE

#include <memory>
#include <vector>
#include <string>

#include "ScriptTypes.h"
#include "ScriptVal.h"

#include "js/jsapi.h"

#include "lib/file/vfs/vfs_path.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

class AutoGCRooter;

namespace boost { class rand48; }

// Set the maximum number of function arguments that can be handled
// (This should be as small as possible (for compiler efficiency),
// but as large as necessary for all wrapped functions)
#define SCRIPT_INTERFACE_MAX_ARGS 6

#ifdef NDEBUG
#define ENABLE_SCRIPT_PROFILING 0
#else
#define ENABLE_SCRIPT_PROFILING 1
#endif

struct ScriptInterface_impl;

class ScriptRuntime;

/**
 * Abstraction around a SpiderMonkey JSContext.
 *
 * Thread-safety:
 * - May be used in non-main threads.
 * - Each ScriptInterface must be created, used, and destroyed, all in a single thread
 *   (it must never be shared between threads).
 */
class ScriptInterface
{
public:

	/**
	 * Returns a runtime, which can used to initialise any number of
	 * ScriptInterfaces contexts. Values created in one context may be used
	 * in any other context from the same runtime (but not any other runtime).
	 * Each runtime should only ever be used on a single thread.
	 */
	static shared_ptr<ScriptRuntime> CreateRuntime();

	/**
	 * Constructor.
	 * @param nativeScopeName Name of global object that functions (via RegisterFunction) will
	 *   be placed into, as a scoping mechanism; typically "Engine"
	 */
	ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptRuntime>& runtime);

	~ScriptInterface();

	/**
	 * Shut down the JS system to clean up memory. Must only be called when there
	 * are no ScriptInterfaces alive.
	 */
	static void ShutDown();

	void SetCallbackData(void* cbdata);
	static void* GetCallbackData(JSContext* cx);

	JSContext* GetContext() const;
	JSRuntime* GetRuntime() const;

	void ReplaceNondeterministicFunctions(boost::rand48& rng);

	/**
	 * Call a constructor function, equivalent to JS "new ctor(arg)".
	 * @return The new object; or JSVAL_VOID on failure, and logs an error message
	 */
	jsval CallConstructor(jsval ctor, jsval arg);

	/**
	 * Create an object as with CallConstructor except don't actually execute the
	 * constructor function.
	 * @return The new object; or JSVAL_VOID on failure, and logs an error message
	 */
	jsval NewObjectFromConstructor(jsval ctor);

	/**
	 * Call the named property on the given object, with void return type and 0 arguments
	 */
	bool CallFunctionVoid(jsval val, const char* name);

	/**
	 * Call the named property on the given object, with void return type and 1 argument
	 */
	template<typename T0>
	bool CallFunctionVoid(jsval val, const char* name, const T0& a0);

	/**
	 * Call the named property on the given object, with void return type and 2 arguments
	 */
	template<typename T0, typename T1>
	bool CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1);

	/**
	 * Call the named property on the given object, with void return type and 3 arguments
	 */
	template<typename T0, typename T1, typename T2>
	bool CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2);

	/**
	 * Call the named property on the given object, with return type R and 0 arguments
	 */
	template<typename R>
	bool CallFunction(jsval val, const char* name, R& ret);

	/**
	 * Call the named property on the given object, with return type R and 1 argument
	 */
	template<typename T0, typename R>
	bool CallFunction(jsval val, const char* name, const T0& a0, R& ret);

	/**
	 * Call the named property on the given object, with return type R and 2 arguments
	 */
	template<typename T0, typename T1, typename R>
	bool CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, R& ret);

	/**
	 * Call the named property on the given object, with return type R and 3 arguments
	 */
	template<typename T0, typename T1, typename T2, typename R>
	bool CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, R& ret);

	/**
	 * Call the named property on the given object, with return type R and 4 arguments
	 */
	template<typename T0, typename T1, typename T2, typename T3, typename R>
	bool CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, const T3& a3, R& ret);

	jsval GetGlobalObject();

	JSClass* GetGlobalClass();

	/**
	 * Set the named property on the global object.
	 * If @p replace is true, an existing property will be overwritten; otherwise attempts
	 * to set an already-defined value will fail.
	 */
	template<typename T>
	bool SetGlobal(const char* name, const T& value, bool replace = false);

	/**
	 * Set the named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetProperty(jsval obj, const char* name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Set the integer-named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetPropertyInt(jsval obj, int name, const T& value, bool constant = false, bool enumerate = true);

	template<typename T>
	bool GetProperty(jsval obj, const char* name, T& out);

	bool HasProperty(jsval obj, const char* name);

	bool EnumeratePropertyNamesWithPrefix(jsval obj, const char* prefix, std::vector<std::string>& out);

	bool SetPrototype(jsval obj, jsval proto);

	bool FreezeObject(jsval obj, bool deep);

	bool Eval(const char* code);

	template<typename T, typename CHAR> bool Eval(const CHAR* code, T& out);

	std::wstring ToString(jsval obj);

	/**
	 * Parse a JSON string. Returns the undefined value on error.
	 */
	CScriptValRooted ParseJSON(const utf16string& string);

	/**
	 * Parse a UTF-8-encoded JSON string. Returns the undefined value on error.
	 */
	CScriptValRooted ParseJSON(const std::string& string_utf8);

	/**
	 * Read a JSON file. Returns the undefined value on error.
	 */
	CScriptValRooted ReadJSONFile(const VfsPath& path);

	/**
	 * Stringify to a JSON string, UTF-8 encoded. Returns an empty string on error.
	 */
	std::string StringifyJSON(jsval obj, bool indent = true);

	/**
	 * Report the given error message through the JS error reporting mechanism,
	 * and throw a JS exception. (Callers can check IsPendingException, and must
	 * return JS_FALSE in that case to propagate the exception.)
	 */
	void ReportError(const char* msg);

	/**
	 * Load and execute the given script in a new function scope.
	 * @param filename Name for debugging purposes (not used to load the file)
	 * @param code JS code to execute
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadScript(const std::wstring& filename, const std::wstring& code);

	/**
	 * Load and execute the given script in the global scope.
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadGlobalScriptFile(const VfsPath& path);

	/**
	 * Construct a new value (usable in this ScriptInterface's context) by cloning
	 * a value from a different context.
	 * Complex values (functions, XML, etc) won't be cloned correctly, but basic
	 * types and cyclic references should be fine.
	 */
	jsval CloneValueFromOtherContext(ScriptInterface& otherContext, jsval val);

	/**
	 * Convert a jsval to a C++ type. (This might trigger GC.)
	 */
	template<typename T> static bool FromJSVal(JSContext* cx, jsval val, T& ret);

	/**
	 * Convert a C++ type to a jsval. (This might trigger GC. The return
	 * value must be rooted if you don't want it to be collected.)
	 */
	template<typename T> static jsval ToJSVal(JSContext* cx, T const& val);

	AutoGCRooter* ReplaceAutoGCRooter(AutoGCRooter* rooter);

	/**
	 * Dump some memory heap debugging information to stderr.
	 */
	void DumpHeap();

	void MaybeGC();

	/**
	 * Structured clones are a way to serialize 'simple' JS values into a buffer
	 * that can safely be passed between contexts and runtimes and threads.
	 * A StructuredClone can be stored and read multiple times if desired.
	 * We wrap them in shared_ptr so memory management is automatic and
	 * thread-safe.
	 */
	class StructuredClone
	{
		NONCOPYABLE(StructuredClone);
	public:
		StructuredClone();
		~StructuredClone();
		JSContext* m_Context;
		uint64* m_Data;
		size_t m_Size;
	};

	shared_ptr<StructuredClone> WriteStructuredClone(jsval v);
	jsval ReadStructuredClone(const shared_ptr<StructuredClone>& ptr);

private:
	bool CallFunction_(jsval val, const char* name, size_t argc, jsval* argv, jsval& ret);
	bool Eval_(const char* code, jsval& ret);
	bool Eval_(const wchar_t* code, jsval& ret);
	bool SetGlobal_(const char* name, jsval value, bool replace);
	bool SetProperty_(jsval obj, const char* name, jsval value, bool readonly, bool enumerate);
	bool SetPropertyInt_(jsval obj, int name, jsval value, bool readonly, bool enumerate);
	bool GetProperty_(jsval obj, const char* name, jsval& value);
	static bool IsExceptionPending(JSContext* cx);
	static JSClass* GetClass(JSContext* cx, JSObject* obj);
	static void* GetPrivate(JSContext* cx, JSObject* obj);

	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<ScriptInterface_impl> m;

// The nasty macro/template bits are split into a separate file so you don't have to look at them
public:
	#include "NativeWrapperDecls.h"
	// This declares:
	//
	//   template <R, T0..., TR (*fptr) (void* cbdata, T0...)>
	//   void RegisterFunction(const char* functionName);
	//
	//   template <R, T0..., TR (*fptr) (void* cbdata, T0...)>
	//   static JSNative call;
	//
	//   template <R, T0..., JSClass*, TC, TR (TC:*fptr) (T0...)>
	//   static JSNative callMethod;
	//
	//   template <dummy, T0...>
	//   static size_t nargs();
};

// Implement those declared functions
#include "NativeWrapperDefns.h"

template<typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, R& ret)
{
	jsval jsRet;
	bool ok = CallFunction_(val, name, 0, NULL, jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T0>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0)
{
	jsval jsRet;
	jsval argv[1];
	argv[0] = ToJSVal(GetContext(), a0);
	return CallFunction_(val, name, 1, argv, jsRet);
}

template<typename T0, typename T1>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1)
{
	jsval jsRet;
	jsval argv[2];
	argv[0] = ToJSVal(GetContext(), a0);
	argv[1] = ToJSVal(GetContext(), a1);
	return CallFunction_(val, name, 2, argv, jsRet);
}

template<typename T0, typename T1, typename T2>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2)
{
	jsval jsRet;
	jsval argv[3];
	argv[0] = ToJSVal(GetContext(), a0);
	argv[1] = ToJSVal(GetContext(), a1);
	argv[2] = ToJSVal(GetContext(), a2);
	return CallFunction_(val, name, 3, argv, jsRet);
}

template<typename T0, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, R& ret)
{
	jsval jsRet;
	jsval argv[1];
	argv[0] = ToJSVal(GetContext(), a0);
	bool ok = CallFunction_(val, name, 1, argv, jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T0, typename T1, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, R& ret)
{
	jsval jsRet;
	jsval argv[2];
	argv[0] = ToJSVal(GetContext(), a0);
	argv[1] = ToJSVal(GetContext(), a1);
	bool ok = CallFunction_(val, name, 2, argv, jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T0, typename T1, typename T2, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, R& ret)
{
	jsval jsRet;
	jsval argv[3];
	argv[0] = ToJSVal(GetContext(), a0);
	argv[1] = ToJSVal(GetContext(), a1);
	argv[2] = ToJSVal(GetContext(), a2);
	bool ok = CallFunction_(val, name, 3, argv, jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T0, typename T1, typename T2, typename T3, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, const T3& a3, R& ret)
{
	jsval jsRet;
	jsval argv[4];
	argv[0] = ToJSVal(GetContext(), a0);
	argv[1] = ToJSVal(GetContext(), a1);
	argv[2] = ToJSVal(GetContext(), a2);
	argv[3] = ToJSVal(GetContext(), a3);
	bool ok = CallFunction_(val, name, 4, argv, jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T>
bool ScriptInterface::SetGlobal(const char* name, const T& value, bool replace)
{
	return SetGlobal_(name, ToJSVal(GetContext(), value), replace);
}

template<typename T>
bool ScriptInterface::SetProperty(jsval obj, const char* name, const T& value, bool readonly, bool enumerate)
{
	return SetProperty_(obj, name, ToJSVal(GetContext(), value), readonly, enumerate);
}

template<typename T>
bool ScriptInterface::SetPropertyInt(jsval obj, int name, const T& value, bool readonly, bool enumerate)
{
	return SetPropertyInt_(obj, name, ToJSVal(GetContext(), value), readonly, enumerate);
}

template<typename T>
bool ScriptInterface::GetProperty(jsval obj, const char* name, T& out)
{
	jsval val;
	if (! GetProperty_(obj, name, val))
		return false;
	return FromJSVal(GetContext(), val, out);
}

template<typename T, typename CHAR>
bool ScriptInterface::Eval(const CHAR* code, T& ret)
{
	jsval rval;
	if (! Eval_(code, rval))
		return false;
	return FromJSVal(GetContext(), rval, ret);
}

#endif // INCLUDED_SCRIPTINTERFACE
