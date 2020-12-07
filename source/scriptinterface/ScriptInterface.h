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

#ifndef INCLUDED_SCRIPTINTERFACE
#define INCLUDED_SCRIPTINTERFACE

#include "lib/file/vfs/vfs_path.h"
#include "maths/Fixed.h"
#include "ps/Errors.h"
#include "scriptinterface/ScriptExceptions.h"
#include "scriptinterface/ScriptTypes.h"

#include <boost/random/linear_congruential.hpp>
#include <map>

ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, SetupFailed);

ERROR_SUBGROUP(Scripting, LoadFile);
ERROR_TYPE(Scripting_LoadFile, OpenFailed);
ERROR_TYPE(Scripting_LoadFile, EvalErrors);

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

class JSStructuredCloneData;

class ScriptInterface;
struct ScriptInterface_impl;

class ScriptContext;
// Using a global object for the context is a workaround until Simulation, AI, etc,
// use their own threads and also their own contexts.
extern thread_local shared_ptr<ScriptContext> g_ScriptContext;

/**
 * RAII structure which encapsulates an access to the context and compartment of a ScriptInterface.
 * This struct provides:
 * - a pointer to the context, while acting like JSAutoRequest
 * - a pointer to the global object of the compartment, while acting like JSAutoRealm
 *
 * This way, getting and using those pointers is safe with respect to the GC
 * and to the separation of compartments.
 */
class ScriptRequest
{
public:
	ScriptRequest() = delete;
	ScriptRequest(const ScriptRequest& rq) = delete;
	ScriptRequest& operator=(const ScriptRequest& rq) = delete;
	ScriptRequest(const ScriptInterface& scriptInterface);
	ScriptRequest(const ScriptInterface* scriptInterface) : ScriptRequest(*scriptInterface) {}
	ScriptRequest(shared_ptr<ScriptInterface> scriptInterface) : ScriptRequest(*scriptInterface) {}
	~ScriptRequest();

	JS::Value globalValue() const;
	JSContext* cx;
	JSObject* glob;
private:
	JS::Realm* m_formerRealm;
};

/**
 * Abstraction around a SpiderMonkey JS::Realm.
 *
 * Thread-safety:
 * - May be used in non-main threads.
 * - Each ScriptInterface must be created, used, and destroyed, all in a single thread
 *   (it must never be shared between threads).
 */
class ScriptInterface
{
	NONCOPYABLE(ScriptInterface);

	friend class ScriptRequest;
public:

