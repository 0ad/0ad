/* Copyright (C) 2021 Wildfire Games.
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

#include "precompiled.h"

#include "FunctionWrapper.h"
#include "ScriptContext.h"
#include "ScriptExtraHeaders.h"
#include "ScriptInterface.h"
#include "ScriptStats.h"
#include "StructuredClone.h"

#include "lib/debug.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

#include <map>
#include <string>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/flyweight/no_locking.hpp>
#include <boost/flyweight/no_tracking.hpp>

/**
 * @file
 * Abstractions of various SpiderMonkey features.
 * Engine code should be using functions of these interfaces rather than
 * directly accessing the underlying JS api.
 */

struct ScriptInterface_impl
{
	ScriptInterface_impl(const char* nativeScopeName, const std::shared_ptr<ScriptContext>& context);
	~ScriptInterface_impl();

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the context destructor.
	std::shared_ptr<ScriptContext> m_context;

	friend ScriptRequest;
	private:
		JSContext* m_cx;
		JS::PersistentRootedObject m_glob; // global scope object

	public:
		boost::rand48* m_rng;
		JS::PersistentRootedObject m_nativeScope; // native function scope object
};

/**
 * Constructor for ScriptRequest - here because it needs access into ScriptInterface_impl.
 */
ScriptRequest::ScriptRequest(const ScriptInterface& scriptInterface) :
	cx(scriptInterface.m->m_cx),
	glob(scriptInterface.m->m_glob),
	nativeScope(scriptInterface.m->m_nativeScope),
	m_ScriptInterface(scriptInterface)
{
	m_FormerRealm = JS::EnterRealm(cx, scriptInterface.m->m_glob);
}

ScriptRequest::~ScriptRequest()
{
	JS::LeaveRealm(cx, m_FormerRealm);
}

ScriptRequest::ScriptRequest(JSContext* cx) : ScriptRequest(ScriptInterface::CmptPrivate::GetScriptInterface(cx))
{
}

JS::Value ScriptRequest::globalValue() const
{
	return JS::ObjectValue(*glob);
}

const ScriptInterface& ScriptRequest::GetScriptInterface() const
{
	return m_ScriptInterface;
}

namespace
{

JSClassOps global_classops = {
	nullptr, nullptr,
	nullptr, nullptr,
	nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr,
	JS_GlobalObjectTraceHook
};

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS, &global_classops
};

// Functions in the global namespace:

bool print(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	ScriptRequest rq(cx);

	for (uint i = 0; i < args.length(); ++i)
	{
		std::wstring str;
		if (!Script::FromJSVal(rq, args[i], str))
			return false;
		debug_printf("%s", utf8_from_wstring(str).c_str());
	}
	fflush(stdout);
	args.rval().setUndefined();
	return true;
}

bool logmsg(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() < 1)
	{
		args.rval().setUndefined();
		return true;
	}

	ScriptRequest rq(cx);
	std::wstring str;
	if (!Script::FromJSVal(rq, args[0], str))
		return false;
	LOGMESSAGE("%s", utf8_from_wstring(str));
	args.rval().setUndefined();
	return true;
}

bool warn(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() < 1)
	{
		args.rval().setUndefined();
		return true;
	}

	ScriptRequest rq(cx);
	std::wstring str;
	if (!Script::FromJSVal(rq, args[0], str))
		return false;
	LOGWARNING("%s", utf8_from_wstring(str));
	args.rval().setUndefined();
	return true;
}

bool error(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() < 1)
	{
		args.rval().setUndefined();
		return true;
	}

	ScriptRequest rq(cx);
	std::wstring str;
	if (!Script::FromJSVal(rq, args[0], str))
		return false;
	LOGERROR("%s", utf8_from_wstring(str));
	args.rval().setUndefined();
	return true;
}

