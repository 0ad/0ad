/* Copyright (C) 2019 Wildfire Games.
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

#include "ScriptInterface.h"
#include "ScriptRuntime.h"
#include "ScriptStats.h"

#include "lib/debug.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

#include <cassert>
#include <map>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/key_value.hpp>
#include <boost/flyweight/no_locking.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include "valgrind.h"

#include "scriptinterface/ScriptExtraHeaders.h"

/**
 * @file
 * Abstractions of various SpiderMonkey features.
 * Engine code should be using functions of these interfaces rather than
 * directly accessing the underlying JS api.
 */


struct ScriptInterface_impl
{
	ScriptInterface_impl(const char* nativeScopeName, const shared_ptr<ScriptRuntime>& runtime);
	~ScriptInterface_impl();
	void Register(const char* name, JSNative fptr, uint nargs) const;

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the runtime destructor.
	shared_ptr<ScriptRuntime> m_runtime;

	JSContext* m_cx;
	JS::PersistentRootedObject m_glob; // global scope object
	JSCompartment* m_comp;
	boost::rand48* m_rng;
	JS::PersistentRootedObject m_nativeScope; // native function scope object
};

namespace
{

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	nullptr, nullptr,
	nullptr, nullptr,
	nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	JS_GlobalObjectTraceHook
};

void ErrorReporter(JSContext* cx, const char* message, JSErrorReport* report)
{
	std::stringstream msg;
	bool isWarning = JSREPORT_IS_WARNING(report->flags);
	msg << (isWarning ? "JavaScript warning: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}

	msg << message;

	// If there is an exception, then print its stack trace
	JS::RootedValue excn(cx);
	if (JS_GetPendingException(cx, &excn) && excn.isObject())
	{
		JS::RootedObject excnObj(cx, &excn.toObject());
		// TODO: this violates the docs ("The error reporter callback must not reenter the JSAPI.")

		// Hide the exception from EvaluateScript
		JSExceptionState* excnState = JS_SaveExceptionState(cx);
		JS_ClearPendingException(cx);

		JS::RootedValue rval(cx);
		const char dumpStack[] = "this.stack.trimRight().replace(/^/mg, '  ')"; // indent each line
		JS::CompileOptions opts(cx);
		if (JS::Evaluate(cx, excnObj, opts.setFileAndLine("(eval)", 1), dumpStack, ARRAY_SIZE(dumpStack)-1, &rval))
		{
			std::string stackTrace;
			if (ScriptInterface::FromJSVal(cx, rval, stackTrace))
				msg << "\n" << stackTrace;

			JS_RestoreExceptionState(cx, excnState);
		}
		else
		{
			// Error got replaced by new exception from EvaluateScript
			JS_DropExceptionState(cx, excnState);
		}
	}

	if (isWarning)
		LOGWARNING("%s", msg.str().c_str());
	else
		LOGERROR("%s", msg.str().c_str());

	// When running under Valgrind, print more information in the error message
//	VALGRIND_PRINTF_BACKTRACE("->");
}

// Functions in the global namespace:

bool print(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	for (uint i = 0; i < args.length(); ++i)
	{
		std::wstring str;
		if (!ScriptInterface::FromJSVal(cx, args[i], str))
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

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, args[0], str))
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

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, args[0], str))
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

	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, args[0], str))
		return false;
	LOGERROR("%s", utf8_from_wstring(str));
	args.rval().setUndefined();
	return true;
}

bool deepcopy(JSContext* cx, uint argc, JS::Value* vp)
{
	JSAutoRequest rq(cx);

	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() < 1)
	{
		args.rval().setUndefined();
		return true;
	}

	JS::RootedValue ret(cx);
	if (!JS_StructuredClone(cx, args[0], &ret, NULL, NULL))
		return false;

	args.rval().set(ret);
	return true;
}

bool deepfreeze(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (args.length() != 1 || !args.get(0).isObject())
	{
		JS_ReportError(cx, "deepfreeze requires exactly one object as an argument.");
		return false;
	}

	ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface->FreezeObject(args.get(0), true);
	args.rval().set(args.get(0));
	return true;
}