	/**
	 * Constructor.
	 * @param nativeScopeName Name of global object that functions (via RegisterFunction) will
	 *   be placed into, as a scoping mechanism; typically "Engine"
	 * @param debugName Name of this interface for CScriptStats purposes.
	 * @param context ScriptContext to use when initializing this interface.
	 */
	ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptContext>& context);

	~ScriptInterface();

	struct CmptPrivate
	{
		ScriptInterface* pScriptInterface; // the ScriptInterface object the compartment belongs to
		void* pCBData; // meant to be used as the "this" object for callback functions
	} m_CmptPrivate;

	void SetCallbackData(void* pCBData);
	static CmptPrivate* GetScriptInterfaceAndCBData(JSContext* cx);

	/**
	 * GetGeneralJSContext returns the context without starting a GC request and without
	 * entering the ScriptInterface compartment. It should only be used in specific situations,
	 * for instance when initializing a persistent rooted.
	 * If you need the compartmented context of the ScriptInterface, you should create a
	 * ScriptInterface::Request and use the context from that.
	 */
	JSContext* GetGeneralJSContext() const;
	shared_ptr<ScriptContext> GetContext() const;

	/**
	 * Load global scripts that most script interfaces need,
	 * located in the /globalscripts directory. VFS must be initialized.
	 */
	bool LoadGlobalScripts();

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
	void CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out) const;

	JSObject* CreateCustomObject(const std::string & typeName) const;
	void DefineCustomObjectType(JSClass *clasp, JSNative constructor, uint minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

	/**
	 * Sets the given value to a new plain JS::Object, converts the arguments to JS::Values and sets them as properties.
	 * This is static so that callers like ToJSVal can use it with the JSContext directly instead of having to obtain the instance using GetScriptInterfaceAndCBData.
	 * Can throw an exception.
	 */
	template<typename... Args>
	static bool CreateObject(const ScriptRequest& rq, JS::MutableHandleValue objectValue, Args const&... args)
	{
		JS::RootedObject obj(rq.cx);

		if (!CreateObject_(rq, &obj, args...))
			return false;

		objectValue.setObject(*obj);
		return true;
	}

	/**
	 * Sets the given value to a new JS object or Null Value in case of out-of-memory.
	 */
	static void CreateArray(const ScriptRequest& rq, JS::MutableHandleValue objectValue, size_t length = 0);

	/**
	 * Set the named property on the global object.
	 * Optionally makes it {ReadOnly, DontEnum}. We do not allow to make it DontDelete, so that it can be hotloaded
	 * by deleting it and re-creating it, which is done by setting @p replace to true.
	 */
	template<typename T>
	bool SetGlobal(const char* name, const T& value, bool replace = false, bool constant = true, bool enumerate = true);

	/**
	 * Set the named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetProperty(JS::HandleValue obj, const char* name, const T& value, bool constant = false, bool enumerate = true) const;

	/**
	 * Set the named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetProperty(JS::HandleValue obj, const wchar_t* name, const T& value, bool constant = false, bool enumerate = true) const;

	/**
	 * Set the integer-named property on the given object.
	 * Optionally makes it {ReadOnly, DontDelete, DontEnum}.
	 */
	template<typename T>
	bool SetPropertyInt(JS::HandleValue obj, int name, const T& value, bool constant = false, bool enumerate = true) const;

	/**
	 * Get the named property on the given object.
	 */
	template<typename T>
	bool GetProperty(JS::HandleValue obj, const char* name, T& out) const;

	/**
	 * Get the named property of the given object.
	 */
	bool GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const;
	bool GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out) const;

	/**
	 * Get the integer-named property on the given object.
	 */
	template<typename T>
	bool GetPropertyInt(JS::HandleValue obj, int name, T& out) const;

	/**
	 * Get the named property of the given object.
	 */
	bool GetPropertyInt(JS::HandleValue obj, int name, JS::MutableHandleValue out) const;

	/**
	 * Check the named property has been defined on the given object.
	 */
	bool HasProperty(JS::HandleValue obj, const char* name) const;

	/**
	 * Returns all properties of the object, both own properties and inherited.
	 * This is essentially equivalent to calling Object.getOwnPropertyNames()
	 * and recursing up the prototype chain.
	 * NB: this does not return properties with symbol or numeric keys, as that would
	 * require a variant in the vector, and it's not useful for now.
	 * @param enumerableOnly - only return enumerable properties.
	 */
	bool EnumeratePropertyNames(JS::HandleValue objVal, bool enumerableOnly, std::vector<std::string>& out) const;

	bool SetPrototype(JS::HandleValue obj, JS::HandleValue proto);

	bool FreezeObject(JS::HandleValue objVal, bool deep) const;

	/**
	 * Convert an object to a UTF-8 encoded string, either with JSON
	 * (if pretty == true and there is no JSON error) or with toSource().
	 *
	 * We have to use a mutable handle because JS_Stringify requires that for unknown reasons.
	 */
	std::string ToString(JS::MutableHandleValue obj, bool pretty = false) const;

	/**
	 * Parse a UTF-8-encoded JSON string. Returns the unmodified value on error
	 * and prints an error message.
	 * @return true on success; false otherwise
	 */
	bool ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out) const;

	/**
	 * Read a JSON file. Returns the unmodified value on error and prints an error message.
	 */
	void ReadJSONFile(const VfsPath& path, JS::MutableHandleValue out) const;

	/**
	 * Stringify to a JSON string, UTF-8 encoded. Returns an empty string on error.
	 */
	std::string StringifyJSON(JS::MutableHandleValue obj, bool indent = true) const;

	/**
	 * Load and execute the given script in a new function scope.
	 * @param filename Name for debugging purposes (not used to load the file)
	 * @param code JS code to execute
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadScript(const VfsPath& filename, const std::string& code) const;

	/**
	 * Load and execute the given script in the global scope.
	 * @param filename Name for debugging purposes (not used to load the file)
	 * @param code JS code to execute
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadGlobalScript(const VfsPath& filename, const std::string& code) const;

	/**
	 * Load and execute the given script in the global scope.
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool LoadGlobalScriptFile(const VfsPath& path) const;

	/**
	 * Evaluate some JS code in the global scope.
	 * @return true on successful compilation and execution; false otherwise
	 */
	bool Eval(const char* code) const;
	bool Eval(const char* code, JS::MutableHandleValue out) const;
	template<typename T> bool Eval(const char* code, T& out) const;

	/**
	 * Convert a JS::Value to a C++ type. (This might trigger GC.)
	 */
	template<typename T> static bool FromJSVal(const ScriptRequest& rq, const JS::HandleValue val, T& ret);

	/**
	 * Convert a C++ type to a JS::Value. (This might trigger GC. The return
	 * value must be rooted if you don't want it to be collected.)
	 * NOTE: We are passing the JS::Value by reference instead of returning it by value.
	 * The reason is a memory corruption problem that appears to be caused by a bug in Visual Studio.
	 * Details here: http://www.wildfiregames.com/forum/index.php?showtopic=17289&p=285921
	 */
	template<typename T> static void ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue ret, T const& val);

	/**
	 * Convert a named property of an object to a C++ type.
	 */
	template<typename T> static bool FromJSProperty(const ScriptRequest& rq, const JS::HandleValue val, const char* name, T& ret, bool strict = false);

	/**
	 * MathRandom (this function) calls the random number generator assigned to this ScriptInterface instance and
	 * returns the generated number.
	 * Math_random (with underscore, not this function) is a global function, but different random number generators can be
	 * stored per ScriptInterface. It calls MathRandom of the current ScriptInterface instance.
	 */
	bool MathRandom(double& nbr);

	/**
	 * Structured clones are a way to serialize 'simple' JS::Values into a buffer
	 * that can safely be passed between compartments and between threads.
	 * A StructuredClone can be stored and read multiple times if desired.
	 * We wrap them in shared_ptr so memory management is automatic and
	 * thread-safe.
	 */
	using StructuredClone = shared_ptr<JSStructuredCloneData>;

	StructuredClone WriteStructuredClone(JS::HandleValue v) const;
	void ReadStructuredClone(const StructuredClone& ptr, JS::MutableHandleValue ret) const;

	/**
	 * Construct a new value (usable in this ScriptInterface's compartment) by cloning
	 * a value from a different compartment.
	 * Complex values (functions, XML, etc) won't be cloned correctly, but basic
	 * types and cyclic references should be fine.
	 */
	JS::Value CloneValueFromOtherCompartment(const ScriptInterface& otherCompartment, JS::HandleValue val) const;

	/**
	 * Retrieve the private data field of a JSObject that is an instance of the given JSClass.
	 */
	template <typename T>
	static T* GetPrivate(const ScriptRequest& rq, JS::HandleObject thisobj, JSClass* jsClass)
	{
		T* value = static_cast<T*>(JS_GetInstancePrivate(rq.cx, thisobj, jsClass, nullptr));

		if (value == nullptr)
			ScriptException::Raise(rq, "Private data of the given object is null!");

		return value;
	}

	/**
	 * Retrieve the private data field of a JS Object that is an instance of the given JSClass.
	 * If an error occurs, GetPrivate will report it with the according stack.
	 */
	template <typename T>
	static T* GetPrivate(const ScriptRequest& rq, JS::CallArgs& callArgs, JSClass* jsClass)
	{
		if (!callArgs.thisv().isObject())
		{
			ScriptException::Raise(rq, "Cannot retrieve private JS class data because from a non-object value!");
			return nullptr;
		}

		JS::RootedObject thisObj(rq.cx, &callArgs.thisv().toObject());
		T* value = static_cast<T*>(JS_GetInstancePrivate(rq.cx, thisObj, jsClass, &callArgs));

		if (value == nullptr)
			ScriptException::Raise(rq, "Private data of the given object is null!");

		return value;
	}

	/**
	 * Converts |a| if needed and assigns it to |handle|.
	 * This is meant for use in other templates where we want to use the same code for JS::RootedValue&/JS::HandleValue and
	 * other types. Note that functions are meant to take JS::HandleValue instead of JS::RootedValue&, but this implicit
	 * conversion does not work for templates (exact type matches required for type deduction).
	 * A similar functionality could also be implemented as a ToJSVal specialization. The current approach was preferred
	 * because "conversions" from JS::HandleValue to JS::MutableHandleValue are unusual and should not happen "by accident".
	 */
	template <typename T>
	static void AssignOrToJSVal(const ScriptRequest& rq, JS::MutableHandleValue handle, const T& a);

	/**
	 * The same as AssignOrToJSVal, but also allows JS::Value for T.
	 * In most cases it's not safe to use the plain (unrooted) JS::Value type, but this can happen quite
	 * easily with template functions. The idea is that the linker prints an error if AssignOrToJSVal is
	 * used with JS::Value. If the specialization for JS::Value should be allowed, you can use this
	 * "unrooted" version of AssignOrToJSVal.
	 */
	template <typename T>
	static void AssignOrToJSValUnrooted(const ScriptRequest& rq, JS::MutableHandleValue handle, const T& a)
	{
		AssignOrToJSVal(rq, handle, a);
	}

	/**
	 * Converts |val| to T if needed or just returns it if it's a handle.
	 * This is meant for use in other templates where we want to use the same code for JS::HandleValue and
	 * other types.
	 */
	template <typename T>
	static T AssignOrFromJSVal(const ScriptRequest& rq, const JS::HandleValue& val, bool& ret);