JS::Value deepcopy(const ScriptRequest& rq, JS::HandleValue val)
{
	if (val.isNullOrUndefined())
	{
		ScriptException::Raise(rq, "deepcopy requires one argument.");
		return JS::UndefinedValue();
	}

	JS::RootedValue ret(rq.cx, Script::DeepCopy(rq, val));
	if (ret.isNullOrUndefined())
	{
		ScriptException::Raise(rq, "deepcopy StructureClone copy failed.");
		return JS::UndefinedValue();
	}
	return ret;
}

JS::Value deepfreeze(const ScriptInterface& scriptInterface, JS::HandleValue val)
{
	ScriptRequest rq(scriptInterface);
	if (!val.isObject())
	{
		ScriptException::Raise(rq, "deepfreeze requires exactly one object as an argument.");
		return JS::UndefinedValue();
	}

	Script::FreezeObject(rq, val, true);
	return val;
}

void ProfileStart(const std::string& regionName)
{
	const char* name = "(ProfileStart)";

	typedef boost::flyweight<
		std::string,
		boost::flyweights::no_tracking,
		boost::flyweights::no_locking
	> StringFlyweight;

	if (!regionName.empty())
		name = StringFlyweight(regionName).get().c_str();

	if (CProfileManager::IsInitialised() && Threading::IsMainThread())
		g_Profiler.StartScript(name);

	g_Profiler2.RecordRegionEnter(name);
}

void ProfileStop()
{
	if (CProfileManager::IsInitialised() && Threading::IsMainThread())
		g_Profiler.Stop();

	g_Profiler2.RecordRegionLeave();
}

void ProfileAttribute(const std::string& attr)
{
	const char* name = "(ProfileAttribute)";

	typedef boost::flyweight<
		std::string,
		boost::flyweights::no_tracking,
		boost::flyweights::no_locking
	> StringFlyweight;

	if (!attr.empty())
		name = StringFlyweight(attr).get().c_str();

	g_Profiler2.RecordAttribute("%s", name);
}

// Math override functions:

// boost::uniform_real is apparently buggy in Boost pre-1.47 - for integer generators
// it returns [min,max], not [min,max). The bug was fixed in 1.47.
// We need consistent behaviour, so manually implement the correct version:
static double generate_uniform_real(boost::rand48& rng, double min, double max)
{
	while (true)
	{
		double n = (double)(rng() - rng.min());
		double d = (double)(rng.max() - rng.min()) + 1.0;
		ENSURE(d > 0 && n >= 0 && n <= d);
		double r = n / d * (max - min) + min;
		if (r < max)
			return r;
	}
}

} // anonymous namespace

bool ScriptInterface::MathRandom(double& nbr) const
{
	if (m->m_rng == nullptr)
		return false;
	nbr = generate_uniform_real(*(m->m_rng), 0.0, 1.0);
	return true;
}

bool ScriptInterface::Math_random(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	double r;
	if (!ScriptInterface::CmptPrivate::GetScriptInterface(cx).MathRandom(r))
		return false;

	args.rval().setNumber(r);
	return true;
}