bool ProfileStart(JSContext* cx, uint argc, JS::Value* vp)
{
	const char* name = "(ProfileStart)";

	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() >= 1)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(cx, args[0], str))
			return false;

		typedef boost::flyweight<
			std::string,
			boost::flyweights::no_tracking,
			boost::flyweights::no_locking
		> StringFlyweight;

		name = StringFlyweight(str).get().c_str();
	}

	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
		g_Profiler.StartScript(name);

	g_Profiler2.RecordRegionEnter(name);

	args.rval().setUndefined();
	return true;
}

bool ProfileStop(JSContext* UNUSED(cx), uint UNUSED(argc), JS::Value* vp)
{
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
		g_Profiler.Stop();

	g_Profiler2.RecordRegionLeave();

	rec.rval().setUndefined();
	return true;
}

bool ProfileAttribute(JSContext* cx, uint argc, JS::Value* vp)
{
	const char* name = "(ProfileAttribute)";

	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() >= 1)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(cx, args[0], str))
			return false;

		typedef boost::flyweight<
			std::string,
			boost::flyweights::no_tracking,
			boost::flyweights::no_locking
		> StringFlyweight;

		name = StringFlyweight(str).get().c_str();
	}

	g_Profiler2.RecordAttribute("%s", name);

	args.rval().setUndefined();
	return true;
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

bool Math_random(JSContext* cx, uint UNUSED(argc), JS::Value* vp)
{
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	double r;
	if (!ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface->MathRandom(r))
		return false;

	rec.rval().setNumber(r);
	return true;
}

} // anonymous namespace

bool ScriptInterface::MathRandom(double& nbr)
{
	if (m->m_rng == NULL)
		return false;
	nbr = generate_uniform_real(*(m->m_rng), 0.0, 1.0);
	return true;
}

ScriptInterface_impl::ScriptInterface_impl(const char* nativeScopeName, const shared_ptr<ScriptRuntime>& runtime) :
	m_runtime(runtime), m_glob(runtime->m_rt), m_nativeScope(runtime->m_rt)
{
	bool ok;

	m_cx = JS_NewContext(m_runtime->m_rt, STACK_CHUNK_SIZE);
	ENSURE(m_cx);

	JS_SetOffthreadIonCompilationEnabled(m_runtime->m_rt, true);

	// For GC debugging:
	// JS_SetGCZeal(m_cx, 2, JS_DEFAULT_ZEAL_FREQ);

	JS_SetContextPrivate(m_cx, NULL);

	JS_SetErrorReporter(m_runtime->m_rt, ErrorReporter);

	JS_SetGlobalJitCompilerOption(m_runtime->m_rt, JSJITCOMPILER_ION_ENABLE, 1);
	JS_SetGlobalJitCompilerOption(m_runtime->m_rt, JSJITCOMPILER_BASELINE_ENABLE, 1);

	JS::RuntimeOptionsRef(m_cx)
		.setExtraWarnings(true)
		.setWerror(false)
		.setVarObjFix(true)
		.setStrictMode(true);

	JS::CompartmentOptions opt;
	opt.setVersion(JSVERSION_LATEST);
	// Keep JIT code during non-shrinking GCs. This brings a quite big performance improvement.
	opt.setPreserveJitCode(true);

	JSAutoRequest rq(m_cx);
	JS::RootedObject globalRootedVal(m_cx, JS_NewGlobalObject(m_cx, &global_class, NULL, JS::OnNewGlobalHookOption::FireOnNewGlobalHook, opt));
	m_comp = JS_EnterCompartment(m_cx, globalRootedVal);
	ok = JS_InitStandardClasses(m_cx, globalRootedVal);
	ENSURE(ok);
	m_glob = globalRootedVal.get();

	JS_DefineProperty(m_cx, m_glob, "global", globalRootedVal, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, nullptr, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, globalRootedVal, "print", ::print,        0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "log",   ::logmsg,       1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "warn",  ::warn,         1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "error", ::error,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "clone", ::deepcopy,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "deepfreeze", ::deepfreeze, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	Register("ProfileStart", ::ProfileStart, 1);
	Register("ProfileStop", ::ProfileStop, 0);
	Register("ProfileAttribute", ::ProfileAttribute, 1);

	runtime->RegisterContext(m_cx);
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	m_runtime->UnRegisterContext(m_cx);
	{
		JSAutoRequest rq(m_cx);
		JS_LeaveCompartment(m_cx, m_comp);
	}
	JS_DestroyContext(m_cx);
}

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uint nargs) const
{
	JSAutoRequest rq(m_cx);
	JS::RootedObject nativeScope(m_cx, m_nativeScope);
	JS::RootedFunction func(m_cx, JS_DefineFunction(m_cx, nativeScope, name, fptr, nargs, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));
}