private:

	static bool CreateObject_(const ScriptRequest& rq, JS::MutableHandleObject obj);

	template<typename T, typename... Args>
	static bool CreateObject_(const ScriptRequest& rq, JS::MutableHandleObject obj, const char* propertyName, const T& propertyValue, Args const&... args)
	{
		JS::RootedValue val(rq.cx);
		AssignOrToJSVal(rq, &val, propertyValue);

		return CreateObject_(rq, obj, args...) && JS_DefineProperty(rq.cx, obj, propertyName, val, JSPROP_ENUMERATE);
	}

	bool CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret) const;
	bool SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate);
	bool SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool constant, bool enumerate) const;
	bool SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool constant, bool enumerate) const;
	bool SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool constant, bool enumerate) const;
	bool GetProperty_(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const;
	bool GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue value) const;

	struct CustomType
	{
		JS::PersistentRootedObject m_Prototype;
		JSClass* m_Class;
		JSNative m_Constructor;
	};
	void Register(const char* name, JSNative fptr, size_t nargs) const;

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the custom destructor of ScriptInterface_impl.
	std::unique_ptr<ScriptInterface_impl> m;

	boost::rand48* m_rng;
	std::map<std::string, CustomType> m_CustomObjectTypes;

// The nasty macro/template bits are split into a separate file so you don't have to look at them
public:
	#include "NativeWrapperDecls.h"
	// This declares:
	//
	//   template <R, T0..., TR (*fptr) (void* cbdata, T0...)>
	//   void RegisterFunction(const char* functionName) const;
	//
	//   template <R, T0..., TR (*fptr) (void* cbdata, T0...)>
	//   static JSNative call;
	//
	//   template <R, T0..., JSClass*, TC, TR (TC:*fptr) (T0...)>
	//   static JSNative callMethod;
	//
	//   template <R, T0..., JSClass*, TC, TR (TC:*fptr) const (T0...)>
	//   static JSNative callMethodConst;
	//
	//   template <T0...>
	//   static size_t nargs();
	//
	//   template <R, T0...>
	//   bool CallFunction(JS::HandleValue val, const char* name, R& ret, const T0&...) const;
	//
	//   template <R, T0...>
	//   bool CallFunction(JS::HandleValue val, const char* name, JS::Rooted<R>* ret, const T0&...) const;
	//
	//   template <R, T0...>
	//   bool CallFunction(JS::HandleValue val, const char* name, JS::MutableHandle<R> ret, const T0&...) const;
	//
	//   template <T0...>
	//   bool CallFunctionVoid(JS::HandleValue val, const char* name, const T0&...) const;
};

