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

	friend ScriptInterface::Request;
	private:
		JSContext* m_cx;
		JS::PersistentRootedObject m_glob; // global scope object

	public:
		boost::rand48* m_rng;
		JS::PersistentRootedObject m_nativeScope; // native function scope object
};

ScriptInterface::Request::Request(const ScriptInterface& scriptInterface) :
	cx(scriptInterface.m->m_cx)
{
	JS_BeginRequest(cx);
	m_formerCompartment = JS_EnterCompartment(cx, scriptInterface.m->m_glob);
	glob = JS::CurrentGlobalOrNull(cx);
}

JS::Value ScriptInterface::Request::globalValue() const
{
	return JS::ObjectValue(*glob);
}

ScriptInterface::Request::~Request()
{
	JS_LeaveCompartment(cx, m_formerCompartment);
	JS_EndRequest(cx);
}

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

// Functions in the global namespace:

bool print(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \

	for (uint i = 0; i < args.length(); ++i)
	{
		std::wstring str;
		if (!ScriptInterface::FromJSVal(rq, args[i], str))
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

	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	std::wstring str;
	if (!ScriptInterface::FromJSVal(rq, args[0], str))
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

	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	std::wstring str;
	if (!ScriptInterface::FromJSVal(rq, args[0], str))
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

	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	std::wstring str;
	if (!ScriptInterface::FromJSVal(rq, args[0], str))
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

	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	JS::RootedValue ret(cx);
	if (!JS_StructuredClone(rq.cx, args[0], &ret, NULL, NULL))
		return false;

	args.rval().set(ret);
	return true;
}

bool deepfreeze(JSContext* cx, uint argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \

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
	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	if (args.length() >= 1)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(rq, args[0], str))
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
	ScriptInterface::Request rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface); \
	if (args.length() >= 1)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(rq, args[0], str))
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
	m_runtime(runtime), m_cx(runtime->GetGeneralJSContext()), m_glob(runtime->GetJSRuntime()), m_nativeScope(runtime->GetJSRuntime())
{
	JS::CompartmentOptions opt;
	opt.setVersion(JSVERSION_LATEST);
	// Keep JIT code during non-shrinking GCs. This brings a quite big performance improvement.
	opt.setPreserveJitCode(true);

	JSAutoRequest rq(m_cx);
	m_glob = JS_NewGlobalObject(m_cx, &global_class, nullptr, JS::OnNewGlobalHookOption::FireOnNewGlobalHook, opt);

	JSAutoCompartment autoCmpt(m_cx, m_glob);

	ENSURE(JS_InitStandardClasses(m_cx, m_glob));

	JS_DefineProperty(m_cx, m_glob, "global", m_glob, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, nullptr, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, m_glob, "print", ::print,        0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "log",   ::logmsg,       1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "warn",  ::warn,         1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "error", ::error,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "clone", ::deepcopy,        1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(m_cx, m_glob, "deepfreeze", ::deepfreeze, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);

	Register("ProfileStart", ::ProfileStart, 1);
	Register("ProfileStop", ::ProfileStop, 0);
	Register("ProfileAttribute", ::ProfileAttribute, 1);

	m_runtime->RegisterCompartment(js::GetObjectCompartment(m_glob));
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	m_runtime->UnRegisterCompartment(js::GetObjectCompartment(m_glob));
}

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uint nargs) const
{
	JSAutoRequest rq(m_cx);
	JSAutoCompartment autoCmpt(m_cx, m_glob);
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

	Request rq(this);
	m_CmptPrivate.pScriptInterface = this;
	JS_SetCompartmentPrivate(js::GetObjectCompartment(rq.glob), (void*)&m_CmptPrivate);
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
	m_CmptPrivate.pCBData = pCBData;
}

ScriptInterface::CmptPrivate* ScriptInterface::GetScriptInterfaceAndCBData(JSContext* cx)
{
	CmptPrivate* pCmptPrivate = (CmptPrivate*)JS_GetCompartmentPrivate(js::GetContextCompartment(cx));
	return pCmptPrivate;
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
	Request rq(this);
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

	LOGERROR("ReplaceNondeterministicRNG: failed to replace Math.random");
	return false;
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs) const
{
	m->Register(name, fptr, (uint)nargs);
}

JSRuntime* ScriptInterface::GetJSRuntime() const
{
	return m->m_runtime->GetJSRuntime();
}

shared_ptr<ScriptRuntime> ScriptInterface::GetRuntime() const
{
	return m->m_runtime;
}

