/* Copyright (C) 2012 Wildfire Games.
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

#include "ps/Errors.h"
ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, SetupFailed);

ERROR_SUBGROUP(Scripting, LoadFile);
ERROR_TYPE(Scripting_LoadFile, OpenFailed);
ERROR_TYPE(Scripting_LoadFile, EvalErrors);

ERROR_TYPE(Scripting, ConversionFailed);
ERROR_TYPE(Scripting, CallFunctionFailed);
ERROR_TYPE(Scripting, RegisterFunctionFailed);
ERROR_TYPE(Scripting, DefineConstantFailed);
ERROR_TYPE(Scripting, CreateObjectFailed);
ERROR_TYPE(Scripting, TypeDoesNotExist);

ERROR_SUBGROUP(Scripting, DefineType);
ERROR_TYPE(Scripting_DefineType, AlreadyExists);
ERROR_TYPE(Scripting_DefineType, CreationFailed);

#include "lib/file/vfs/vfs_path.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

#include <boost/random/linear_congruential.hpp>

class AutoGCRooter;

// Set the maximum number of function arguments that can be handled
// (This should be as small as possible (for compiler efficiency),
// but as large as necessary for all wrapped functions)
#define SCRIPT_INTERFACE_MAX_ARGS 8

// TODO: what's a good default?
#define DEFAULT_RUNTIME_SIZE 16 * 1024 * 1024

struct ScriptInterface_impl;

class ScriptRuntime;

extern shared_ptr<ScriptRuntime> g_ScriptRuntime;

class CDebuggingServer;

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
	 * @param runtimeSize Maximum size in bytes of the new runtime
	 */
	static shared_ptr<ScriptRuntime> CreateRuntime(int runtimeSize = DEFAULT_RUNTIME_SIZE);


	/**
	 * Constructor.
	 * @param nativeScopeName Name of global object that functions (via RegisterFunction) will
	 *   be placed into, as a scoping mechanism; typically "Engine"
	 * @param debugName Name of this interface for CScriptStats purposes.
	 * @param runtime ScriptRuntime to use when initializing this interface.
	 */
	ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptRuntime>& runtime);

	~ScriptInterface();

	/**
	 * Shut down the JS system to clean up memory. Must only be called when there
	 * are no ScriptInterfaces alive.
	 */
	static void ShutDown();

	struct CxPrivate
	{
		ScriptInterface* pScriptInterface; // the ScriptInterface object the current context belongs to
		void* pCBData; // meant to be used as the "this" object for callback functions
	} m_CxPrivate;

	void SetCallbackData(void* pCBData);
	static CxPrivate* GetScriptInterfaceAndCBData(JSContext* cx);

	JSContext* GetContext() const;
	JSRuntime* GetJSRuntime() const;
	shared_ptr<ScriptRuntime> GetRuntime() const;

	/**
	 * Load global scripts that most script contexts need,
	 * located in the /globalscripts directory. VFS must be initialized.
	 */
	bool LoadGlobalScripts();

	enum CACHED_VAL { CACHE_VECTOR2DPROTO, CACHE_VECTOR3DPROTO };
	CScriptValRooted GetCachedValue(CACHED_VAL valueIdentifier);

	/**
	 * Replace the default JS random number geenrator with a seeded, network-sync'd one.
	 */
	bool ReplaceNondeterministicRNG(boost::rand48& rng);

	/**
	 * Call a constructor function, equivalent to JS "new ctor(arg)".
	 * @return The new object; or JSVAL_VOID on failure, and logs an error message
	 */
	jsval CallConstructor(jsval ctor, uint argc, jsval argv);

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

	JSObject* CreateCustomObject(const std::string & typeName);
	void DefineCustomObjectType(JSClass *clasp, JSNative constructor, uint minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

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
	 * Set the named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetProperty(jsval obj, const wchar_t* name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Set the integer-named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetPropertyInt(jsval obj, int name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Get the named property on the given object.
	 */
	template<typename T>
	bool GetProperty(jsval obj, const char* name, T& out);
	
	/**
	 * This function overload is used for JS::MutableHandleValue type.
	 * If we use JS::RootedValue with the GetProperty function template, it will generate an overload for the type
	 * JS::RootedValue*, but JS::MutableHandleValue needs to be used when passing JS::RootedValue& to a function.
	 * Check the SpiderMonkey rooting guide for details.
	 */
	bool GetPropertyJS(jsval obj, const char* name, JS::MutableHandleValue out);

	/**
	 * Get the integer-named property on the given object.
	 */
	template<typename T>
	bool GetPropertyInt(jsval obj, int name, T& out);

	/**
	 * Check the named property has been defined on the given object.
	 */
	bool HasProperty(jsval obj, const char* name);

	bool EnumeratePropertyNamesWithPrefix(JS::HandleValue objVal, const char* prefix, std::vector<std::string>& out);

	bool SetPrototype(JS::HandleValue obj, JS::HandleValue proto);

	bool FreezeObject(jsval obj, bool deep);

	bool Eval(const char* code);

	template<typename T, typename CHAR> bool Eval(const CHAR* code, T& out);

	std::wstring ToString(jsval obj, bool pretty = false);

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
	bool LoadScript(const VfsPath& filename, const std::string& code);

	/**
	 * Load and execute the given script in the global scope.
	 * @param filename Name for debugging purposes (not used to load the file)
	 * @param code JS code to execute
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadGlobalScript(const VfsPath& filename, const std::wstring& code);

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
	template<typename T> static bool FromJSVal(JSContext* cx, const JS::HandleValue val, T& ret);

	/**
	 * Convert a C++ type to a jsval. (This might trigger GC. The return
	 * value must be rooted if you don't want it to be collected.)
	 * NOTE: We are passing the JS::Value by reference instead of returning it by value.
	 * The reason is a memory corruption problem that appears to be caused by a bug in Visual Studio.
	 * Details here: http://www.wildfiregames.com/forum/index.php?showtopic=17289&p=285921
	 */
	template<typename T> static void ToJSVal(JSContext* cx, JS::Value& ret, T const& val);

	AutoGCRooter* ReplaceAutoGCRooter(AutoGCRooter* rooter);

	/**
	 * Dump some memory heap debugging information to stderr.
	 */
	void DumpHeap();

	/**
	 * MaybeGC tries to determine whether garbage collection in cx's runtime would free up enough memory to be worth the amount of time it would take.
	 * This calls JS_MaybeGC directly, which does not do incremental GC. Usually you should prefer MaybeIncrementalRuntimeGC.
	 */
	void MaybeGC();
	
	/**
	 * MaybeIncrementalRuntimeGC tries to determine whether a runtime-wide garbage collection would free up enough memory to 
	 * be worth the amount of time it would take. It does this with our own logic and NOT some predefined JSAPI logic because
	 * such functionality currently isn't available out of the box.
	 * It does incremental GC which means it will collect one slice each time it's called until the garbage collection is done.
	 * This can and should be called quite regularly. It shouldn't cost much performance because it tries to run a GC only if 
	 * necessary.
	 */
	void MaybeIncrementalRuntimeGC();
	
	/**
	 * Triggers a full non-incremental garbage collection immediately. That should only be required in special cases and normally
	 * you should try to use MaybeIncrementalRuntimeGC instead.
	 */
	void ForceGC();

	/**
	 * MathRandom (this function) calls the random number generator assigned to this ScriptInterface instance and
	 * returns the generated number.
	 * Math_random (with underscore, not this function) is a global function, but different random number generators can be 
	 * stored per ScriptInterface. It calls MathRandom of the current ScriptInterface instance.
	 */
	bool MathRandom(double& nbr);

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
		u64* m_Data;
		size_t m_Size;
	};

	shared_ptr<StructuredClone> WriteStructuredClone(jsval v);
	jsval ReadStructuredClone(const shared_ptr<StructuredClone>& ptr);

private:
	bool CallFunction_(JS::HandleValue val, const char* name, uint argc, jsval* argv, JS::MutableHandleValue ret);
	bool Eval_(const char* code, JS::MutableHandleValue ret);
	bool Eval_(const wchar_t* code, JS::MutableHandleValue ret);
	bool SetGlobal_(const char* name, jsval value, bool replace);
	bool SetProperty_(jsval obj, const char* name, jsval value, bool readonly, bool enumerate);
	bool SetProperty_(jsval obj, const wchar_t* name, jsval value, bool readonly, bool enumerate);
	bool SetPropertyInt_(jsval obj, int name, jsval value, bool readonly, bool enumerate);
	bool GetProperty_(jsval obj, const char* name, JS::MutableHandleValue out);
	bool GetPropertyInt_(jsval obj, int name, JS::MutableHandleValue value);
	static bool IsExceptionPending(JSContext* cx);
	static JSClass* GetClass(JSObject* obj);
	static void* GetPrivate(JSObject* obj);

	class CustomType
	{
	public:
		JSObject*	m_Prototype;
		JSClass*	m_Class;
		JSNative 	m_Constructor;
	};
	void Register(const char* name, JSNative fptr, size_t nargs);
	std::auto_ptr<ScriptInterface_impl> m;
	
	boost::rand48* m_rng;
	std::map<std::string, CustomType> m_CustomObjectTypes;

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
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	bool ok = CallFunction_(val1, name, 0, NULL, &jsRet);
	if (!ok)
		return false;
	return FromJSVal(GetContext(), jsRet, ret);
}

template<typename T0>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(1);
	ToJSVal(cx, argv[0], a0);
	return CallFunction_(val1, name, 1, argv.begin(), &jsRet);
}