// Implement those declared functions
#include "NativeWrapperDefns.h"

template<typename T>
inline void ScriptInterface::AssignOrToJSVal(const ScriptRequest& rq, JS::MutableHandleValue handle, const T& a)
{
	ToJSVal(rq, handle, a);
}

template<>
inline void ScriptInterface::AssignOrToJSVal<JS::PersistentRootedValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::PersistentRootedValue& a)
{
	handle.set(a);
}

template<>
inline void ScriptInterface::AssignOrToJSVal<JS::Heap<JS::Value> >(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::Heap<JS::Value>& a)
{
	handle.set(a);
}

template<>
inline void ScriptInterface::AssignOrToJSVal<JS::RootedValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::RootedValue& a)
{
	handle.set(a);
}

template <>
inline void ScriptInterface::AssignOrToJSVal<JS::HandleValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::HandleValue& a)
{
	handle.set(a);
}

template <>
inline void ScriptInterface::AssignOrToJSValUnrooted<JS::Value>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::Value& a)
{
	handle.set(a);
}

template<typename T>
inline T ScriptInterface::AssignOrFromJSVal(const ScriptRequest& rq, const JS::HandleValue& val, bool& ret)
{
	T retVal;
	ret = FromJSVal(rq, val, retVal);
	return retVal;
}