void ScriptInterface::CallConstructor(JS::HandleValue ctor, JS::HandleValueArray argv, JS::MutableHandleValue out) const
{
	Request rq(this);
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
	Request rq(this);
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

	if (obj == NULL)
		throw PSERROR_Scripting_DefineType_CreationFailed();

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

	Request rq(this);
	JS::RootedObject prototype(rq.cx, it->second.m_Prototype.get());
	return JS_NewObjectWithGivenProto(rq.cx, it->second.m_Class, prototype);
}

bool ScriptInterface::CallFunction_(JS::HandleValue val, const char* name, JS::HandleValueArray argv, JS::MutableHandleValue ret) const
{
	Request rq(this);
	JS::RootedObject obj(rq.cx);
	if (!JS_ValueToObject(rq.cx, val, &obj) || !obj)
		return false;

	// Check that the named function actually exists, to avoid ugly JS error reports
	// when calling an undefined value
	bool found;
	if (!JS_HasProperty(rq.cx, obj, name, &found) || !found)
		return false;

	return JS_CallFunctionName(rq.cx, obj, name, argv, ret);
}

bool ScriptInterface::CreateObject_(const Request& rq, JS::MutableHandleObject object)
{
	object.set(JS_NewPlainObject(rq.cx));

	if (!object)
		throw PSERROR_Scripting_CreateObjectFailed();

	return true;
}

void ScriptInterface::CreateArray(const Request& rq, JS::MutableHandleValue objectValue, size_t length)
{
	objectValue.setObjectOrNull(JS_NewArrayObject(rq.cx, length));
	if (!objectValue.isObject())
		throw PSERROR_Scripting_CreateObjectFailed();
}