ScriptInterface::ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptRuntime>& runtime) :
	m(new ScriptInterface_impl(nativeScopeName, runtime))
{
	// Profiler stats table isn't thread-safe, so only enable this on the main thread
	if (ThreadUtil::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Add(this, debugName);
	}

	m_CxPrivate.pScriptInterface = this;
	JS_SetContextPrivate(m->m_cx, (void*)&m_CxPrivate);
}

ScriptInterface::~ScriptInterface()
{
	if (ThreadUtil::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Remove(this);
	}
}

void ScriptInterface::SetCallbackData(void* pCBData)
{
	m_CxPrivate.pCBData = pCBData;
}

ScriptInterface::CxPrivate* ScriptInterface::GetScriptInterfaceAndCBData(JSContext* cx)
{
	CxPrivate* pCxPrivate = (CxPrivate*)JS_GetContextPrivate(cx);
	return pCxPrivate;
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
	JSAutoRequest rq(m->m_cx);
	JS::RootedValue math(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	if (JS_GetProperty(m->m_cx, global, "Math", &math) && math.isObject())
	{
		JS::RootedObject mathObj(m->m_cx, &math.toObject());
		JS::RootedFunction random(m->m_cx, JS_DefineFunction(m->m_cx, mathObj, "random", Math_random, 0,
			JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));
		if (random)
		{
			m->m_rng = &rng;
			return true;
		}
	}

	LOGERROR("ReplaceNondeterministicRNG: failed to replace Math.random");
	return false;
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs) const
{
	m->Register(name, fptr, (uint)nargs);
}

JSContext* ScriptInterface::GetContext() const
{
	return m->m_cx;
}

JSRuntime* ScriptInterface::GetJSRuntime() const
{
	return m->m_runtime->m_rt;
}

shared_ptr<ScriptRuntime> ScriptInterface::GetRuntime() const
{
	return m->m_runtime;
}

void ScriptInterface::CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out) const
{
	JSAutoRequest rq(m->m_cx);
	if (!ctor.isObject())
	{
		LOGERROR("CallConstructor: ctor is not an object");
		out.setNull();
		return;
	}

	JS::RootedObject ctorObj(m->m_cx, &ctor.toObject());
	out.setObjectOrNull(JS_New(m->m_cx, ctorObj, argv));
}

void ScriptInterface::DefineCustomObjectType(JSClass *clasp, JSNative constructor, uint minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
	JSAutoRequest rq(m->m_cx);
	std::string typeName = clasp->name;

	if (m_CustomObjectTypes.find(typeName) != m_CustomObjectTypes.end())
	{
		// This type already exists
		throw PSERROR_Scripting_DefineType_AlreadyExists();
	}

	JS::RootedObject global(m->m_cx, m->m_glob);
	JS::RootedObject obj(m->m_cx, JS_InitClass(m->m_cx, global, JS::NullPtr(),
									clasp,
									constructor, minArgs,				// Constructor, min args
									ps, fs,								// Properties, methods
									static_ps, static_fs));				// Constructor properties, methods

	if (obj == NULL)
		throw PSERROR_Scripting_DefineType_CreationFailed();

	CustomType& type = m_CustomObjectTypes[typeName];

	type.m_Prototype.init(m->m_cx, obj);
	type.m_Class = clasp;
	type.m_Constructor = constructor;
}