ScriptInterface_impl::ScriptInterface_impl(const char* nativeScopeName, const std::shared_ptr<ScriptContext>& context) :
	m_context(context), m_cx(context->GetGeneralJSContext()), m_glob(context->GetGeneralJSContext()), m_nativeScope(context->GetGeneralJSContext())
{
	JS::RealmCreationOptions creationOpt;
	// Keep JIT code during non-shrinking GCs. This brings a quite big performance improvement.
	creationOpt.setPreserveJitCode(true);
	// Enable uneval
	creationOpt.setToSourceEnabled(true);
	JS::RealmOptions opt(creationOpt, JS::RealmBehaviors{});

	m_glob = JS_NewGlobalObject(m_cx, &global_class, nullptr, JS::OnNewGlobalHookOption::FireOnNewGlobalHook, opt);

	JSAutoRealm autoRealm(m_cx, m_glob);

	ENSURE(JS::InitRealmStandardClasses(m_cx));

	JS_DefineProperty(m_cx, m_glob, "global", m_glob, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	// These first 4 actually use CallArgs & thus don't use ScriptFunction
	JS_DefineFunction(m_cx, m_glob, "print", ::print,        0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "log",   ::logmsg,       1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "warn",  ::warn,         1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "error", ::error,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	ScriptFunction::Register<deepcopy>(m_cx, m_glob, "clone");
	ScriptFunction::Register<deepfreeze>(m_cx, m_glob, "deepfreeze");

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, nullptr, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	ScriptFunction::Register<&ProfileStart>(m_cx, m_nativeScope, "ProfileStart");
	ScriptFunction::Register<&ProfileStop>(m_cx, m_nativeScope, "ProfileStop");
	ScriptFunction::Register<&ProfileAttribute>(m_cx, m_nativeScope, "ProfileAttribute");

	m_context->RegisterRealm(JS::GetObjectRealmOrNull(m_glob));
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	m_context->UnRegisterRealm(JS::GetObjectRealmOrNull(m_glob));
}

ScriptInterface::ScriptInterface(const char* nativeScopeName, const char* debugName, const std::shared_ptr<ScriptContext>& context) :
	m(std::make_unique<ScriptInterface_impl>(nativeScopeName, context))
{
	// Profiler stats table isn't thread-safe, so only enable this on the main thread
	if (Threading::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Add(this, debugName);
	}

	ScriptRequest rq(this);
	m_CmptPrivate.pScriptInterface = this;
	JS::SetRealmPrivate(JS::GetObjectRealmOrNull(rq.glob), (void*)&m_CmptPrivate);
}

ScriptInterface::~ScriptInterface()
{
	if (Threading::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Remove(this);
	}
}

const ScriptInterface& ScriptInterface::CmptPrivate::GetScriptInterface(JSContext *cx)
{
	CmptPrivate* pCmptPrivate = (CmptPrivate*)JS::GetRealmPrivate(JS::GetCurrentRealmOrNull(cx));
	ENSURE(pCmptPrivate);
	return *pCmptPrivate->pScriptInterface;
}

void* ScriptInterface::CmptPrivate::GetCBData(JSContext *cx)
{
	CmptPrivate* pCmptPrivate = (CmptPrivate*)JS::GetRealmPrivate(JS::GetCurrentRealmOrNull(cx));
	return pCmptPrivate ? pCmptPrivate->pCBData : nullptr;
}

void ScriptInterface::SetCallbackData(void* pCBData)
{
	m_CmptPrivate.pCBData = pCBData;
}

template <>
void* ScriptInterface::ObjectFromCBData<void>(const ScriptRequest& rq)
{
	return ScriptInterface::CmptPrivate::GetCBData(rq.cx);
}

bool ScriptInterface::LoadGlobalScripts()
{
	// Ignore this failure in tests
	if (!g_VFS)
		return false;

	// Load and execute *.js in the global scripts directory
	VfsPaths pathnames;
	vfs::GetPathnames(g_VFS, L"globalscripts/", L"*.js", pathnames);
	for (const VfsPath& path : pathnames)
		if (!LoadGlobalScriptFile(path))
		{
			LOGERROR("LoadGlobalScripts: Failed to load script %s", path.string8());
			return false;
		}

	return true;
}

bool ScriptInterface::ReplaceNondeterministicRNG(boost::rand48& rng)
{
	ScriptRequest rq(this);
	JS::RootedValue math(rq.cx);
	JS::RootedObject global(rq.cx, rq.glob);
	if (JS_GetProperty(rq.cx, global, "Math", &math) && math.isObject())
	{
		JS::RootedObject mathObj(rq.cx, &math.toObject());
		JS::RootedFunction random(rq.cx, JS_DefineFunction(rq.cx, mathObj, "random", Math_random, 0,
			JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));
		if (random)
		{
			m->m_rng = &rng;
			return true;
		}
	}

	ScriptException::CatchPending(rq);
	LOGERROR("ReplaceNondeterministicRNG: failed to replace Math.random");
	return false;
}

JSContext* ScriptInterface::GetGeneralJSContext() const
{
	return m->m_context->GetGeneralJSContext();
}

std::shared_ptr<ScriptContext> ScriptInterface::GetContext() const
{
	return m->m_context;
}

void ScriptInterface::CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out) const
{
	ScriptRequest rq(this);
	if (!ctor.isObject())
	{
		LOGERROR("CallConstructor: ctor is not an object");
		out.setNull();
		return;
	}

	JS::RootedObject ctorObj(rq.cx, &ctor.toObject());
	out.setObjectOrNull(JS_New(rq.cx, ctorObj, argv));
}

void ScriptInterface::DefineCustomObjectType(JSClass *clasp, JSNative constructor, uint minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
	ScriptRequest rq(this);
	std::string typeName = clasp->name;

	if (m_CustomObjectTypes.find(typeName) != m_CustomObjectTypes.end())
	{
		// This type already exists
		throw PSERROR_Scripting_DefineType_AlreadyExists();
	}

	JS::RootedObject global(rq.cx, rq.glob);
	JS::RootedObject obj(rq.cx, JS_InitClass(rq.cx, global, nullptr,
	                                           clasp,
	                                           constructor, minArgs,     // Constructor, min args
	                                           ps, fs,                   // Properties, methods
	                                           static_ps, static_fs));   // Constructor properties, methods

	if (obj == nullptr)
	{
		ScriptException::CatchPending(rq);
		throw PSERROR_Scripting_DefineType_CreationFailed();
	}

	CustomType& type = m_CustomObjectTypes[typeName];

	type.m_Prototype.init(rq.cx, obj);
	type.m_Class = clasp;
	type.m_Constructor = constructor;
}

JSObject* ScriptInterface::CreateCustomObject(const std::string& typeName) const
{
	std::map<std::string, CustomType>::const_iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
		throw PSERROR_Scripting_TypeDoesNotExist();

	ScriptRequest rq(this);
	JS::RootedObject prototype(rq.cx, it->second.m_Prototype.get());
	return JS_NewObjectWithGivenProto(rq.cx, it->second.m_Class, prototype);
}

bool ScriptInterface::SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate)
{
	ScriptRequest rq(this);
	JS::RootedObject global(rq.cx, rq.glob);

	bool found;
	if (!JS_HasProperty(rq.cx, global, name, &found))
		return false;
	if (found)
	{
		JS::Rooted<JS::PropertyDescriptor> desc(rq.cx);
		if (!JS_GetOwnPropertyDescriptor(rq.cx, global, name, &desc))
			return false;

		if (!desc.writable())
		{
			if (!replace)
			{
				ScriptException::Raise(rq, "SetGlobal \"%s\" called multiple times", name);
				return false;
			}

			// This is not supposed to happen, unless the user has called SetProperty with constant = true on the global object
			// instead of using SetGlobal.
			if (!desc.configurable())
			{
				ScriptException::Raise(rq, "The global \"%s\" is permanent and cannot be hotloaded", name);
				return false;
			}

			LOGMESSAGE("Hotloading new value for global \"%s\".", name);
			ENSURE(JS_DeleteProperty(rq.cx, global, name));
		}
	}

	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	return JS_DefineProperty(rq.cx, global, name, value, attrs);
}