bool ScriptInterface::SetGlobal_(const char* name, JS::HandleValue value, bool replace, bool constant, bool enumerate)
{
	Request rq(this);
	JS::RootedObject global(rq.cx, rq.glob);

	bool found;
	if (!JS_HasProperty(rq.cx, global, name, &found))
		return false;
	if (found)
	{
		JS::Rooted<JSPropertyDescriptor> desc(rq.cx);
		if (!JS_GetOwnPropertyDescriptor(rq.cx, global, name, &desc))
			return false;

		if (!desc.writable())
		{
			if (!replace)
			{
				JS_ReportError(rq.cx, "SetGlobal \"%s\" called multiple times", name);
				return false;
			}

			// This is not supposed to happen, unless the user has called SetProperty with constant = true on the global object
			// instead of using SetGlobal.
			if (!desc.configurable())
			{
				JS_ReportError(rq.cx, "The global \"%s\" is permanent and cannot be hotloaded", name);
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

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const char* name, JS::HandleValue value, bool constant, bool enumerate) const
{
	Request rq(this);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	if (!JS_DefineProperty(rq.cx, object, name, value, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetProperty_(JS::HandleValue obj, const wchar_t* name, JS::HandleValue value, bool constant, bool enumerate) const
{
	Request rq(this);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	utf16string name16(name, name + wcslen(name));
	if (!JS_DefineUCProperty(rq.cx, object, reinterpret_cast<const char16_t*>(name16.c_str()), name16.length(), value, attrs))
		return false;
	return true;
}

bool ScriptInterface::SetPropertyInt_(JS::HandleValue obj, int name, JS::HandleValue value, bool constant, bool enumerate) const
{
	Request rq(this);
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerate)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	JS::RootedId id(rq.cx, INT_TO_JSID(name));
	if (!JS_DefinePropertyById(rq.cx, object, id, value, attrs))
		return false;
	return true;
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleValue out) const
{
	return GetProperty_(obj, name, out);
}

bool ScriptInterface::GetProperty(JS::HandleValue obj, const char* name, JS::MutableHandleObject out) const
{
	Request rq(this);
	JS::RootedValue val(rq.cx);
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
	Request rq(this);
	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	if (!JS_GetProperty(rq.cx, object, name, out))
		return false;
	return true;
}

bool ScriptInterface::GetPropertyInt_(JS::HandleValue obj, int name, JS::MutableHandleValue out) const
{
	Request rq(this);
	JS::RootedId nameId(rq.cx, INT_TO_JSID(name));
	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	if (!JS_GetPropertyById(rq.cx, object, nameId, out))
		return false;
	return true;
}

bool ScriptInterface::HasProperty(JS::HandleValue obj, const char* name) const
{
	// TODO: proper errorhandling
	Request rq(this);
	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	bool found;
	if (!JS_HasProperty(rq.cx, object, name, &found))
		return false;
	return found;
}

bool ScriptInterface::EnumeratePropertyNames(JS::HandleValue objVal, bool enumerableOnly, std::vector<std::string>& out) const
{
	Request rq(this);

	if (!objVal.isObjectOrNull())
	{
		LOGERROR("EnumeratePropertyNames expected object type!");
		return false;
	}

	JS::RootedObject obj(rq.cx, &objVal.toObject());
	JS::AutoIdVector props(rq.cx);
	// This recurses up the prototype chain on its own.
	if (!js::GetPropertyKeys(rq.cx, obj, enumerableOnly? 0 : JSITER_HIDDEN, &props))
		return false;

	out.reserve(out.size() + props.length());
	for (size_t i = 0; i < props.length(); ++i)
	{
		JS::RootedId id(rq.cx, props[i]);
		JS::RootedValue val(rq.cx);
		if (!JS_IdToValue(rq.cx, id, &val))
			return false;

		// Ignore integer properties for now.
		// TODO: is this actually a thing in ECMAScript 6?
		if (!val.isString())
			continue;

		std::string propName;
		if (!FromJSVal(rq, val, propName))
			return false;

		out.emplace_back(std::move(propName));
	}

	return true;
}

bool ScriptInterface::SetPrototype(JS::HandleValue objVal, JS::HandleValue protoVal)
{
	Request rq(this);
	if (!objVal.isObject() || !protoVal.isObject())
		return false;
	JS::RootedObject obj(rq.cx, &objVal.toObject());
	JS::RootedObject proto(rq.cx, &protoVal.toObject());
	return JS_SetPrototype(rq.cx, obj, proto);
}

bool ScriptInterface::FreezeObject(JS::HandleValue objVal, bool deep) const
{
	Request rq(this);
	if (!objVal.isObject())
		return false;

	JS::RootedObject obj(rq.cx, &objVal.toObject());

	if (deep)
		return JS_DeepFreezeObject(rq.cx, obj);
	else
		return JS_FreezeObject(rq.cx, obj);
}

bool ScriptInterface::LoadScript(const VfsPath& filename, const std::string& code) const
{
	Request rq(this);
	JS::RootedObject global(rq.cx, rq.glob);
	utf16string codeUtf16(code.begin(), code.end());
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::CompileOptions options(rq.cx);
	options.setFileAndLine(filenameStr.c_str(), lineNo);
	options.setIsRunOnce(false);

	JS::RootedFunction func(rq.cx);
	JS::AutoObjectVector emptyScopeChain(rq.cx);
	if (!JS::CompileFunction(rq.cx, emptyScopeChain, options, NULL, 0, NULL,
	                         reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &func))
		return false;

	JS::RootedValue rval(rq.cx);
	return JS_CallFunction(rq.cx, nullptr, func, JS::HandleValueArray::empty(), &rval);
}

bool ScriptInterface::LoadGlobalScript(const VfsPath& filename, const std::wstring& code) const
{
	Request rq(this);
	utf16string codeUtf16(code.begin(), code.end());
	uint lineNo = 1;
	// CompileOptions does not copy the contents of the filename string pointer.
	// Passing a temporary string there will cause undefined behaviour, so we create a separate string to avoid the temporary.
	std::string filenameStr = filename.string8();

	JS::RootedValue rval(rq.cx);
	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	return JS::Evaluate(rq.cx, opts,
			reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &rval);
}

bool ScriptInterface::LoadGlobalScriptFile(const VfsPath& path) const
{
	Request rq(this);
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

	JS::RootedValue rval(rq.cx);
	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine(filenameStr.c_str(), lineNo);
	return JS::Evaluate(rq.cx, opts,
			reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)(codeUtf16.length()), &rval);
}

bool ScriptInterface::Eval(const char* code) const
{
	Request rq(this);
	JS::RootedValue rval(rq.cx);
	return Eval_(code, &rval);
}

bool ScriptInterface::Eval_(const char* code, JS::MutableHandleValue rval) const
{
	Request rq(this);
	utf16string codeUtf16(code, code+strlen(code));

	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(rq.cx, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::Eval_(const wchar_t* code, JS::MutableHandleValue rval) const
{
	Request rq(this);
	utf16string codeUtf16(code, code+wcslen(code));

	JS::CompileOptions opts(rq.cx);
	opts.setFileAndLine("(eval)", 1);
	return JS::Evaluate(rq.cx, opts, reinterpret_cast<const char16_t*>(codeUtf16.c_str()), (uint)codeUtf16.length(), rval);
}

bool ScriptInterface::ParseJSON(const std::string& string_utf8, JS::MutableHandleValue out) const
{
	Request rq(this);
	std::wstring attrsW = wstring_from_utf8(string_utf8);
 	utf16string string(attrsW.begin(), attrsW.end());
	if (JS_ParseJSON(rq.cx, reinterpret_cast<const char16_t*>(string.c_str()), (u32)string.size(), out))
		return true;

	LOGERROR("JS_ParseJSON failed!");
	if (!JS_IsExceptionPending(rq.cx))
		return false;

	JS::RootedValue exc(rq.cx);
	if (!JS_GetPendingException(rq.cx, &exc))
		return false;

	JS_ClearPendingException(rq.cx);
	// We expect an object of type SyntaxError
	if (!exc.isObject())
		return false;

	JS::RootedValue rval(rq.cx);
	JS::RootedObject excObj(rq.cx, &exc.toObject());
	if (!JS_CallFunctionName(rq.cx, excObj, "toString", JS::HandleValueArray::empty(), &rval))
		return false;

	std::wstring error;
	ScriptInterface::FromJSVal(rq, rval, error);
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
	Request rq(this);
	Stringifier str;
	JS::RootedValue indentVal(rq.cx, indent ? JS::Int32Value(2) : JS::UndefinedValue());
	if (!JS_Stringify(rq.cx, obj, nullptr, indentVal, &Stringifier::callback, &str))
	{
		JS_ClearPendingException(rq.cx);
		LOGERROR("StringifyJSON failed");
		return std::string();
	}

	return str.stream.str();
}


std::string ScriptInterface::ToString(JS::MutableHandleValue obj, bool pretty) const
{
	Request rq(this);

	if (obj.isUndefined())
		return "(void 0)";

	// Try to stringify as JSON if possible
	// (TODO: this is maybe a bad idea since it'll drop 'undefined' values silently)
	if (pretty)
	{
		Stringifier str;
		JS::RootedValue indentVal(rq.cx, JS::Int32Value(2));

		// Temporary disable the error reporter, so we don't print complaints about cyclic values
		JSErrorReporter er = JS_SetErrorReporter(GetJSRuntime(), nullptr);

		bool ok = JS_Stringify(rq.cx, obj, nullptr, indentVal, &Stringifier::callback, &str);

		// Restore error reporter
		JS_SetErrorReporter(GetJSRuntime(), er);

		if (ok)
			return str.stream.str();

		// Clear the exception set when Stringify failed
		JS_ClearPendingException(rq.cx);
	}

	// Caller didn't want pretty output, or JSON conversion failed (e.g. due to cycles),
	// so fall back to obj.toSource()

	std::wstring source = L"(error)";
	CallFunction(obj, "toSource", source);
	return utf8_from_wstring(source);
}

void ScriptInterface::ReportError(const char* msg) const
{
	Request rq(this);
	// JS_ReportError by itself doesn't seem to set a JS-style exception, and so
	// script callers will be unable to catch anything. So use JS_SetPendingException
	// to make sure there really is a script-level exception. But just set it to undefined
	// because there's not much value yet in throwing a real exception object.
	JS_SetPendingException(rq.cx, JS::UndefinedHandleValue);
	// And report the actual error
	JS_ReportError(rq.cx, "%s", msg);

	// TODO: Why doesn't JS_ReportPendingException(cx); work?
}

bool ScriptInterface::IsExceptionPending(const Request& rq)
{
	return JS_IsExceptionPending(rq.cx) ? true : false;
}

JS::Value ScriptInterface::CloneValueFromOtherCompartment(const ScriptInterface& otherCompartment, JS::HandleValue val) const
{
	PROFILE("CloneValueFromOtherCompartment");
	Request rq(this);
	JS::RootedValue out(rq.cx);
	shared_ptr<StructuredClone> structuredClone = otherCompartment.WriteStructuredClone(val);
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
	Request rq(this);
	u64* data = NULL;
	size_t nbytes = 0;
	if (!JS_WriteStructuredClone(rq.cx, v, &data, &nbytes, NULL, NULL, JS::UndefinedHandleValue))
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
	Request rq(this);
	JS_ReadStructuredClone(rq.cx, ptr->m_Data, ptr->m_Size, JS_STRUCTURED_CLONE_VERSION, ret, NULL, NULL);
}