JSObject* ScriptInterface::CreateCustomObject(const std::string& typeName) const
{
	std::map<std::string, CustomType>::const_iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
		throw PSERROR_Scripting_TypeDoesNotExist();

	JS::RootedObject prototype(m->m_cx, it->second.m_Prototype.get());
	return JS_NewObjectWithGivenProto(m->m_cx, it->second.m_Class, prototype);
}

bool ScriptInterface::CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject obj(m->m_cx);
	if (!JS_ValueToObject(m->m_cx, val, &obj) || !obj)
		return false;

	// Check that the named function actually exists, to avoid ugly JS error reports
	// when calling an undefined value
	bool found;
	if (!JS_HasProperty(m->m_cx, obj, name, &found) || !found)
		return false;

	bool ok = JS_CallFunctionName(m->m_cx, obj, name, argv, ret);

	return ok;
}

JS::Value ScriptInterface::GetGlobalObject() const
{
	JSAutoRequest rq(m->m_cx);
	return JS::ObjectValue(*JS::CurrentGlobalOrNull(m->m_cx));
}

bool ScriptInterface::SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);

	bool found;
	if (!JS_HasProperty(m->m_cx, global, name, &found))
		return false;
	if (found)
	{
		JS::Rooted<JSPropertyDescriptor> desc(m->m_cx);
		if (!JS_GetOwnPropertyDescriptor(m->m_cx, global, name, &desc))
			return false;

		if (desc.isReadonly())
		{
			if (!replace)
			{
				JS_ReportError(m->m_cx, "SetGlobal \"%s\" called multiple times", name);
				return false;
			}

			// This is not supposed to happen, unless the user has called SetProperty with constant = true on the global object
			// instead of using SetGlobal.
			if (desc.isPermanent())
			{
				JS_ReportError(m->m_cx, "The global \"%s\" is permanent and cannot be hotloaded", name);
				return false;
			}

			LOGMESSAGE("Hotloading new value for global \"%s\".", name);
			ENSURE(JS_DeleteProperty(m->m_cx, global, name));
		}
	}

	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	return JS_DefineProperty(m->m_cx, global, name, value, attrs);
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool constant, bool enumerate) const
{
	JSAutoRequest rq(m->m_cx);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	if (!JS_DefineProperty(m->m_cx, object, name, value, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool constant, bool enumerate) const
{
	JSAutoRequest rq(m->m_cx);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	utf16string name16(name, name + wcslen(name));
	if (!JS_DefineUCProperty(m->m_cx, object, reinterpret_cast<const char16_t*>(name16.c_str()), name16.length(), value, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool constant, bool enumerate) const
{
	JSAutoRequest rq(m->m_cx);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	JS::RootedId id(m->m_cx, INT_TO_JSID(name));
	if (!JS_DefinePropertyById(m->m_cx, object, id, value, attrs))
		return false;
	return true;
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const
{
	return GetProperty_(obj, name, out);
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out) const
{
	JSContext* cx = GetContext();
	JSAutoRequest rq(cx);
	JS::RootedValue val(cx);
	if (!GetProperty_(obj, name, &val))
		return false;
	if (!val.isObject())
	{
		LOGERROR("GetProperty failed: trying to get an object, but the property is not an object!");
		return false;
	}

	out.set(&val.toObject());
	return true;
}

bool ScriptInterface::GetPropertyInt(JS::HandleValue obj, int name, JS::MutableHandleValue out) const
{
	return GetPropertyInt_(obj, name, out);
}

bool ScriptInterface::GetProperty_(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const
{
	JSAutoRequest rq(m->m_cx);
	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	if (!JS_GetProperty(m->m_cx, object, name, out))
		return false;
	return true;
}

bool ScriptInterface::GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue out) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedId nameId(m->m_cx, INT_TO_JSID(name));
	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	if (!JS_GetPropertyById(m->m_cx, object, nameId, out))
		return false;
	return true;
}

bool ScriptInterface::HasProperty(JS::HandleValue obj, const char* name) const
{
	// TODO: proper errorhandling
	JSAutoRequest rq(m->m_cx);
	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());

	bool found;
	if (!JS_HasProperty(m->m_cx, object, name, &found))
		return false;
	return found;
}

bool ScriptInterface::EnumeratePropertyNamesWithPrefix(JS::HandleValue objVal, const char* prefix, std::vector<std::string>& out) const
{
	JSAutoRequest rq(m->m_cx);

	if (!objVal.isObjectOrNull())
	{
		LOGERROR("EnumeratePropertyNamesWithPrefix expected object type!");
		return false;
	}

	if (objVal.isNull())
		return true; // reached the end of the prototype chain

	JS::RootedObject obj(m->m_cx, &objVal.toObject());
	JS::AutoIdArray props(m->m_cx, JS_Enumerate(m->m_cx, obj));
	if (!props)
		return false;

	for (size_t i = 0; i < props.length(); ++i)
	{
		JS::RootedId id(m->m_cx, props[i]);
		JS::RootedValue val(m->m_cx);
		if (!JS_IdToValue(m->m_cx, id, &val))
			return false;

		if (!val.isString())
			continue; // ignore integer properties

		JS::RootedString name(m->m_cx, val.toString());
		size_t len = strlen(prefix)+1;
		std::vector<char> buf(len);
		size_t prefixLen = strlen(prefix) * sizeof(char);
		JS_EncodeStringToBuffer(m->m_cx, name, &buf[0], prefixLen);
		buf[len-1]= '\0';
		if (0 == strcmp(&buf[0], prefix))
		{
			if (JS_StringHasLatin1Chars(name))
			{
				size_t length;
				JS::AutoCheckCannotGC nogc;
				const JS::Latin1Char* chars = JS_GetLatin1StringCharsAndLength(m->m_cx, nogc, name, &length);
				if (chars)
					out.push_back(std::string(chars, chars+length));
			}
			else
			{
				size_t length;
				JS::AutoCheckCannotGC nogc;
				const char16_t* chars = JS_GetTwoByteStringCharsAndLength(m->m_cx, nogc, name, &length);
				if (chars)
					out.push_back(std::string(chars, chars+length));
			}
		}
	}

	// Recurse up the prototype chain
	JS::RootedObject prototype(m->m_cx);
	if (JS_GetPrototype(m->m_cx, obj, &prototype))
	{
		JS::RootedValue prototypeVal(m->m_cx, JS::ObjectOrNullValue(prototype));
		if (!EnumeratePropertyNamesWithPrefix(prototypeVal, prefix, out))
			return false;
	}

	return true;
}

bool ScriptInterface::SetPrototype(JS::HandleValue objVal, JS::HandleValue protoVal)
{
	JSAutoRequest rq(m->m_cx);
	if (!objVal.isObject() || !protoVal.isObject())
		return false;
	JS::RootedObject obj(m->m_cx, &objVal.toObject());
	JS::RootedObject proto(m->m_cx, &protoVal.toObject());
	return JS_SetPrototype(m->m_cx, obj, proto);
}

bool ScriptInterface::FreezeObject(JS::HandleValue objVal, bool deep) const
{
	JSAutoRequest rq(m->m_cx);
	if (!objVal.isObject())
		return false;

	JS::RootedObject obj(m->m_cx, &objVal.toObject());

	if (deep)
		return JS_DeepFreezeObject(m->m_cx, obj);
	else
		return JS_FreezeObject(m->m_cx, obj);
}

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::string& code) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code.begin(), code.end());
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::CompileOptions options(m->m_cx);
	options.setFileAndLine(filenameStr.c_str(), lineNo);
	options.setCompileAndGo(true);

	JS::RootedFunction func(m->m_cx);
	JS::AutoObjectVector emptyScopeChain(m->m_cx);
	if (!JS::CompileFunction(m->m_cx, emptyScopeChain, options, NULL, 0, NULL,
	                         reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &func))
		return false;

	JS::RootedValue rval(m->m_cx);
	return JS_CallFunction(m->m_cx, JS::NullPtr(), func, JS::HandleValueArray::empty(), &rval);
}

