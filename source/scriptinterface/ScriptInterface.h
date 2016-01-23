/* Copyright (C) 2016 Wildfire Games.
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

#include <boost/random/linear_congruential.hpp>

#include "lib/file/vfs/vfs_path.h"

#include "ScriptTypes.h"
#include "ScriptVal.h"
#include "ps/Errors.h"
#include "ps/Profile.h"

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

// Set the maximum number of function arguments that can be handled
// (This should be as small as possible (for compiler efficiency),
// but as large as necessary for all wrapped functions)
#define SCRIPT_INTERFACE_MAX_ARGS 8

// TODO: what's a good default?
#define DEFAULT_RUNTIME_SIZE 16 * 1024 * 1024
#define DEFAULT_HEAP_GROWTH_BYTES_GCTRIGGER 2 * 1024 *1024

struct ScriptInterface_impl;

class ScriptRuntime;

extern shared_ptr<ScriptRuntime> g_ScriptRuntime;


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
	NONCOPYABLE(ScriptInterface);
	
public:

	/**
	 * Returns a runtime, which can used to initialise any number of
	 * ScriptInterfaces contexts. Values created in one context may be used
	 * in any other context from the same runtime (but not any other runtime).
	 * Each runtime should only ever be used on a single thread.
	 * @param runtimeSize Maximum size in bytes of the new runtime
	 */
	static shared_ptr<ScriptRuntime> CreateRuntime(shared_ptr<ScriptRuntime> parentRuntime = shared_ptr<ScriptRuntime>(), int runtimeSize = DEFAULT_RUNTIME_SIZE, 
		int heapGrowthBytesGCTrigger = DEFAULT_HEAP_GROWTH_BYTES_GCTRIGGER);


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
	JS::Value GetCachedValue(CACHED_VAL valueIdentifier);

	/**
	 * Replace the default JS random number geenrator with a seeded, network-sync'd one.
	 */
	bool ReplaceNondeterministicRNG(boost::rand48& rng);

	/**
	 * Call a constructor function, equivalent to JS "new ctor(arg)".
	 * @param ctor An object that can be used as constructor
	 * @param argv Constructor arguments
	 * @param out The new object; On error an error message gets logged and out is Null (out.isNull() == true).
	 */
	void CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out);

	/**
	 * Call the named property on the given object, with void return type and 0 arguments
	 */
	bool CallFunctionVoid(JS::HandleValue val, const char* name);

	/**
	 * Call the named property on the given object, with void return type and 1 argument
	 */
	template<typename T0>
	bool CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0);

	/**
	 * Call the named property on the given object, with void return type and 2 arguments
	 */
	template<typename T0, typename T1>
	bool CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0, const T1& a1);

	/**
	 * Call the named property on the given object, with void return type and 3 arguments
	 */
	template<typename T0, typename T1, typename T2>
	bool CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0, const T1& a1, const T2& a2);

	JSObject* CreateCustomObject(const std::string & typeName) const;
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
	bool SetProperty(JS::HandleValue obj, const char* name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Set the named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetProperty(JS::HandleValue obj, const wchar_t* name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Set the integer-named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetPropertyInt(JS::HandleValue obj, int name, const T& value, bool constant = false, bool enumerate = true);

	/**
	 * Get the named property on the given object.
	 */
	template<typename T>
	bool GetProperty(JS::HandleValue obj, const char* name, T& out);
	
	/**
	 * Get the named property of the given object.
	 */
	bool GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out);
	bool GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out);

	/**
	 * Get the integer-named property on the given object.
	 */
	template<typename T>
	bool GetPropertyInt(JS::HandleValue obj, int name, T& out);
	
	/**
	 * Get the named property of the given object.
	 */
	bool GetPropertyInt(JS::HandleValue obj, int name, JS::MutableHandleValue out);

	/**
	 * Check the named property has been defined on the given object.
	 */
	bool HasProperty(JS::HandleValue obj, const char* name);

	bool EnumeratePropertyNamesWithPrefix(JS::HandleValue objVal, const char* prefix, std::vector<std::string>& out);

	bool SetPrototype(JS::HandleValue obj, JS::HandleValue proto);

	bool FreezeObject(JS::HandleValue objVal, bool deep);

	bool Eval(const char* code);

	template<typename CHAR> bool Eval(const CHAR* code, JS::MutableHandleValue out);
	template<typename T, typename CHAR> bool Eval(const CHAR* code, T& out);

	/**
	 * Convert an object to a UTF-8 encoded string, either with JSON
	 * (if pretty == true and there is no JSON error) or with toSource().
	 *
	 * We have to use a mutable handle because JS_Stringify requires that for unknown reasons.
	 */
	std::string ToString(JS::MutableHandleValue obj, bool pretty = false);

	/**
	 * Parse a UTF-8-encoded JSON string. Returns the unmodified value on error
	 * and prints an error message.
	 * @return true on success; false otherwise
	 */
	bool ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out);

	/**
	 * Read a JSON file. Returns the unmodified value on error and prints an error message.
	 */
	void ReadJSONFile(const VfsPath& path, JS::MutableHandleValue out);

	/**
	 * Stringify to a JSON string, UTF-8 encoded. Returns an empty string on error.
	 */
	std::string StringifyJSON(JS::MutableHandleValue obj, bool indent = true);
	
	/**
	 * Report the given error message through the JS error reporting mechanism,
	 * and throw a JS exception. (Callers can check IsPendingException, and must
	 * return false in that case to propagate the exception.)
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
	JS::Value CloneValueFromOtherContext(ScriptInterface& otherContext, JS::HandleValue val);

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
	template<typename T> static void ToJSVal(JSContext* cx, JS::MutableHandleValue ret, T const& val);

	/**
	 * MaybeGC tries to determine whether garbage collection in cx's runtime would free up enough memory to be worth the amount of time it would take.
	 * This calls JS_MaybeGC directly, which does not do incremental GC. Usually you should prefer MaybeIncrementalRuntimeGC.
	 */
	void MaybeGC();
	
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

	shared_ptr<StructuredClone> WriteStructuredClone(JS::HandleValue v);
	void ReadStructuredClone(const shared_ptr<StructuredClone>& ptr, JS::MutableHandleValue ret);

	/**
	 * Converts |a| if needed and assigns it to |handle|.
	 * This is meant for use in other templates where we want to use the same code for JS::RootedValue&/JS::HandleValue and
	 * other types. Note that functions are meant to take JS::HandleValue instead of JS::RootedValue&, but this implicit
	 * conversion does not work for templates (exact type matches required for type deduction).
	 * A similar functionality could also be implemented as a ToJSVal specialization. The current approach was preferred
	 * because "conversions" from JS::HandleValue to JS::MutableHandleValue are unusual and should not happen "by accident".
	 */
	template <typename T>
	static void AssignOrToJSVal(JSContext* cx, JS::MutableHandleValue handle, const T& a);

	/**
	 * The same as AssignOrToJSVal, but also allows JS::Value for T.
	 * In most cases it's not safe to use the plain (unrooted) JS::Value type, but this can happen quite
	 * easily with template functions. The idea is that the linker prints an error if AssignOrToJSVal is
	 * used with JS::Value. If the specialization for JS::Value should be allowed, you can use this 
	 * "unrooted" version of AssignOrToJSVal.
	 */
	template <typename T>
	static void AssignOrToJSValUnrooted(JSContext* cx, JS::MutableHandleValue handle, const T& a)
	{
		AssignOrToJSVal(cx, handle, a);
	}
	
	/**
	 * Converts |val| to T if needed or just returns it if it's a handle.
	 * This is meant for use in other templates where we want to use the same code for JS::HandleValue and
	 * other types.
	 */
	template <typename T>
	static T AssignOrFromJSVal(JSContext* cx, const JS::HandleValue& val, bool& ret);

