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
#include "ScriptStats.h"
#include "ScriptExtraHeaders.h"

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


/**
 * @file
 * Abstractions of various SpiderMonkey features.
 * Engine code should be using functions of these interfaces rather than
 * directly accessing the underlying JS api.
 */

void checkJSError(JSContext* cx)
{
    if(JS_IsExceptionPending(cx)) 
    {
         JS::RootedValue exception(cx);
         if(JS_GetPendingException(cx,&exception) && exception.isObject()) 
         {
             JS::AutoSaveExceptionState savedExc(cx);
             JS::Rooted<JSObject*> exceptionObject(cx, &exception.toObject());
             JSErrorReport* what = JS_ErrorFromException(cx,exceptionObject);
             if(what) ErrorReporter(cx, what);
         }
     } 
}

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
	boost::rand48* m_rng;
	JS::PersistentRootedObject m_nativeScope; // native function scope object
};

namespace
{

    static JSClass global_class = {
	    "global", JSCLASS_GLOBAL_FLAGS,
        &JS::DefaultGlobalClassOps};

void ErrorReporter(JSContext* cx, JSErrorReport* report)
{

	std::stringstream msg;
	bool isWarning = JSREPORT_IS_WARNING(report->flags);
	msg << (isWarning ? "JavaScript warning: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}

	msg << report->message().c_str();

	// If there is an exception, then print its stack trace
	JS::RootedValue excn(cx);
	if (JS_GetPendingException(cx, &excn) && excn.isObject())
	{
		JS::RootedValue stackVal(cx);
		JS::RootedObject excnObj(cx, &excn.toObject());
		JS_GetProperty(cx, excnObj, "stack", &stackVal);

		std::string stackText;
		ScriptInterface::FromJSVal(cx, stackVal, stackText);

		std::istringstream stream(stackText);
		for (std::string line; std::getline(stream, line);)
			msg << "\n  " << line;
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
		printf("%s", utf8_from_wstring(str).c_str());
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
		JS_ReportErrorASCII(cx, "deepfreeze requires exactly one object as an argument.");
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

bool ProfileStop(JSContext* UNUSED(cx), uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (CProfileManager::IsInitialised() && ThreadUtil::IsMainThread())
		g_Profiler.Stop();

	g_Profiler2.RecordRegionLeave();

	args.rval().setUndefined();
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

bool Math_random(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	double r;
	if (!ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface->MathRandom(r))
		return false;

	args.rval().setNumber(r);
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
	m_runtime(runtime), m_glob(), m_nativeScope()
{
	bool ok;

    m_cx = runtime->m_ctx;

    ENSURE(m_cx);

	JS::RealmOptions opt;

	JS::RootedObject globalRootedVal(m_cx, JS_NewGlobalObject(m_cx, 
                                           &global_class, 
                                           nullptr, 
                                           JS::FireOnNewGlobalHook, 
                                           opt));
    	
    ENSURE(globalRootedVal);
    {
        m_glob.init(m_cx);
        m_glob = globalRootedVal;
        JSAutoRealm ar(m_cx, globalRootedVal);
        ok = JS::InitRealmStandardClasses(m_cx);
        ENSURE(ok);

        JS_DefineProperty(m_cx, globalRootedVal, "global", m_glob, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

        m_nativeScope.init(m_cx, JS_DefineObject(m_cx, m_glob, nativeScopeName, nullptr, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));

        JS_DefineFunction(m_cx, globalRootedVal, "print", ::print,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        JS_DefineFunction(m_cx, globalRootedVal, "log",   ::logmsg,       1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        JS_DefineFunction(m_cx, globalRootedVal, "warn",  ::warn,         1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        JS_DefineFunction(m_cx, globalRootedVal, "error", ::error,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        JS_DefineFunction(m_cx, globalRootedVal, "clone", ::deepcopy,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
        JS_DefineFunction(m_cx, globalRootedVal, "deepfreeze", ::deepfreeze, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
    }
	Register("ProfileStart", ::ProfileStart, 1);
	Register("ProfileStop", ::ProfileStop, 0);
	Register("ProfileAttribute", ::ProfileAttribute, 1);

}

ScriptInterface_impl::~ScriptInterface_impl() {}

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uint nargs) const
{
    JSAutoRealm ar(m_cx, m_glob);
	JS::RootedObject nativeScope(m_cx, m_nativeScope);
	JS::RootedFunction func(m_cx, JS_DefineFunction(m_cx, nativeScope, name, fptr, nargs, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));

    checkJSError(m_cx);
}

ScriptInterface::ScriptInterface(const char* nativeScopeName, const char* debugName, const shared_ptr<ScriptRuntime>& runtime) : m(new ScriptInterface_impl(nativeScopeName, runtime))
{    
	// Profiler stats table isn't thread-safe, so only enable this on the main thread
	if (ThreadUtil::IsMainThread())
	{
		if (g_ScriptStatsTable)
			g_ScriptStatsTable->Add(this, debugName);
	}

	m_RealmPrivate.pScriptInterface = this;
    JSAutoRealm ar(m->m_cx, m->m_glob);
    JS::SetRealmPrivate(JS::GetCurrentRealmOrNull(m->m_cx), (void*)&m_RealmPrivate);
    checkJSError(m->m_cx);
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
	m_RealmPrivate.pCBData = pCBData;
}

ScriptInterface::RealmPrivate* ScriptInterface::GetScriptInterfaceAndCBData(JSContext* ctx)
{	
    RealmPrivate* pRealmPrivate = (RealmPrivate*)JS::GetRealmPrivate(JS::GetCurrentRealmOrNull(ctx));
    checkJSError(ctx);
	return pRealmPrivate;
}


bool ScriptInterface::LoadGlobalScripts()
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
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
    JSAutoRealm ar(m->m_cx, m->m_glob);
	JS::RootedValue math(m->m_cx);
	JS::RootedObject global(m->m_cx, m->m_glob);
	if (JS_GetProperty(m->m_cx, global, "Math", &math) && math.isObject())
	{
		JS::RootedObject mathObj(m->m_cx, &math.toObject());
		JS::RootedFunction random(m->m_cx, JS_DefineFunction(m->m_cx, mathObj, "random", Math_random, 0,
			JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT));
    
        checkJSError(m->m_cx);
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
    JSAutoRealm ar(m->m_cx, m->m_glob);
	if (!ctor.isObject())
	{
		LOGERROR("CallConstructor: ctor is not an object");
		out.setNull();
		return;
	}

	JS::RootedObject ctorObj(m->m_cx, &ctor.toObject());
	out.setObjectOrNull(JS_New(m->m_cx, ctorObj, argv));
    checkJSError(m->m_cx);
}

void ScriptInterface::DefineCustomObjectType(JSClass *clasp, JSNative constructor, uint minArgs, JSPropertySpec *ps, JSFunctionSpec *fs, JSPropertySpec *static_ps, JSFunctionSpec *static_fs)
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	std::string typeName = clasp->name;

	if (m_CustomObjectTypes.find(typeName) != m_CustomObjectTypes.end())
	{
		// This type already exists
		throw PSERROR_Scripting_DefineType_AlreadyExists();
	}

	JS::RootedObject global(m->m_cx, m->m_glob);
	JS::RootedObject obj(m->m_cx, JS_InitClass(m->m_cx, global, nullptr,
	                                           clasp,
	                                           constructor, minArgs,     // Constructor, min args
	                                           ps, fs,                   // Properties, methods
	                                           static_ps, static_fs));   // Constructor properties, methods

    checkJSError(m->m_cx);

	if (!obj)
		throw PSERROR_Scripting_DefineType_CreationFailed();

	CustomType& type = m_CustomObjectTypes[typeName];

	type.m_Prototype.init(m->m_cx, obj);
    checkJSError(m->m_cx);
	type.m_Class = clasp;
	type.m_Constructor = constructor;
}

JSObject* ScriptInterface::CreateCustomObject(const std::string& typeName) const
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	std::map<std::string, CustomType>::const_iterator it = m_CustomObjectTypes.find(typeName);

	if (it == m_CustomObjectTypes.end())
		throw PSERROR_Scripting_TypeDoesNotExist();

	JS::RootedObject prototype(m->m_cx, it->second.m_Prototype.get());

    JSObject* ret = JS_NewObjectWithGivenProto(m->m_cx, it->second.m_Class, prototype);
    checkJSError(m->m_cx);
	return ret;
}

bool ScriptInterface::CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret) const
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	JS::RootedObject obj(m->m_cx);
	if (!JS_ValueToObject(m->m_cx, val, &obj) || !obj)
    {
        checkJSError(m->m_cx);
		return false;
    }

	// Check that the named function actually exists, to avoid ugly JS error reports
	// when calling an undefined value
	bool found;
	if (!JS_HasProperty(m->m_cx, obj, name, &found) || !found)
    {
        checkJSError(m->m_cx);
		return false;
    }

    checkJSError(m->m_cx);
	bool ok = JS_CallFunctionName(m->m_cx, obj, name, argv, ret);
    checkJSError(m->m_cx);
	return ok;
}

bool ScriptInterface::CreateObject_(JSContext* cx, JS::MutableHandleObject object)
{
	object.set(JS_NewPlainObject(cx));
    checkJSError(cx);
	if (!object)
		throw PSERROR_Scripting_CreateObjectFailed();

	return true;
}

void ScriptInterface::CreateArray(JSContext* cx, JS::MutableHandleValue objectValue, size_t length)
{
	objectValue.setObjectOrNull(JS_NewArrayObject(cx, length));
    checkJSError(cx);
	if (!objectValue.isObject())
		throw PSERROR_Scripting_CreateObjectFailed();
}

JS::Value ScriptInterface::GetGlobalObject() const
{
    CX_IN_REALM(cx,this)
    JS::Value globalObject = JS::ObjectValue(*JS::CurrentGlobalOrNull(m->m_cx));
    checkJSError(m->m_cx);
	return globalObject;
}

bool ScriptInterface::SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate)
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	JS::RootedObject global(m->m_cx, m->m_glob);

	bool found;
	if (!JS_HasProperty(m->m_cx, global, name, &found))
    {              
        checkJSError(m->m_cx);
		return false;
    }

	if (found)
	{
		JS::Rooted<JS::PropertyDescriptor> desc(m->m_cx);
		if (!JS_GetOwnPropertyDescriptor(m->m_cx, global, name, &desc))
        {
			return false;
        }

		if (!desc.writable())
		{
			if (!replace)
			{
				JS_ReportErrorASCII(m->m_cx, "SetGlobal \"%s\" called multiple times", name);
                checkJSError(m->m_cx);
			}

			// This is not supposed to happen, unless the user has called SetProperty with constant = true on the global object
			// instead of using SetGlobal.
			if (!desc.configurable())
			{
				JS_ReportErrorASCII(m->m_cx, "The global \"%s\" is permanent and cannot be hotloaded", name);       
                checkJSError(m->m_cx);
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

    bool ret = JS_DefineProperty(m->m_cx, global, name, value, attrs);
    checkJSError(m->m_cx);
	return ret;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool constant, bool enumerate) const
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
    {
		return false;
    }

	JS::RootedObject object(m->m_cx, &obj.toObject());

	if (!JS_DefineProperty(m->m_cx, object, name, value, attrs))
    {
        checkJSError(m->m_cx);
		return false;
    }
	return true;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool constant, bool enumerate) const
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
    {
        checkJSError(m->m_cx);
		return false;
    }

	JS::RootedObject object(m->m_cx, &obj.toObject());

	utf16string name16(name, name + wcslen(name));
	if (!JS_DefineUCProperty(m->m_cx, object, reinterpret_cast<const char16_t*>(name16.c_str()), name16.length(), value, attrs))
    {
        checkJSError(m->m_cx);
		return false;
    }
	return true;
}

bool ScriptInterface::SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool constant, bool enumerate) const
{
    JSAutoRealm ar(m->m_cx, m->m_glob);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
    {
        checkJSError(m->m_cx);
		return false;
    }
	JS::RootedObject object(m->m_cx, &obj.toObject());

	JS::RootedId id(m->m_cx, INT_TO_JSID(name));
	if (!JS_DefinePropertyById(m->m_cx, object, id, value, attrs))
    {
        checkJSError(m->m_cx);
		return false;
    }
	return true;
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const
{
	return GetProperty_(obj, name, out);
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out) const
{
    CX_IN_REALM(cx,this)

	JS::RootedValue val(cx);
	if (!GetProperty_(obj, name, &val))
    {
        checkJSError(cx);
		return false;
    }

	if (!val.isObject())
	{
        checkJSError(cx);
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
    CX_IN_REALM(cx,this)

	if (!obj.isObject())
    {
        checkJSError(cx);
		return false;
    }
	
    JS::RootedObject object(cx, &obj.toObject());

	if (!JS_GetProperty(cx, object, name, out))
    {
        checkJSError(cx);
		return false;
    }
	return true;
}

bool ScriptInterface::GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue out) const
{
    CX_IN_REALM(cx,this)

	JS::RootedId nameId(cx, INT_TO_JSID(name));
	if (!obj.isObject())
    {
        checkJSError(cx);
		return false;
    }
	JS::RootedObject object(cx, &obj.toObject());

	if (!JS_GetPropertyById(cx, object, nameId, out))
    {
        checkJSError(cx);
		return false;
    }
	return true;
}

bool ScriptInterface::HasProperty(JS::HandleValue obj, const char* name) const
{
    CX_IN_REALM(cx,this)

	// TODO: proper errorhandling
	if (!obj.isObject())
    {
        checkJSError(cx);
		return false;
    }
	JS::RootedObject object(cx, &obj.toObject());

	bool found;
	if (!JS_HasProperty(cx, object, name, &found))
    {
        checkJSError(cx);
		return false;
    }
    
    checkJSError(cx);

	return found;
}

bool ScriptInterface::EnumeratePropertyNamesWithPrefix(JS::HandleValue objVal, const char* prefix, std::vector<std::string>& out) const
{

    CX_IN_REALM(cx,this)

	if (!objVal.isObjectOrNull())
	{
        checkJSError(cx);
		LOGERROR("EnumeratePropertyNamesWithPrefix expected object type!");
		return false;
	}

	if (objVal.isNull())
    {
        checkJSError(cx);
		return true; // reached the end of the prototype chain
    }

	JS::RootedObject obj(cx, &objVal.toObject());
	JS::Rooted<JS::IdVector> props(cx, JS::IdVector(cx));
	if (!JS_Enumerate(cx, obj, &props))
    {
        checkJSError(cx);
		return false;
    }

	for (size_t i = 0; i < props.length(); ++i)
	{
		JS::RootedId id(cx, props[i]);
		JS::RootedValue val(cx);
		if (!JS_IdToValue(cx, id, &val))
        {
            checkJSError(cx);
			return false;
        }

		if (!val.isString())
        {
			continue; // ignore integer properties
        }

		JS::RootedString name(cx, val.toString());
		size_t len = strlen(prefix)+1;
		JS::UniqueChars ubuf = JS_EncodeStringToASCII(cx, name);
        checkJSError(cx);
        std::vector<char> buf;
        buf.resize(len);
        for(size_t i = 0; i < len-1; i++) buf[i] = ubuf.get()[i];

		buf[len-1]= '\0';
		if (0 == strcmp(&buf[0], prefix))
		{
			if (JS_StringHasLatin1Chars(name))
			{
                checkJSError(cx);
				size_t length;
				JS::AutoCheckCannotGC nogc;
				const JS::Latin1Char* chars = JS_GetLatin1StringCharsAndLength(cx, nogc, name, &length);
				if (chars)
					out.push_back(std::string(chars, chars+length));
			}
			else
			{
				size_t length;
				JS::AutoCheckCannotGC nogc;
				const char16_t* chars = JS_GetTwoByteStringCharsAndLength(cx, nogc, name, &length);
				if (chars)
					out.push_back(std::string(chars, chars+length));
                
                checkJSError(cx);
			}
		}
	}

	// Recurse up the prototype chain
	JS::RootedObject prototype(cx);
	if (JS_GetPrototype(cx, obj, &prototype))
	{
        checkJSError(cx);
		JS::RootedValue prototypeVal(cx, JS::ObjectOrNullValue(prototype));
		if (!EnumeratePropertyNamesWithPrefix(prototypeVal, prefix, out))
        {
            checkJSError(cx);
			return false;
        }
	}
    
    checkJSError(cx);

	return true;
}

bool ScriptInterface::SetPrototype(JS::HandleValue objVal, JS::HandleValue protoVal)
{
    CX_IN_REALM(cx,this)

	if (!objVal.isObject() || !protoVal.isObject())
    {
        checkJSError(cx);
		return false;
    }
	JS::RootedObject obj(cx, &objVal.toObject());
	JS::RootedObject proto(cx, &protoVal.toObject());
	return JS_SetPrototype(cx, obj, proto);
}

bool ScriptInterface::FreezeObject(JS::HandleValue objVal, bool deep) const
{
    CX_IN_REALM(cx,this)
	
    if (!objVal.isObject())
    {
        checkJSError(cx);
		return false;
    }

	JS::RootedObject obj(cx, &objVal.toObject());

    bool eval;
	if (deep) eval = JS_DeepFreezeObject(cx, obj);
	else eval = JS_FreezeObject(cx, obj);
    
    checkJSError(cx);
	return eval;
}

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::string& code) const
{
    CX_IN_REALM(cx,this)

	JS::RootedObject global(cx, m->m_glob);
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::CompileOptions options(cx);
	options.setFileAndLine(filenameStr.c_str(), lineNo);
	options.setIsRunOnce(false);

	JS::RootedObjectVector emptyScopeChain(cx);

	utf16string codeUtf16(code.begin(), code.end());
    JS::SourceText<char16_t> source;
    if (!source.init(m->m_cx, 
                     (const char16_t*)(codeUtf16.c_str()), 
                     codeUtf16.size(), 
                     JS::SourceOwnership::Borrowed)) {    
     checkJSError(cx);
     return false;                                                               
    }

	JS::RootedFunction func(cx,
                            JS::CompileFunction(cx, 
                                emptyScopeChain, 
                                options, 
                                NULL, 
                                0, 
                                NULL,
	                            source));
	
    if(!func) 
    { 
        checkJSError(cx);
        return false; 
    }

	JS::RootedValue rval(cx);
    bool eval = JS_CallFunction(cx, nullptr, func, JS::HandleValueArray::empty(), &rval);
    checkJSError(cx);
	return eval; 
}

shared_ptr<ScriptRuntime> ScriptInterface::CreateRuntime(shared_ptr<ScriptRuntime> parentRuntime, int runtimeSize, int heapGrowthBytesGCTrigger)
{
	return shared_ptr<ScriptRuntime>(new ScriptRuntime(parentRuntime, runtimeSize, heapGrowthBytesGCTrigger));
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::wstring& code) const
{
    CX_IN_REALM(cx,this)

	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::RootedValue rval(cx);
	JS::CompileOptions opts(cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	
    utf16string codeUtf16(code.begin(), code.end());
    JS::SourceText<char16_t> source;
    if (!source.init(cx, 
                     (const char16_t*)codeUtf16.c_str(), 
                     codeUtf16.size(), 
                     JS::SourceOwnership::Borrowed)) {    
     checkJSError(cx);
     return false;                                                               
    }
    
    bool eval = JS::Evaluate(cx, opts, source, &rval); 
    checkJSError(cx);
	return eval; 
}

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path) const
{
    CX_IN_REALM(cx,this)

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

	auto codeUtf8 = file.DecodeUTF8(); // assume it's UTF-8

	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = path.string8();

	JS::RootedValue rval(cx);
	JS::CompileOptions opts(cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);

    JS::SourceText<mozilla::Utf8Unit> source;
    if (!source.init(cx, 
                     codeUtf8.c_str(), 
                     codeUtf8.size(), 
                     JS::SourceOwnership::Borrowed)) 
    {    
     checkJSError(cx);
     return false;                                                               
    }

    bool eval = JS::Evaluate(cx, opts, source, &rval); 
    checkJSError(cx);
	return eval;
}


JSAutoRealm ScriptInterface::AutoRealm() const
{
    return JSAutoRealm(m->m_cx, m->m_glob);
}

bool ScriptInterface::Eval(const char* code) const
{
    CX_IN_REALM(cx,this)

	JS::RootedValue rval(cx);
	return Eval_(code, &rval);
}

bool ScriptInterface::Eval_(const char* code, JS::MutableHandleValue rval) const
{
    CX_IN_REALM(cx,this)

    JS::CompileOptions opts(cx);
	opts.setFileAndLine("(eval)", 1);

	utf16string codeUtf16(code, code+strlen(code));
    JS::SourceText<char16_t> source;
    if (!source.init(cx, 
                     (const char16_t*)codeUtf16.c_str(), 
                     codeUtf16.size(), 
                     JS::SourceOwnership::Borrowed)) {    
     checkJSError(cx);
     return false;                                                               
    }

    bool eval = JS::Evaluate(m->m_cx, opts, source, rval);
    checkJSError(cx);
	return eval; 
}

bool ScriptInterface::Eval_(const wchar_t* code, JS::MutableHandleValue rval) const
{
    CX_IN_REALM(cx,this)

	JS::CompileOptions opts(cx);
	opts.setFileAndLine("(eval)", 1);

	utf16string codeUtf16(code, code+wcslen(code));
    JS::SourceText<char16_t> source;
    if (!source.init(cx, 
                     (const char16_t*)codeUtf16.c_str(), 
                     codeUtf16.size(), 
                     JS::SourceOwnership::Borrowed)) 
    {    
        checkJSError(cx);
        return false;                                                               
    }

    bool eval = JS::Evaluate(m->m_cx, opts, source, rval);
    checkJSError(cx);
	return eval; 
}

bool ScriptInterface::ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out) const
{
    CX_IN_REALM(cx,this)

	std::wstring attrsW = wstring_from_utf8(string_utf8);
 	utf16string string(attrsW.begin(), attrsW.end());
	if (JS_ParseJSON(cx, reinterpret_cast<const char16_t*>(string.c_str()), (u32)string.size(), out))
    {
        checkJSError(cx);
		return true;
    }

	LOGERROR("JS_ParseJSON failed!");
	if (!JS_IsExceptionPending(cx))
    {
        checkJSError(cx);
		return false;
    }

	JS::RootedValue exc(m->m_cx);
	if (!JS_GetPendingException(cx, &exc))
    {
        checkJSError(cx);
		return false;
    }

	JS_ClearPendingException(cx);
	// We expect an object of type SyntaxError
	if (!exc.isObject())
    {
        checkJSError(cx);
		return false;
    }

	JS::RootedValue rval(cx);
	JS::RootedObject excObj(cx, &exc.toObject());
	if (!JS_CallFunctionName(cx, excObj, "toString", JS::HandleValueArray::empty(), &rval))
    {
        checkJSError(cx);
		return false;
    }


    checkJSError(cx);

	std::wstring error;
	ScriptInterface::FromJSVal(cx, rval, error);
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
    CX_IN_REALM(cx, this)

	Stringifier str;
	JS::RootedValue indentVal(cx, indent ? JS::Int32Value(2) : JS::UndefinedValue());
	if (!JS_Stringify(cx, obj, nullptr, indentVal, &Stringifier::callback, &str))
	{
		JS_ClearPendingException(cx);
		LOGERROR("StringifyJSON failed");
		return std::string();
	}

	return str.stream.str();
}


std::string ScriptInterface::ToString(JS::MutableHandleValue obj, bool pretty) const
{
    CX_IN_REALM(cx, this)
	
    if (obj.isUndefined())
		return "(void 0)";

	// Try to stringify as JSON if possible
	// (TODO: this is maybe a bad idea since it'll drop 'undefined' values silently)
	if (pretty)
	{
		Stringifier str;
		JS::RootedValue indentVal(m->m_cx, JS::Int32Value(2));

		// Temporary disable the error reporter, so we don't print complaints about cyclic values
        JS::WarningReporter er = JS::SetWarningReporter(cx, NULL);

		bool ok = JS_Stringify(cx, obj, nullptr, indentVal, &Stringifier::callback, &str);

		// Restore error reporter
        JS::SetWarningReporter(cx, er);

		if (ok)
			return str.stream.str();

		// Clear the exception set when Stringify failed
		JS_ClearPendingException(cx);
	}

	// Caller didn't want pretty output, or JSON conversion failed (e.g. due to cycles),
	// so fall back to obj.toSource()

	std::wstring source = L"(error)";
	CallFunction(obj, "toSource", source);
	return utf8_from_wstring(source);
}

void ScriptInterface::ReportError(const char* message, const char* filename, size_t lineno) const
{
    // JS_ReportErrorASCII by itself doesn't seem to set a JS-style exception, and so
    // script callers will be unable to catch anything. So use JS_SetPendingException
    // to make sure there really is a script-level exception. But just set it to undefined
    // because there's not much value yet in throwing a real exception object.
    // And report the actual error

    CX_IN_REALM(cx, this)

    JS::RootedString messageStr(cx, JS_NewStringCopyZ(cx, message));
    JS::RootedString filenameStr(cx, JS_NewStringCopyZ(cx, filename));

    JS::AutoValueArray<3> args(cx);
    args[0].setString(messageStr);
    args[1].setString(filenameStr);
    args[2].setInt32(lineno);
    JS::RootedValue exc(cx);
    JS::RootedObject global(cx, &(GetGlobalObject().toObject()));

    // The JSAPI code here is actually simulating `throw Error(message)` without
    // the new, as new is a bit harder to simulate using the JSAPI. In this case,
    // unless the script has redefined Error, it amounts to the same thing.
    JS_CallFunctionName(cx, global, "Error", args, &exc);

    JS_SetPendingException(cx, exc);
}

bool ScriptInterface::IsExceptionPending(JSContext* cx)
{
	return JS_IsExceptionPending(cx) ? true : false;
}

JS::Value ScriptInterface::CloneValueFromOtherContext(const ScriptInterface& otherContext, JS::HandleValue val) const
{
    CX_IN_REALM(cx, this)

	PROFILE("CloneValueFromOtherContext");
	JS::RootedValue out(cx);
	shared_ptr<StructuredClone> structuredClone = otherContext.WriteStructuredClone(val);
	ReadStructuredClone(structuredClone, &out);
	return out.get();
}

ScriptInterface::StructuredClone::StructuredClone() :
	m_data(JS::StructuredCloneScope::SameProcessDifferentThread, NULL, NULL)
{
}

ScriptInterface::StructuredClone::~StructuredClone(){}

shared_ptr<ScriptInterface::StructuredClone> ScriptInterface::WriteStructuredClone(JS::HandleValue v) const
{
    CX_IN_REALM(cx, this)

	shared_ptr<StructuredClone> ret(new StructuredClone());
	if (!ret->m_data.write(cx, v)){
		debug_warn(L"Writing a structured clone failed!");
		return shared_ptr<StructuredClone>();
	}


	return ret;
}


void ScriptInterface::ReadStructuredClone(const shared_ptr<ScriptInterface::StructuredClone>& ptr, JS::MutableHandleValue ret) const
{
    CX_IN_REALM(cx, this)

    if(!ptr->m_data.read(cx, ret)){
        debug_warn(L"Reading a structured clone failed!");
    } 
}