shared_ptr<ScriptRuntime> ScriptInterface::CreateRuntime(shared_ptr<ScriptRuntime> parentRuntime, int runtimeSize, int heapGrowthBytesGCTrigger)
{
	return shared_ptr<ScriptRuntime>(new ScriptRuntime(parentRuntime, runtimeSize, heapGrowthBytesGCTrigger));
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::wstring& code) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code.begin(), code.end());
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::RootedValue rval(m->m_cx);
	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	return JS::Evaluate(m->m_cx, global, opts,
			reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &rval);
}

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
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

	std::wstring code = wstring_from_utf8(file.DecodeUTF8()); // assume it's UTF-8

	utf16string codeUtf16(code.begin(), code.end());
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = path.string8();

	JS::RootedValue rval(m->m_cx);
	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	return JS::Evaluate(m->m_cx, global, opts,
			reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &rval);
}

bool ScriptInterface::Eval(const char* code) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedValue rval(m->m_cx);
	return Eval_(code, &rval);
}

bool ScriptInterface::Eval_(const char* code, JS::MutableHandleValue rval) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code, code+strlen(code));

	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(m->m_cx, global, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::Eval_(const wchar_t* code, JS::MutableHandleValue rval) const
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code, code+wcslen(code));

	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(m->m_cx, global, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out) const
{
	JSAutoRequest rq(m->m_cx);
	std::wstring attrsW = wstring_from_utf8(string_utf8);
 	utf16string string(attrsW.begin(), attrsW.end());
	if (JS_ParseJSON(m->m_cx, reinterpret_cast<const char16_t*>(string.c_str()), (u32)string.size(), out))
		return true;

	LOGERROR("JS_ParseJSON failed!");
	if (!JS_IsExceptionPending(m->m_cx))
		return false;

	JS::RootedValue exc(m->m_cx);
	if (!JS_GetPendingException(m->m_cx, &exc))
		return false;

	JS_ClearPendingException(m->m_cx);
	// We expect an object of type SyntaxError
	if (!exc.isObject())
		return false;

	JS::RootedValue rval(m->m_cx);
	JS::RootedObject excObj(m->m_cx, &exc.toObject());
	if (!JS_CallFunctionName(m->m_cx, excObj, "toString", JS::HandleValueArray::empty(), &rval))
		return false;

	std::wstring error;
	ScriptInterface::FromJSVal(m->m_cx, rval, error);
	LOGERROR("%s", utf8_from_wstring(error));
	return false;
}

