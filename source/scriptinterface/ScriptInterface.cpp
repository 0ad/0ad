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
	void Register(const char* name, JSNative fptr, uint nargs);

	// Take care to keep this declaration before heap rooted members. Destructors of heap rooted
	// members have to be called before the runtime destructor.
	shared_ptr<ScriptRuntime> m_runtime;

	JSContext* m_cx;
	JS::PersistentRootedObject m_glob; // global scope object
	JSCompartment* m_comp;
	boost::rand48* m_rng;
	JS::PersistentRootedObject m_nativeScope; // native function scope object

	// TODO: we need DefPersistentRooted to work around a problem with JS::PersistentRooted<T>
	// that is already solved in newer versions of SpiderMonkey (related to std::pair and
	// and the copy constructor of PersistentRooted<T> taking a non-const reference).
	// Switch this to PersistentRooted<T> when upgrading to a newer SpiderMonkey version than v31.
	typedef std::map<ScriptInterface::CACHED_VAL, DefPersistentRooted<JS::Value> > ScriptValCache;
	ScriptValCache m_ScriptValCache;
};

namespace
{

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_DeletePropertyStub, 
	JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
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

bool print(JSContext* cx, uint argc, jsval* vp)
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

bool logmsg(JSContext* cx, uint argc, jsval* vp)
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

bool warn(JSContext* cx, uint argc, jsval* vp)
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

bool error(JSContext* cx, uint argc, jsval* vp)
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

bool deepcopy(JSContext* cx, uint argc, jsval* vp)
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

bool ProfileStart(JSContext* cx, uint argc, jsval* vp)
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

bool ProfileStop(JSContext* UNUSED(cx), uint UNUSED(argc), jsval* vp)
{
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
		g_Profiler.Stop();

	g_Profiler2.RecordRegionLeave();

	rec.rval().setUndefined();
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

bool Math_random(JSContext* cx, uint UNUSED(argc), jsval* vp)
{
	JS::CallReceiver rec = JS::CallReceiverFromVp(vp);
	double r;
	if(!ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface->MathRandom(r))
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

	JS_SetParallelIonCompilationEnabled(m_runtime->m_rt, true);

	// For GC debugging:
	// JS_SetGCZeal(m_cx, 2, JS_DEFAULT_ZEAL_FREQ);

	JS_SetContextPrivate(m_cx, NULL);

	JS_SetErrorReporter(m_cx, ErrorReporter);

	JS_SetGlobalJitCompilerOption(m_runtime->m_rt, JSJITCOMPILER_ION_ENABLE, 1);
	JS_SetGlobalJitCompilerOption(m_runtime->m_rt, JSJITCOMPILER_BASELINE_ENABLE, 1);

	JS::ContextOptionsRef(m_cx).setExtraWarnings(1)
		.setWerror(0)
		.setVarObjFix(1)
		.setStrictMode(1);

	JS::CompartmentOptions opt;
	opt.setVersion(JSVERSION_LATEST);

	JSAutoRequest rq(m_cx);
	JS::RootedObject globalRootedVal(m_cx, JS_NewGlobalObject(m_cx, &global_class, NULL, JS::OnNewGlobalHookOption::FireOnNewGlobalHook, opt));
	m_comp = JS_EnterCompartment(m_cx, globalRootedVal);
	ok = JS_InitStandardClasses(m_cx, globalRootedVal);
	ENSURE(ok);
	m_glob = globalRootedVal.get();

	// Use the testing functions to globally enable gcPreserveCode. This brings quite a 
	// big performance improvement. In future SpiderMonkey versions, we should probably 
	// use the functions implemented here: https://bugzilla.mozilla.org/show_bug.cgi?id=1068697
	JS::RootedObject testingFunctionsObj(m_cx, js::GetTestingFunctions(m_cx));
	ENSURE(testingFunctionsObj);
	JS::RootedValue ret(m_cx);
	JS_CallFunctionName(m_cx, testingFunctionsObj, "gcPreserveCode", JS::HandleValueArray::empty(), &ret);

	JS_DefineProperty(m_cx, m_glob, "global", globalRootedVal, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, globalRootedVal, "print", ::print,        0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "log",   ::logmsg,       1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "warn",  ::warn,         1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "error", ::error,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, globalRootedVal, "deepcopy", ::deepcopy,  1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	Register("ProfileStart", ::ProfileStart, 1);
	Register("ProfileStop", ::ProfileStop, 0);

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

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uint nargs)
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

void ScriptInterface::ShutDown()
{
	JS_ShutDown();
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

JS::Value ScriptInterface::GetCachedValue(CACHED_VAL valueIdentifier)
{
	std::map<ScriptInterface::CACHED_VAL, DefPersistentRooted<JS::Value>>::iterator it = m->m_ScriptValCache.find(valueIdentifier);
	ENSURE(it != m->m_ScriptValCache.end());
	return it->second.get();
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

	JSAutoRequest rq(m->m_cx);
	JS::RootedValue proto(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	if (JS_GetProperty(m->m_cx, global, "Vector2Dprototype", &proto))
		m->m_ScriptValCache[CACHE_VECTOR2DPROTO] = DefPersistentRooted<JS::Value>(GetJSRuntime(), proto);
	if (JS_GetProperty(m->m_cx, global, "Vector3Dprototype", &proto))
		m->m_ScriptValCache[CACHE_VECTOR3DPROTO] = DefPersistentRooted<JS::Value>(GetJSRuntime(), proto);
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

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs)
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

void ScriptInterface::CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out)
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

	CustomType type;

	type.m_Prototype = DefPersistentRooted<JSObject*>(m->m_cx, obj);
	type.m_Class = clasp;
	type.m_Constructor = constructor;

	m_CustomObjectTypes[typeName] = std::move(type);
}

JSObject* ScriptInterface::CreateCustomObject(const std::string& typeName) const
{
	std::map<std::string, CustomType>::const_iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
		throw PSERROR_Scripting_TypeDoesNotExist();

	JS::RootedObject prototype(m->m_cx, it->second.m_Prototype.get());
	return JS_NewObject(m->m_cx, (*it).second.m_Class, prototype, JS::NullPtr());
}

bool ScriptInterface::CallFunctionVoid(JS::HandleValue val, const char* name)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedValue jsRet(m->m_cx);
	return CallFunction_(val, name, JS::HandleValueArray::empty(), &jsRet);
}

bool ScriptInterface::CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret)
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

jsval ScriptInterface::GetGlobalObject()
{
	JSAutoRequest rq(m->m_cx);
	return JS::ObjectValue(*JS::CurrentGlobalOrNull(m->m_cx));
}

JSClass* ScriptInterface::GetGlobalClass()
{
	return &global_class;
}

bool ScriptInterface::SetGlobal_(const char* name, JS::HandleValue value, bool replace)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	if (!replace)
	{
		bool found;
		if (!JS_HasProperty(m->m_cx, global, name, &found))
			return false;
		if (found)
		{
			JS_ReportError(m->m_cx, "SetGlobal \"%s\" called multiple times", name);
			return false;
		}
	}