bool ScriptInterface::GetGlobalProperty(const ScriptRequest& rq, const std::string& name, JS::MutableHandleValue out)
{
	// Try to get the object as a property of the global object.
	JS::RootedObject global(rq.cx, rq.glob);
	if (!JS_GetProperty(rq.cx, global, name.c_str(), out))
	{
		out.set(JS::NullHandleValue);
		return false;
	}

	if (!out.isNullOrUndefined())
		return true;

	// Some objects, such as const definitions, or Class definitions, are hidden inside closures.
	// We must fetch those from the correct lexical scope.
	//JS::RootedValue glob(cx);
	JS::RootedObject lexical_environment(rq.cx, JS_GlobalLexicalEnvironment(rq.glob));
	if (!JS_GetProperty(rq.cx, lexical_environment, name.c_str(), out))
	{
		out.set(JS::NullHandleValue);
		return false;
	}

	if (!out.isNullOrUndefined())
		return true;

	out.set(JS::NullHandleValue);
	return false;
}

bool ScriptInterface::SetPrototype(JS::HandleValue objVal, JS::HandleValue protoVal)
{
	ScriptRequest rq(this);
	if (!objVal.isObject() || !protoVal.isObject())
		return false;
	JS::RootedObject obj(rq.cx, &objVal.toObject());
	JS::RootedObject proto(rq.cx, &protoVal.toObject());
	return JS_SetPrototype(rq.cx, obj, proto);
}

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::string& code) const
{
	ScriptRequest rq(this);
	JS::RootedObject global(rq.cx, rq.glob);

	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::CompileOptions options(rq.cx);
	// Set the line to 0 because CompileFunction silently adds a `(function() {` as the first line,
	// and errors get misreported.
	// TODO: it would probably be better to not implicitly introduce JS scopes.
	options.setFileAndLine(filenameStr.c_str(), 0);
	options.setIsRunOnce(false);

	JS::SourceText<mozilla::Utf8Unit> src;
	ENSURE(src.init(rq.cx, code.c_str(), code.length(), JS::SourceOwnership::Borrowed));
	JS::RootedObjectVector emptyScopeChain(rq.cx);
	JS::RootedFunction func(rq.cx, JS::CompileFunction(rq.cx, emptyScopeChain, options, NULL, 0, NULL, src));
	if (func == nullptr)
	{
		ScriptException::CatchPending(rq);
		return false;
	}

	JS::RootedValue rval(rq.cx);
	if (JS_CallFunction(rq.cx, nullptr, func, JS::HandleValueArray::empty(), &rval))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::string& code) const
{
	ScriptRequest rq(this);
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::RootedValue rval(rq.cx);
	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine(filenameStr.c_str(), 1);

	JS::SourceText<mozilla::Utf8Unit> src;
	ENSURE(src.init(rq.cx, code.c_str(), code.length(), JS::SourceOwnership::Borrowed));
	if (JS::Evaluate(rq.cx, opts, src, &rval))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path) const
{
	ScriptRequest rq(this);
	if (!VfsFileExists(path))
	{
		LOGERROR("File '%s' does not exist", path.string8());
		return false;
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, path);

	if (ret != PSRETURN_OK)
	{
		LOGERROR("Failed to load file '%s': %s", path.string8(), GetErrorString(ret));
		return false;
	}

	CStr code = file.DecodeUTF8(); // assume it's UTF-8

	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = path.string8();

	JS::RootedValue rval(rq.cx);
	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	JS::SourceText<mozilla::Utf8Unit> src;
	ENSURE(src.init(rq.cx, code.c_str(), code.length(), JS::SourceOwnership::Borrowed));
	if (JS::Evaluate(rq.cx, opts, src, &rval))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}

bool ScriptInterface::Eval(const char* code) const
{
	ScriptRequest rq(this);
	JS::RootedValue rval(rq.cx);

	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine("(eval)", 1);
	JS::SourceText<mozilla::Utf8Unit> src;
	ENSURE(src.init(rq.cx, code, strlen(code), JS::SourceOwnership::Borrowed));
	if (JS::Evaluate(rq.cx, opts, src, &rval))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}

bool ScriptInterface::Eval(const char* code, JS::MutableHandleValue rval) const
{
	ScriptRequest rq(this);

	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine("(eval)", 1);
	JS::SourceText<mozilla::Utf8Unit> src;
	ENSURE(src.init(rq.cx, code, strlen(code), JS::SourceOwnership::Borrowed));
	if (JS::Evaluate(rq.cx, opts, src, rval))
		return true;

	ScriptException::CatchPending(rq);
	return false;
}