private:
	
	bool CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret);
	bool Eval_(const char* code, JS::MutableHandleValue ret);
	bool Eval_(const wchar_t* code, JS::MutableHandleValue ret);
	bool SetGlobal_(const char* name, JS::HandleValue value, bool replace);
	bool SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool readonly, bool enumerate);
	bool SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool readonly, bool enumerate);
	bool SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool readonly, bool enumerate);
	bool GetProperty_(JS::HandleValue obj, const char* name, JS::MutableHandleValue out);
	bool GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue value);
	static bool IsExceptionPending(JSContext* cx);
	static const JSClass* GetClass(JS::HandleObject obj);
	static void* GetPrivate(JS::HandleObject obj);

	struct CustomType
	{
		// TODO: Move assignment operator and move constructor only have to be
		// explicitly defined for Visual Studio. VS2013 is still behind on C++11 support
		// What's missing is what they call "Rvalue references v3.0", see
		// https://msdn.microsoft.com/en-us/library/hh567368.aspx#rvref
		CustomType() {}
		CustomType& operator=(CustomType&& other)
		{
			m_Prototype = std::move(other.m_Prototype);
			m_Class = std::move(other.m_Class);
			m_Constructor = std::move(other.m_Constructor);
			return *this;
		}
		CustomType(CustomType&& other)
		{
			m_Prototype = std::move(other.m_Prototype);
			m_Class = std::move(other.m_Class);
			m_Constructor = std::move(other.m_Constructor);
		}

		DefPersistentRooted<JSObject*>	m_Prototype;
		JSClass*	m_Class;
		JSNative 	m_Constructor;
	};
	void Register(const char* name, JSNative fptr, size_t nargs);
	
	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the runtime destructor.
	std::unique_ptr<ScriptInterface_impl> m;
	
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