	bool ok = JS_DefineProperty(m->m_cx, global, name, value, JSPROP_ENUMERATE | JSPROP_READONLY
 			| JSPROP_PERMANENT);
	return ok;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool constant, bool enumerate)
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

	if (! JS_DefineProperty(m->m_cx, object, name, value, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool constant, bool enumerate)
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
	if (!JS_DefineUCProperty(m->m_cx, object, reinterpret_cast<const char16_t*>(name16.c_str()), name16.length(), value, NULL, NULL, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool constant, bool enumerate)
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

	if (! JS_DefinePropertyById(m->m_cx, object, INT_TO_JSID(name), value, NULL, NULL, attrs))
		return false;
	return true;
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out)
{
	return GetProperty_(obj, name, out);
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out)
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

bool ScriptInterface::GetPropertyInt(JS::HandleValue obj, int name, JS::MutableHandleValue out)
{
	return GetPropertyInt_(obj, name, out);
}

bool ScriptInterface::GetProperty_(JS::HandleValue obj, const char* name, JS::MutableHandleValue out)
{
	JSAutoRequest rq(m->m_cx);
	if (!obj.isObject())
		return false;
	JS::RootedObject object(m->m_cx, &obj.toObject());
	
	if (!JS_GetProperty(m->m_cx, object, name, out))
		return false;
	return true;
}

bool ScriptInterface::GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue out)
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

bool ScriptInterface::HasProperty(JS::HandleValue obj, const char* name)
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