template<typename T0, typename T1>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(2);
	ToJSVal(cx, argv[0], a0);
	ToJSVal(cx, argv[1], a1);
	return CallFunction_(val1, name, 2, argv.begin(), &jsRet);
}

template<typename T0, typename T1, typename T2>
bool ScriptInterface::CallFunctionVoid(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(3);
	ToJSVal(cx, argv[0], a0);
	ToJSVal(cx, argv[1], a1);
	ToJSVal(cx, argv[2], a2);
	return CallFunction_(val1, name, 3, argv.begin(), &jsRet);
}

template<typename T0, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, R& ret)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(1);
	ToJSVal(cx, argv[0], a0);
	bool ok = CallFunction_(val1, name, 1, argv.begin(), &jsRet);
	if (!ok)
		return false;
	return FromJSVal(cx, jsRet, ret);
}

template<typename T0, typename T1, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, R& ret)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(2);
	ToJSVal(cx, argv[0], a0);
	ToJSVal(cx, argv[1], a1);
	bool ok = CallFunction_(val1, name, 2, argv.begin(), &jsRet);
	if (!ok)
		return false;
	return FromJSVal(cx, jsRet, ret);
}

template<typename T0, typename T1, typename T2, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, R& ret)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(3);
	ToJSVal(cx, argv[0], a0);
	ToJSVal(cx, argv[1], a1);
	ToJSVal(cx, argv[2], a2);
	bool ok = CallFunction_(val1, name, 3, argv.begin(), &jsRet);
	if (!ok)
		return false;
	return FromJSVal(cx, jsRet, ret);
}