template<typename T>
inline void ScriptInterface::AssignOrToJSVal(JSContext* cx, JS::MutableHandleValue handle, const T& a)
{
	ToJSVal(cx, handle, a);
}

template<>
inline void ScriptInterface::AssignOrToJSVal<JS::PersistentRootedValue>(JSContext* UNUSED(cx), JS::MutableHandleValue handle, const JS::PersistentRootedValue& a)
{
	handle.set(a);
}

template<>
inline void ScriptInterface::AssignOrToJSVal<JS::RootedValue>(JSContext* UNUSED(cx), JS::MutableHandleValue handle, const JS::RootedValue& a)
{
	handle.set(a);
}

template <>
inline void ScriptInterface::AssignOrToJSVal<JS::HandleValue>(JSContext* UNUSED(cx), JS::MutableHandleValue handle, const JS::HandleValue& a)
{
	handle.set(a);
}

template <>
inline void ScriptInterface::AssignOrToJSValUnrooted<JS::Value>(JSContext* UNUSED(cx), JS::MutableHandleValue handle, const JS::Value& a)
{
	handle.set(a);
}

template<typename T>
inline T ScriptInterface::AssignOrFromJSVal(JSContext* cx, const JS::HandleValue& val, bool& ret)
{
	T retVal;
	ret = FromJSVal(cx, val, retVal);
	return retVal;
}

template<>
inline JS::HandleValue ScriptInterface::AssignOrFromJSVal<JS::HandleValue>(JSContext* UNUSED(cx), const JS::HandleValue& val, bool& ret)
{
	ret = true;
	return val;
}

template<typename T0>
bool ScriptInterface::CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(1);
	AssignOrToJSVal(cx, argv.handleAt(0), a0);
	return CallFunction_(val, name, argv, &jsRet);
}

template<typename T0, typename T1>
bool ScriptInterface::CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0, const T1& a1)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(2);
	AssignOrToJSVal(cx, argv.handleAt(0), a0);
	AssignOrToJSVal(cx, argv.handleAt(1), a1);
	return CallFunction_(val, name, argv, &jsRet);
}

template<typename T0, typename T1, typename T2>
bool ScriptInterface::CallFunctionVoid(JS::HandleValue val, const char* name, const T0& a0, const T1& a1, const T2& a2)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue jsRet(cx);
	JS::AutoValueVector argv(cx);
	argv.resize(3);
	AssignOrToJSVal(cx, argv.handleAt(0), a0);
	AssignOrToJSVal(cx, argv.handleAt(1), a1);
	AssignOrToJSVal(cx, argv.handleAt(2), a2);
	return CallFunction_(val, name, argv, &jsRet);
}

template<typename T>
bool ScriptInterface::SetGlobal(const char* name, const T& value, bool replace)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	AssignOrToJSVal(GetContext(), &val, value);
	return SetGlobal_(name, val, replace);
}

template<typename T>
bool ScriptInterface::SetProperty(JS::HandleValue obj, const char* name, const T& value, bool readonly, bool enumerate)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	AssignOrToJSVal(GetContext(), &val, value);
	return SetProperty_(obj, name, val, readonly, enumerate);
}

template<typename T>
bool ScriptInterface::SetProperty(JS::HandleValue obj, const wchar_t* name, const T& value, bool readonly, bool enumerate)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	AssignOrToJSVal(GetContext(), &val, value);
	return SetProperty_(obj, name, val, readonly, enumerate);
}

template<typename T>
bool ScriptInterface::SetPropertyInt(JS::HandleValue obj, int name, const T& value, bool readonly, bool enumerate)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	AssignOrToJSVal(GetContext(), &val, value);
	return SetPropertyInt_(obj, name, val, readonly, enumerate);
}

template<typename T>
bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, T& out)
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue val(cx);
	if (! GetProperty_(obj, name, &val))
		return false;
	return FromJSVal(cx, val, out);
}

template<typename T>
bool ScriptInterface::GetPropertyInt(JS::HandleValue obj, int name, T& out)
{
	JSAutoRequest rq(GetContext());
	JS::RootedValue val(GetContext());
	if (! GetPropertyInt_(obj, name, &val))
		return false;
	return FromJSVal(GetContext(), val, out);
}

template<typename CHAR>
bool ScriptInterface::Eval(const CHAR* code, JS::MutableHandleValue ret)
{
	if (! Eval_(code, ret))
		return false;
	return true;
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