bool ScriptInterface::EnumeratePropertyNamesWithPrefix(JS::HandleValue objVal, const char* prefix, std::vector<std::string>& out)
{
	JSAutoRequest rq(m->m_cx);
	
	if (!objVal.isObjectOrNull())
	{
		LOGERROR("EnumeratePropertyNamesWithPrefix expected object type!");
		return false;
	}
		
	if(objVal.isNull())
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
		if(0 == strcmp(&buf[0], prefix))
		{
			size_t len;
			const char16_t* chars = JS_GetStringCharsAndLength(m->m_cx, name, &len);
			out.push_back(std::string(chars, chars+len));
		}
	}

	// Recurse up the prototype chain
	JS::RootedObject prototype(m->m_cx);
	if (JS_GetPrototype(m->m_cx, obj, &prototype))
	{
		JS::RootedValue prototypeVal(m->m_cx, JS::ObjectOrNullValue(prototype));
		if (! EnumeratePropertyNamesWithPrefix(prototypeVal, prefix, out))
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

bool ScriptInterface::FreezeObject(JS::HandleValue objVal, bool deep)
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

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::string& code)
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

	JS::RootedFunction func(m->m_cx,
	JS_CompileUCFunction(m->m_cx, global, NULL, 0, NULL,
			reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), options)
	);
	if (!func)
		return false;

	JS::RootedValue rval(m->m_cx);
	return JS_CallFunction(m->m_cx, JS::NullPtr(), func, JS::HandleValueArray::empty(), &rval);
}

shared_ptr<ScriptRuntime> ScriptInterface::CreateRuntime(shared_ptr<ScriptRuntime> parentRuntime, int runtimeSize, int heapGrowthBytesGCTrigger)
{
	return shared_ptr<ScriptRuntime>(new ScriptRuntime(parentRuntime, runtimeSize, heapGrowthBytesGCTrigger));
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::wstring& code)
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

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path)
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

bool ScriptInterface::Eval(const char* code)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedValue rval(m->m_cx);
	return Eval_(code, &rval);
}

bool ScriptInterface::Eval_(const char* code, JS::MutableHandleValue rval)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code, code+strlen(code));
	
	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(m->m_cx, global, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::Eval_(const wchar_t* code, JS::MutableHandleValue rval)
{
	JSAutoRequest rq(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	utf16string codeUtf16(code, code+wcslen(code));

	JS::CompileOptions opts(m->m_cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(m->m_cx, global, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out)
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

void ScriptInterface::ReadJSONFile(const VfsPath& path, JS::MutableHandleValue out)
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
std::string ScriptInterface::StringifyJSON(JS::MutableHandleValue obj, bool indent)
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


std::string ScriptInterface::ToString(JS::MutableHandleValue obj, bool pretty)
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
		JSErrorReporter er = JS_SetErrorReporter(m->m_cx, NULL);

		bool ok = JS_Stringify(m->m_cx, obj, JS::NullPtr(), indentVal, &Stringifier::callback, &str);

		// Restore error reporter
		JS_SetErrorReporter(m->m_cx, er);

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

void ScriptInterface::ReportError(const char* msg)
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

void ScriptInterface::MaybeGC()
{
	JS_MaybeGC(m->m_cx);
}

void ScriptInterface::ForceGC()
{
	PROFILE2("JS_GC");
	JS_GC(this->GetJSRuntime());
}

JS::Value ScriptInterface::CloneValueFromOtherContext(ScriptInterface& otherContext, JS::HandleValue val)
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

shared_ptr<ScriptInterface::StructuredClone> ScriptInterface::WriteStructuredClone(JS::HandleValue v)
{
	JSAutoRequest rq(m->m_cx);
	u64* data = NULL;
	size_t nbytes = 0;
	if (!JS_WriteStructuredClone(m->m_cx, v, &data, &nbytes, NULL, NULL, JS::UndefinedHandleValue))
	{
		debug_warn(L"Writing a structured clone with JS_WriteStructuredClone failed!");
		return shared_ptr<StructuredClone>();
	}

	shared_ptr<StructuredClone> ret (new StructuredClone);
	ret->m_Data = data;
	ret->m_Size = nbytes;
	return ret;
}

void ScriptInterface::ReadStructuredClone(const shared_ptr<ScriptInterface::StructuredClone>& ptr, JS::MutableHandleValue ret)
{
	JSAutoRequest rq(m->m_cx);
	JS_ReadStructuredClone(m->m_cx, ptr->m_Data, ptr->m_Size, JS_STRUCTURED_CLONE_VERSION, ret, NULL, NULL);
}