void ScriptInterface::ReadJSONFile(const VfsPath& path, JS::MutableHandleValue out) const
{
	if (!VfsFileExists(path))
	{
		LOGERROR("File '%s' does not exist", path.string8());
		return;
	}

	CVFSFile file;

	PSRETURN ret = file.Load(g_VFS, path);

	if (ret != PSRETURN_OK)
	{
		LOGERROR("Failed to load file '%s': %s", path.string8(), GetErrorString(ret));
		return;
	}

	std::string content(file.DecodeUTF8()); // assume it's UTF-8

	if (!ParseJSON(content, out))
		LOGERROR("Failed to parse '%s'", path.string8());
}

struct Stringifier
{
	static bool callback(const char16_t* buf, u32 len, void* data)
	{
		utf16string str(buf, buf+len);
		std::wstring strw(str.begin(), str.end());

		Status err; // ignore Unicode errors
		static_cast<Stringifier*>(data)->stream << utf8_from_wstring(strw, &err);
		return true;
	}

	std::stringstream stream;
};

// TODO: It's not quite clear why JS_Stringify needs JS::MutableHandleValue. |obj| should not get modified.
// It probably has historical reasons and could be changed by SpiderMonkey in the future.
std::string ScriptInterface::StringifyJSON(JS::MutableHandleValue obj, bool indent) const
{
	JSAutoRequest rq(m->m_cx);
	Stringifier str;
	JS::RootedValue indentVal(m->m_cx, indent ? JS::Int32Value(2) : JS::UndefinedValue());
	if (!JS_Stringify(m->m_cx, obj, JS::NullPtr(), indentVal, &Stringifier::callback, &str))
	{
		JS_ClearPendingException(m->m_cx);
		LOGERROR("StringifyJSON failed");
		return std::string();
	}

	return str.stream.str();
}