template<>
inline JS::HandleValue ScriptInterface::AssignOrFromJSVal<JS::HandleValue>(const ScriptRequest& UNUSED(rq), const JS::HandleValue& val, bool& ret)
{
	ret = true;
	return val;
}

template<typename T>
bool ScriptInterface::SetGlobal(const char* name, const T& value, bool replace, bool constant, bool enumerate)
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	AssignOrToJSVal(rq, &val, value);
	return SetGlobal_(name, val, replace, constant, enumerate);
}

template<typename T>
bool ScriptInterface::SetProperty(JS::HandleValue obj, const char* name, const T& value, bool constant, bool enumerate) const
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	AssignOrToJSVal(rq, &val, value);
	return SetProperty_(obj, name, val, constant, enumerate);
}

template<typename T>
bool ScriptInterface::SetProperty(JS::HandleValue obj, const wchar_t* name, const T& value, bool constant, bool enumerate) const
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	AssignOrToJSVal(rq, &val, value);
	return SetProperty_(obj, name, val, constant, enumerate);
}

template<typename T>
bool ScriptInterface::SetPropertyInt(JS::HandleValue obj, int name, const T& value, bool constant, bool enumerate) const
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	AssignOrToJSVal(rq, &val, value);
	return SetPropertyInt_(obj, name, val, constant, enumerate);
}

template<typename T>
bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, T& out) const
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	if (!GetProperty_(obj, name, &val))
		return false;
	return FromJSVal(rq, val, out);
}

template<typename T>
bool ScriptInterface::GetPropertyInt(JS::HandleValue obj, int name, T& out) const
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	if (!GetPropertyInt_(obj, name, &val))
		return false;
	return FromJSVal(rq, val, out);
}


template<typename T>
bool ScriptInterface::Eval(const char* code, T& ret) const
{
	ScriptRequest rq(this);
	JS::RootedValue rval(rq.cx);
	if (!Eval(code, &rval))
		return false;
	return FromJSVal(rq, rval, ret);
}

#endif // INCLUDED_SCRIPTINTERFACE