template<typename T0, typename T1, typename T2, typename T3, typename R>
bool ScriptInterface::CallFunction(jsval val, const char* name, const T0& a0, const T1& a1, const T2& a2, const T3& a3, R& ret)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::RootedValue val1(cx, val);
	JS::AutoValueVector argv(cx);
	argv.resize(4);
	ToJSVal(cx, argv[0], a0);
	ToJSVal(cx, argv[1], a1);
	ToJSVal(cx, argv[2], a2);
	ToJSVal(cx, argv[3], a3);
	bool ok = CallFunction_(val1, name, 4, argv.begin(), &jsRet);
	if (!ok)
		return false;
	return FromJSVal(cx, jsRet, ret);
}

template<typename T>
bool ScriptInterface::SetGlobal(const char* name, const T& value, bool replace)
{
	JS::Value val;
	ToJSVal(GetContext(), val, value);
	return SetGlobal_(name, val, replace);
}

template<typename T>
bool ScriptInterface::SetProperty(jsval obj, const char* name, const T& value, bool readonly, bool enumerate)
{
	JS::Value val;
	ToJSVal(GetContext(), val, value);
	return SetProperty_(obj, name, val, readonly, enumerate);
}

template<typename T>
bool ScriptInterface::SetProperty(jsval obj, const wchar_t* name, const T& value, bool readonly, bool enumerate)
{
	return SetProperty_(obj, name, ToJSVal(GetContext(), value), readonly, enumerate);
}

template<typename T>
bool ScriptInterface::SetPropertyInt(jsval obj, int name, const T& value, bool readonly, bool enumerate)
{
	JS::Value val;
	ToJSVal(GetContext(), val, value);
	return SetPropertyInt_(obj, name, val, readonly, enumerate);
}

template<typename T>
bool ScriptInterface::GetProperty(jsval obj, const char* name, T& out)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue val(cx);
	if (! GetProperty_(obj, name, &val))
		return false;
	return FromJSVal(cx, val, out);
}

template<typename T>
bool ScriptInterface::GetPropertyInt(jsval obj, int name, T& out)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	if (! GetPropertyInt_(obj, name, &val))
		return false;
	return FromJSVal(GetContext(), val, out);
}

template<typename T, typename CHAR>
bool ScriptInterface::Eval(const CHAR* code, T& ret)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue rval(GetContext());
	if (! Eval_(code, &rval))
		return false;
	return FromJSVal(GetContext(), rval, ret);
}

#endif // INCLUDED_SCRIPTINTERFACE