std::string ScriptInterface::ToString(JS::MutableHandleValue obj, bool pretty) const
{
	JSAutoRequest rq(m->m_cx);

	if (obj.isUndefined())
		return "(void 0)";

	// Try to stringify as JSON if possible
	// (TODO: this is maybe a bad idea since it'll drop 'undefined' values silently)
	if (pretty)
	{
		Stringifier str;
		JS::RootedValue indentVal(m->m_cx, JS::Int32Value(2));

		// Temporary disable the error reporter, so we don't print complaints about cyclic values
		JSErrorReporter er = JS_SetErrorReporter(m->m_runtime->m_rt, NULL);

		bool ok = JS_Stringify(m->m_cx, obj, JS::NullPtr(), indentVal, &Stringifier::callback, &str);

		// Restore error reporter
		JS_SetErrorReporter(m->m_runtime->m_rt, er);

		if (ok)
			return str.stream.str();

		// Clear the exception set when Stringify failed
		JS_ClearPendingException(m->m_cx);
	}

	// Caller didn't want pretty output, or JSON conversion failed (e.g. due to cycles),
	// so fall back to obj.toSource()

	std::wstring source = L"(error)";
	CallFunction(obj, "toSource", source);
	return utf8_from_wstring(source);
}

void ScriptInterface::ReportError(const char* msg) const
{
	JSAutoRequest rq(m->m_cx);
	// JS_ReportError by itself doesn't seem to set a JS-style exception, and so
	// script callers will be unable to catch anything. So use JS_SetPendingException
	// to make sure there really is a script-level exception. But just set it to undefined
	// because there's not much value yet in throwing a real exception object.
	JS_SetPendingException(m->m_cx, JS::UndefinedHandleValue);
	// And report the actual error
	JS_ReportError(m->m_cx, "%s", msg);

	// TODO: Why doesn't JS_ReportPendingException(m->m_cx); work?
}

bool ScriptInterface::IsExceptionPending(JSContext* cx)
{
	JSAutoRequest rq(cx);
	return JS_IsExceptionPending(cx) ? true : false;
}

const JSClass* ScriptInterface::GetClass(JS::HandleObject obj)
{
	return JS_GetClass(obj);
}

void* ScriptInterface::GetPrivate(JS::HandleObject obj)
{
	// TODO: use JS_GetInstancePrivate
	return JS_GetPrivate(obj);
}

JS::Value ScriptInterface::CloneValueFromOtherContext(const ScriptInterface& otherContext, JS::HandleValue val) const
{
	PROFILE("CloneValueFromOtherContext");
	JSAutoRequest rq(m->m_cx);
	JS::RootedValue out(m->m_cx);
	shared_ptr<StructuredClone> structuredClone = otherContext.WriteStructuredClone(val);
	ReadStructuredClone(structuredClone, &out);
	return out.get();
}

ScriptInterface::StructuredClone::StructuredClone() :
	m_Data(NULL), m_Size(0)
{
}

ScriptInterface::StructuredClone::~StructuredClone()
{
	if (m_Data)
		JS_ClearStructuredClone(m_Data, m_Size, NULL, NULL);
}

shared_ptr<ScriptInterface::StructuredClone> ScriptInterface::WriteStructuredClone(JS::HandleValue v) const
{
	JSAutoRequest rq(m->m_cx);
	u64* data = NULL;
	size_t nbytes = 0;
	if (!JS_WriteStructuredClone(m->m_cx, v, &data, &nbytes, NULL, NULL, JS::UndefinedHandleValue))
	{
		debug_warn(L"Writing a structured clone with JS_WriteStructuredClone failed!");
		return shared_ptr<StructuredClone>();
	}

	shared_ptr<StructuredClone> ret(new StructuredClone);
	ret->m_Data = data;
	ret->m_Size = nbytes;
	return ret;
}

void ScriptInterface::ReadStructuredClone(const shared_ptr<ScriptInterface::StructuredClone>& ptr, JS::MutableHandleValue ret) const
{
	JSAutoRequest rq(m->m_cx);
	JS_ReadStructuredClone(m->m_cx, ptr->m_Data, ptr->m_Size, JS_STRUCTURED_CLONE_VERSION, ret, NULL, NULL);
}
