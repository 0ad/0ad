/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SCRIPTINTERFACE
#define INCLUDED_SCRIPTINTERFACE

#include "ps/Errors.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptExceptions.h"
#include "scriptinterface/ScriptRequest.h"
#include "scriptinterface/ScriptTypes.h"

#include <map>

ERROR_GROUP(Scripting);
ERROR_TYPE(Scripting, SetupFailed);

ERROR_SUBGROUP(Scripting, LoadFile);
ERROR_TYPE(Scripting_LoadFile, OpenFailed);
ERROR_TYPE(Scripting_LoadFile, EvalErrors);

ERROR_TYPE(Scripting, CallFunctionFailed);
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

class ScriptInterface;
struct ScriptInterface_impl;

class ScriptContext;
// Using a global object for the context is a workaround until Simulation, AI, etc,
// use their own threads and also their own contexts.
extern thread_local std::shared_ptr<ScriptContext> g_ScriptContext;

namespace boost { namespace random { class rand48; } }

class Path;
using VfsPath = Path;

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
	 * @param nativeScopeName Name of global object that functions (via ScriptFunction::Register) will
	 *   be placed into, as a scoping mechanism; typically "Engine"
	 * @param debugName Name of this interface for CScriptStats purposes.
	 * @param context ScriptContext to use when initializing this interface.
	 */
	ScriptInterface(const char* nativeScopeName, const char* debugName, const std::shared_ptr<ScriptContext>& context);

	/**
	 * Alternate constructor. This creates the new Realm in the same Compartment as the neighbor scriptInterface.
	 * This means that data can be freely exchanged between these two script interfaces without cloning.
	 * @param nativeScopeName Name of global object that functions (via ScriptFunction::Register) will
	 *   be placed into, as a scoping mechanism; typically "Engine"
	 * @param debugName Name of this interface for CScriptStats purposes.
	 * @param scriptInterface 'Neighbor' scriptInterface to share a compartment with.
	 */
	ScriptInterface(const char* nativeScopeName, const char* debugName, const ScriptInterface& neighbor);

	~ScriptInterface();

	struct CmptPrivate
	{
		friend class ScriptInterface;
	public:
		static const ScriptInterface& GetScriptInterface(JSContext* cx);
		static void* GetCBData(JSContext* cx);
	protected:
		ScriptInterface* pScriptInterface; // the ScriptInterface object the compartment belongs to
		void* pCBData; // meant to be used as the "this" object for callback functions
	};

	void SetCallbackData(void* pCBData);

	/**
	 * Convert the CmptPrivate callback data to T*
	 */
	template <typename T>
	static T* ObjectFromCBData(const ScriptRequest& rq)
	{
		static_assert(!std::is_same_v<void, T>);
		return static_cast<T*>(ObjectFromCBData<void>(rq));
	}

	/**
	 * Variant for the function wrapper.
	 */
	template <typename T>
	static T* ObjectFromCBData(const ScriptRequest& rq, JS::CallArgs&)
	{
		return ObjectFromCBData<T>(rq);
	}

	/**
	 * GetGeneralJSContext returns the context without starting a GC request and without
	 * entering the ScriptInterface compartment. It should only be used in specific situations,
	 * for instance when initializing a persistent rooted.
	 * If you need the compartmented context of the ScriptInterface, you should create a
	 * ScriptInterface::Request and use the context from that.
	 */
	JSContext* GetGeneralJSContext() const;
	std::shared_ptr<ScriptContext> GetContext() const;

	/**
	 * Load global scripts that most script interfaces need,
	 * located in the /globalscripts directory. VFS must be initialized.
	 */
	bool LoadGlobalScripts();

	/**
	 * Replace the default JS random number generator with a seeded, network-synced one.
	 */
	bool ReplaceNondeterministicRNG(boost::random::rand48& rng);

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
	 * Set the named property on the global object.
	 * Optionally makes it {ReadOnly, DontEnum}. We do not allow to make it DontDelete, so that it can be hotloaded
	 * by deleting it and re-creating it, which is done by setting @p replace to true.
	 */
	template<typename T>
	bool SetGlobal(const char* name, const T& value, bool replace = false, bool constant = true, bool enumerate = true);

	/**
	 * Get an object from the global scope or any lexical scope.
	 * This can return globally accessible objects even if they are not properties
	 * of the global object (e.g. ES6 class definitions).
	 * @param name - Name of the property.
	 * @param out The object or null.
	 */
	static bool GetGlobalProperty(const ScriptRequest& rq, const std::string& name, JS::MutableHandleValue out);

	bool SetPrototype(JS::HandleValue obj, JS::HandleValue proto);

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
	 * Calls the random number generator assigned to this ScriptInterface instance and returns the generated number.
	 */
	bool MathRandom(double& nbr) const;

	/**
	 * JSNative wrapper of the above.
	 */
	static bool Math_random(JSContext* cx, uint argc, JS::Value* vp);

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

private:
	bool SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate);

	struct CustomType
	{
		JS::PersistentRootedObject m_Prototype;
		JSClass* m_Class;
		JSNative m_Constructor;
	};

	CmptPrivate m_CmptPrivate;

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the custom destructor of ScriptInterface_impl.
	std::unique_ptr<ScriptInterface_impl> m;

	std::map<std::string, CustomType> m_CustomObjectTypes;
};

// Explicitly instantiate void* as that is used for the generic template,
// and we want to define it in the .cpp file.
template <> void* ScriptInterface::ObjectFromCBData(const ScriptRequest& rq);

template<typename T>
bool ScriptInterface::SetGlobal(const char* name, const T& value, bool replace, bool constant, bool enumerate)
{
	ScriptRequest rq(this);
	JS::RootedValue val(rq.cx);
	Script::ToJSVal(rq, &val, value);
	return SetGlobal_(name, val, replace, constant, enumerate);
}

template<typename T>
bool ScriptInterface::Eval(const char* code, T& ret) const
{
	ScriptRequest rq(this);
	JS::RootedValue rval(rq.cx);
	if (!Eval(code, &rval))
		return false;
	return Script::FromJSVal(rq, rval, ret);
}

#endif // INCLUDED_SCRIPTINTERFACE
