/* Copyright (C) 2010 Wildfire Games.
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
#include "AutoRooters.h"

#include "lib/debug.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/utf16string.h"

#include <cassert>

#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_real.hpp>

#include "valgrind.h"

const int RUNTIME_SIZE = 4 * 1024 * 1024; // TODO: how much memory is needed?
const int STACK_CHUNK_SIZE = 8192;

#ifdef NDEBUG
#define ENABLE_SCRIPT_PROFILING 0
#else
#define ENABLE_SCRIPT_PROFILING 1
#endif

#if ENABLE_SCRIPT_PROFILING
#include "js/jsdbgapi.h"
#endif

////////////////////////////////////////////////////////////////

struct ScriptInterface_impl
{
	ScriptInterface_impl(const char* nativeScopeName, JSContext* cx);
	~ScriptInterface_impl();
	void Register(const char* name, JSNative fptr, uintN nargs);

	JSRuntime* m_rt; // NULL if m_cx is shared; non-NULL if we own m_cx
	JSContext* m_cx;
	JSObject* m_glob; // global scope object
	JSObject* m_nativeScope; // native function scope object

	AutoGCRooter* m_rooter;
};

namespace
{

JSClass global_class = {
	"global", JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

void ErrorReporter(JSContext* UNUSED(cx), const char* message, JSErrorReport* report)
{
	// XXX Ugly hack: we want to compile code with 'use strict' and with JSOPTION_STRICT,
	// but the latter causes the former to be reported as a useless expression, so
	// ignore that specific warning here
	if (report->flags == 5 && report->lineno == 0 && report->errorNumber == 163)
		return;

	std::stringstream msg;
	bool isWarning = JSREPORT_IS_WARNING(report->flags);
	msg << (isWarning ? "JavaScript warning: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}
	msg << message;
	if (isWarning)
		LOGWARNING(L"%hs", msg.str().c_str());
	else
		LOGERROR(L"%hs", msg.str().c_str());
	// When running under Valgrind, print more information in the error message
	VALGRIND_PRINTF_BACKTRACE("->");
}

// Functions in the global namespace:

JSBool print(JSContext* cx, uintN argc, jsval* vp)
{
	for (uintN i = 0; i < argc; ++i)
	{
		std::string str;
		if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[i], str))
			return JS_FALSE;
		printf("%s", str.c_str());
	}
	fflush(stdout);
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool warn(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
		return JS_FALSE;
	LOGWARNING(L"%ls", str.c_str());
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

JSBool error(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
	std::wstring str;
	if (!ScriptInterface::FromJSVal(cx, JS_ARGV(cx, vp)[0], str))
		return JS_FALSE;
	LOGERROR(L"%ls", str.c_str());
	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	return JS_TRUE;
}

// Math override functions:

JSBool Math_random(JSContext* cx, uintN UNUSED(argc), jsval* vp)
{
	// Grab the RNG that was hidden in our slot
	jsval rngp;
	if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), 0, &rngp))
		return JS_FALSE;
	boost::rand48* rng = static_cast<boost::rand48*>(JSVAL_TO_PRIVATE(rngp));

	// TODO: is the double generation sufficiently deterministic for us?
	boost::uniform_real<double> dist;
	double r = dist(*rng);

	jsval rv;
	if (!JS_NewNumberValue(cx, r, &rv))
		return JS_FALSE;
	JS_SET_RVAL(cx, vp, rv);
	return JS_TRUE;
}

} // anonymous namespace

#if ENABLE_SCRIPT_PROFILING
static void* jshook_script(JSContext* UNUSED(cx), JSStackFrame* UNUSED(fp), JSBool before, JSBool* UNUSED(ok), void* closure)
{
	if (before)
		g_Profiler.StartScript("script invocation");
	else
		g_Profiler.Stop();

	return closure;
}

static void* jshook_function(JSContext* cx, JSStackFrame* fp, JSBool before, JSBool* UNUSED(ok), void* closure)
{
	JSFunction* fn = JS_GetFrameFunction(cx, fp);
	if (before)
	{
		if (fn)
			g_Profiler.StartScript(JS_GetFunctionName(fn));
		else
			g_Profiler.StartScript("function invocation");
	}
	else
		g_Profiler.Stop();

	return closure;
}
#endif

void jshook_trace(JSTracer* trc, void* data)
{
	ScriptInterface_impl* m = static_cast<ScriptInterface_impl*>(data);

	if (m->m_rooter)
		m->m_rooter->Trace(trc);
}

ScriptInterface_impl::ScriptInterface_impl(const char* nativeScopeName, JSContext* cx) :
	m_rooter(NULL)
{
	JSBool ok;

	if (cx)
	{
		m_rt = NULL;
		m_cx = cx;
		m_glob = JS_GetGlobalObject(m_cx);
	}
	else
	{
		m_rt = JS_NewRuntime(RUNTIME_SIZE);
		debug_assert(m_rt); // TODO: error handling

#if ENABLE_SCRIPT_PROFILING
		if (CProfileManager::IsInitialised())
		{
			JS_SetExecuteHook(m_rt, jshook_script, this);
			JS_SetCallHook(m_rt, jshook_function, this);
		}
#endif

		m_cx = JS_NewContext(m_rt, STACK_CHUNK_SIZE);
		debug_assert(m_cx);

		// For GC debugging:
		// JS_SetGCZeal(m_cx, 2);

		JS_SetContextPrivate(m_cx, NULL);

		JS_SetErrorReporter(m_cx, ErrorReporter);

		JS_SetOptions(m_cx, JSOPTION_STRICT // "warn on dubious practice"
				| JSOPTION_XML // "ECMAScript for XML support: parse <!-- --> as a token"
				| JSOPTION_VAROBJFIX // "recommended" (fixes variable scoping)
		);

		JS_SetVersion(m_cx, JSVERSION_LATEST);

		JS_SetExtraGCRoots(m_rt, jshook_trace, this);

		// Threadsafe SpiderMonkey requires that we have a request before doing anything much
		JS_BeginRequest(m_cx);

		m_glob = JS_NewObject(m_cx, &global_class, NULL, NULL);
		ok = JS_InitStandardClasses(m_cx, m_glob);

		JS_DefineProperty(m_cx, m_glob, "global", OBJECT_TO_JSVAL(m_glob), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
				| JSPROP_PERMANENT);
	}

	m_nativeScope = JS_DefineObject(m_cx, m_glob, nativeScopeName, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);

	JS_DefineFunction(m_cx, m_glob, "print", (JSNative)::print, 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_FAST_NATIVE);
	JS_DefineFunction(m_cx, m_glob, "warn",  (JSNative)::warn,  1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_FAST_NATIVE);
	JS_DefineFunction(m_cx, m_glob, "error", (JSNative)::error, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_FAST_NATIVE);
}

ScriptInterface_impl::~ScriptInterface_impl()
{
	if (m_rt) // if we own the context:
	{
		JS_EndRequest(m_cx);
		JS_DestroyContext(m_cx);
		JS_DestroyRuntime(m_rt);
	}
}

void ScriptInterface_impl::Register(const char* name, JSNative fptr, uintN nargs)
{
	JS_DefineFunction(m_cx, m_nativeScope, name, fptr, nargs, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
}

ScriptInterface::ScriptInterface(const char* nativeScopeName, JSContext* cx) :
	m(new ScriptInterface_impl(nativeScopeName, cx))
{
}

ScriptInterface::~ScriptInterface()
{
}

void ScriptInterface::ShutDown()
{
	JS_ShutDown();
}

void ScriptInterface::SetCallbackData(void* cbdata)
{
	JS_SetContextPrivate(m->m_cx, cbdata);
}

void* ScriptInterface::GetCallbackData(JSContext* cx)
{
	return JS_GetContextPrivate(cx);
}

void ScriptInterface::ReplaceNondeterministicFunctions(boost::rand48& rng)
{
	jsval math;
	if (!JS_GetProperty(m->m_cx, m->m_glob, "Math", &math) || !JSVAL_IS_OBJECT(math))
	{
		LOGERROR(L"ReplaceNondeterministicFunctions: failed to get Math");
		return;
	}

	JSFunction* random = JS_DefineFunction(m->m_cx, JSVAL_TO_OBJECT(math), "random", (JSNative)Math_random, 0,
			JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSFUN_FAST_NATIVE);
	if (!random)
	{
		LOGERROR(L"ReplaceNondeterministicFunctions: failed to replace Math.random");
		return;
	}
	// Store the RNG in a slot which is sort-of-guaranteed to be unused by the JS engine
	JS_SetReservedSlot(m->m_cx, JS_GetFunctionObject(random), 0, PRIVATE_TO_JSVAL(&rng));
}

void ScriptInterface::Register(const char* name, JSNative fptr, size_t nargs)
{
	m->Register(name, fptr, (uintN)nargs);
}

JSContext* ScriptInterface::GetContext() const
{
	return m->m_cx;
}

bool ScriptInterface::AddRoot(void* ptr, const char* name)
{
	return JS_AddNamedRoot(m->m_cx, ptr, name) ? true : false;
}

bool ScriptInterface::RemoveRoot(void* ptr)
{
	return JS_RemoveRoot(m->m_cx, ptr) ? true : false;
}

AutoGCRooter* ScriptInterface::ReplaceAutoGCRooter(AutoGCRooter* rooter)
{
	debug_assert(m->m_rt); // this class must own the runtime, else the rooter won't work
	AutoGCRooter* ret = m->m_rooter;
	m->m_rooter = rooter;
	return ret;
}

ScriptInterface::LocalRootScope::LocalRootScope(JSContext* cx) :
	m_cx(cx)
{
	m_OK = JS_EnterLocalRootScope(m_cx) ? true : false;
}

ScriptInterface::LocalRootScope::~LocalRootScope()
{
	if (m_OK)
		JS_LeaveLocalRootScope(m_cx);
}

bool ScriptInterface::LocalRootScope::OK()
{
	return m_OK;
}

jsval ScriptInterface::CallConstructor(jsval ctor)
{
	// Constructing JS objects similarly to "new Foo" is non-trivial.
	// https://developer.mozilla.org/En/SpiderMonkey/JSAPI_Phrasebook#Constructing_an_object_with_new
	// suggests some ugly ways, so we'll use a different way that's less compatible but less ugly

	if (!(JSVAL_IS_OBJECT(ctor) && JS_ObjectIsFunction(m->m_cx, JSVAL_TO_OBJECT(ctor))))
	{
		LOGERROR(L"CallConstructor: ctor is not a function object");
		return JSVAL_VOID;
	}

	// Get the constructor's prototype
	// (Can't use JS_GetPrototype, since we want .prototype not .__proto__)
	jsval protoVal;
	if (!JS_GetProperty(m->m_cx, JSVAL_TO_OBJECT(ctor), "prototype", &protoVal))
	{
		LOGERROR(L"CallConstructor: can't get prototype");
		return JSVAL_VOID;
	}

	if (!JSVAL_IS_OBJECT(protoVal))
	{
		LOGERROR(L"CallConstructor: prototype is not an object");
		return JSVAL_VOID;
	}

	JSObject* proto = JSVAL_TO_OBJECT(protoVal);
	JSObject* parent = JS_GetParent(m->m_cx, JSVAL_TO_OBJECT(ctor));
	// TODO: rooting?
	if (!proto || !parent)
	{
		LOGERROR(L"CallConstructor: null proto/parent");
		return JSVAL_VOID;
	}

	JSObject* obj = JS_NewObject(m->m_cx, NULL, proto, parent);
	if (!obj)
	{
		LOGERROR(L"CallConstructor: object creation failed");
		return JSVAL_VOID;
	}

	jsval rval;
	if (!JS_CallFunctionValue(m->m_cx, obj, ctor, 0, NULL, &rval))
	{
		LOGERROR(L"CallConstructor: ctor failed");
		return JSVAL_VOID;
	}

	return OBJECT_TO_JSVAL(obj);
}

bool ScriptInterface::CallFunctionVoid(jsval val, const char* name)
{
	jsval jsRet;
	std::vector<jsval> argv;
	return CallFunction_(val, name, argv, jsRet);
}

bool ScriptInterface::CallFunction_(jsval val, const char* name, std::vector<jsval>& args, jsval& ret)
{
	const uintN argc = args.size();
	jsval* argv = NULL;
	if (argc)
		argv = &args[0];

	JSObject* obj;
	if (!JS_ValueToObject(m->m_cx, val, &obj) || obj == NULL)
		return false;
	JS_AddRoot(m->m_cx, &obj);

	// Check that the named function actually exists, to avoid ugly JS error reports
	// when calling an undefined value
	JSBool found;
	if (!JS_HasProperty(m->m_cx, obj, name, &found) || !found)
	{
		JS_RemoveRoot(m->m_cx, &obj);
		return false;
	}

	JSBool ok = JS_CallFunctionName(m->m_cx, obj, name, argc, argv, &ret);
	JS_RemoveRoot(m->m_cx, &obj);

	return ok ? true : false;
}

jsval ScriptInterface::GetGlobalObject()
{
	return OBJECT_TO_JSVAL(JS_GetGlobalObject(m->m_cx));
}

bool ScriptInterface::SetGlobal_(const char* name, jsval value, bool replace)
{
	if (!replace)
	{
		JSBool found;
		if (!JS_HasProperty(m->m_cx, m->m_glob, name, &found))
			return false;
		if (found)
		{
			JS_ReportError(m->m_cx, "SetGlobal \"%s\" called multiple times", name);
			return false;
		}
	}

	JSBool ok = JS_DefineProperty(m->m_cx, m->m_glob, name, value, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY
			| JSPROP_PERMANENT);
	return ok ? true : false;
}

bool ScriptInterface::SetProperty_(jsval obj, const char* name, jsval value, bool constant)
{
	uintN attrs;
	if (constant)
		attrs = JSPROP_READONLY | JSPROP_PERMANENT;
	else
		attrs = JSPROP_ENUMERATE;

	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	if (! JS_DefineProperty(m->m_cx, object, name, value, NULL, NULL, attrs))
		return false;
	return true;
}

bool ScriptInterface::GetProperty_(jsval obj, const char* name, jsval& out)
{
	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	if (!JS_GetProperty(m->m_cx, object, name, &out))
		return false;
	return true;
}

bool ScriptInterface::HasProperty(jsval obj, const char* name)
{
	if (! JSVAL_IS_OBJECT(obj))
		return false;
	JSObject* object = JSVAL_TO_OBJECT(obj);

	JSBool found;
	if (!JS_HasProperty(m->m_cx, object, name, &found))
		return false;
	return (found != JS_FALSE);
}

bool ScriptInterface::EnumeratePropertyNamesWithPrefix(jsval obj, const char* prefix, std::vector<std::string>& out)
{
	LOCAL_ROOT_SCOPE;

	utf16string prefix16 (prefix, prefix+strlen(prefix));

	if (! JSVAL_IS_OBJECT(obj))
		return false; // TODO: log error messages

	JSObject* it = JS_NewPropertyIterator(m->m_cx, JSVAL_TO_OBJECT(obj));
	if (!it)
		return false;

	while (true)
	{
		jsid idp;
		jsval val;
		if (! JS_NextProperty(m->m_cx, it, &idp) || ! JS_IdToValue(m->m_cx, idp, &val))
			return false;
		if (val == JSVAL_VOID)
			break; // end of iteration
		if (! JSVAL_IS_STRING(val))
			continue; // ignore integer properties

		JSString* name = JSVAL_TO_STRING(val);
		size_t len = JS_GetStringLength(name);
		jschar* chars = JS_GetStringChars(name);
		if (len >= prefix16.size() && memcmp(chars, prefix16.c_str(), prefix16.size()*2) == 0)
			out.push_back(std::string(chars, chars+len)); // handles Unicode poorly
	}

	// Recurse up the prototype chain
	JSObject* prototype = JS_GetPrototype(m->m_cx, JSVAL_TO_OBJECT(obj));
	if (prototype)
	{
		if (! EnumeratePropertyNamesWithPrefix(OBJECT_TO_JSVAL(prototype), prefix, out))
			return false;
	}

	return true;
}

bool ScriptInterface::SetPrototype(jsval obj, jsval proto)
{
	if (!JSVAL_IS_OBJECT(obj) || !JSVAL_IS_OBJECT(proto))
		return false;
	return JS_SetPrototype(m->m_cx, JSVAL_TO_OBJECT(obj), JSVAL_TO_OBJECT(proto)) ? true : false;
}

bool ScriptInterface::LoadScript(const std::wstring& filename, const std::wstring& code)
{
	std::string fnAscii(filename.begin(), filename.end());

	// Compile the code in strict mode, to encourage better coding practices and
	// to possibly help SpiderMonkey with optimisations
	std::wstring codeStrict = L"\"use strict\";\n" + code;
	utf16string codeUtf16(codeStrict.begin(), codeStrict.end());
	uintN lineNo = 0; // put the automatic 'use strict' on line 0, so the real code starts at line 1

	JSFunction* func = JS_CompileUCFunction(m->m_cx, NULL, NULL, 0, NULL, reinterpret_cast<const jschar*> (codeUtf16.c_str()), (uintN)(codeUtf16.length()), fnAscii.c_str(), lineNo);
	if (!func)
		return false;

	JS_AddRoot(m->m_cx, &func); // TODO: do we need to root this?
	jsval scriptRval;
	JSBool ok = JS_CallFunction(m->m_cx, NULL, func, 0, NULL, &scriptRval);
	JS_RemoveRoot(m->m_cx, &func);

	return ok ? true : false;
}

bool ScriptInterface::Eval(const char* code)
{
	jsval rval;
	return Eval_(code, rval);
}

bool ScriptInterface::Eval_(const char* code, jsval& rval)
{
	utf16string codeUtf16(code, code+strlen(code));

	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob, (const jschar*)codeUtf16.c_str(), (uintN)codeUtf16.length(), "(eval)", 1, &rval);
	return ok ? true : false;
}

bool ScriptInterface::Eval_(const wchar_t* code, jsval& rval)
{
	utf16string codeUtf16(code, code+wcslen(code));

	JSBool ok = JS_EvaluateUCScript(m->m_cx, m->m_glob, (const jschar*)codeUtf16.c_str(), (uintN)codeUtf16.length(), "(eval)", 1, &rval);
	return ok ? true : false;
}

std::wstring ScriptInterface::ToString(jsval obj)
{
	if (JSVAL_IS_VOID(obj))
		return L"(void 0)";
	std::wstring source = L"(error)";
	CallFunction(obj, "toSource", source);
	return source;
}

void ScriptInterface::ReportError(const char* msg)
{
	// JS_ReportError by itself doesn't seem to set a JS-style exception, and so
	// script callers will be unable to catch anything. So use JS_SetPendingException
	// to make sure there really is a script-level exception. But just set it to undefined
	// because there's not much value yet in throwing a real exception object.
	JS_SetPendingException(m->m_cx, JSVAL_VOID);
	// And report the actual error
	JS_ReportError(m->m_cx, "%s", msg);

	// TODO: Why doesn't JS_ReportPendingException(m->m_cx); work?
}

bool ScriptInterface::IsExceptionPending(JSContext* cx)
{
	return JS_IsExceptionPending(cx) ? true : false;
}

JSClass* ScriptInterface::GetClass(JSContext* cx, JSObject* obj)
{
	UNUSED2(cx); // unused if not JS_THREADSAFE

	return JS_GET_CLASS(cx, obj);
}

void* ScriptInterface::GetPrivate(JSContext* cx, JSObject* obj)
{
	// TODO: use JS_GetInstancePrivate
	return JS_GetPrivate(cx, obj);
}

void ScriptInterface::DumpHeap()
{
#ifdef DEBUG
	JS_DumpHeap(m->m_cx, stderr, NULL, 0, NULL, (size_t)-1, NULL);
#endif
	fprintf(stderr, "# Bytes allocated: %d\n", JS_GetGCParameter(m->m_rt, JSGC_BYTES));
	JS_GC(m->m_cx);
	fprintf(stderr, "# Bytes allocated after GC: %d\n", JS_GetGCParameter(m->m_rt, JSGC_BYTES));
}


class ValueCloner
{
public:
	ValueCloner(JSContext* cxFrom, JSContext* cxTo) : cxFrom(cxFrom), cxTo(cxTo)
	{
	}

	// Return the cloned object (or an already-computed object if we've cloned val before)
	jsval GetOrClone(jsval val)
	{
		if (!JSVAL_IS_GCTHING(val) || JSVAL_IS_NULL(val))
			return val;

		std::map<jsval, jsval>::iterator it = m_Mapping.find(val);
		if (it != m_Mapping.end())
			return it->second;

		m_ValuesOld.push_back(CScriptValRooted(cxFrom, val)); // root it so our mapping doesn't get invalidated

		return Clone(val);
	}

private:

#define CLONE_REQUIRE(expr, msg) if (!(expr)) { debug_warn(L"Internal error in CloneValueFromOtherContext: " msg); return JSVAL_VOID; }

	// Clone a new value (and root it and add it to the mapping)
	jsval Clone(jsval val)
	{
		if (JSVAL_IS_DOUBLE(val))
		{
			jsval rval;
			CLONE_REQUIRE(JS_NewNumberValue(cxTo, *JSVAL_TO_DOUBLE(val), &rval), L"JS_NewNumberValue");
			m_Mapping[val] = rval;
			m_ValuesNew.push_back(CScriptValRooted(cxTo, rval));
			return rval;
		}

		if (JSVAL_IS_STRING(val))
		{
			JSString* str = JS_NewUCStringCopyN(cxTo, JS_GetStringChars(JSVAL_TO_STRING(val)), JS_GetStringLength(JSVAL_TO_STRING(val)));
			CLONE_REQUIRE(str, L"JS_NewUCStringCopyN");
			jsval rval = STRING_TO_JSVAL(str);
			m_Mapping[val] = rval;
			m_ValuesNew.push_back(CScriptValRooted(cxTo, rval));
			return rval;
		}

		debug_assert(JSVAL_IS_OBJECT(val));

		JSObject* newObj;
		if (JS_IsArrayObject(cxFrom, JSVAL_TO_OBJECT(val)))
		{
			jsuint length;
			CLONE_REQUIRE(JS_GetArrayLength(cxFrom, JSVAL_TO_OBJECT(val), &length), L"JS_GetArrayLength");
			newObj = JS_NewArrayObject(cxTo, length, NULL);
			CLONE_REQUIRE(newObj, L"JS_NewArrayObject");
		}
		else
		{
			newObj = JS_NewObject(cxTo, NULL, NULL, NULL);
			CLONE_REQUIRE(newObj, L"JS_NewObject");
		}

		m_Mapping[val] = OBJECT_TO_JSVAL(newObj);
		m_ValuesNew.push_back(CScriptValRooted(cxTo, OBJECT_TO_JSVAL(newObj)));

		JSIdArray* ida = JS_Enumerate(cxFrom, JSVAL_TO_OBJECT(val));
		CLONE_REQUIRE(ida, L"JS_Enumerate");

		IdArrayWrapper idaWrapper(cxFrom, ida);

		for (jsint i = 0; i < ida->length; ++i)
		{
			jsid id = ida->vector[i];
			jsval idval, propval;
			CLONE_REQUIRE(JS_IdToValue(cxFrom, id, &idval), L"JS_IdToValue");
			CLONE_REQUIRE(JS_GetPropertyById(cxFrom, JSVAL_TO_OBJECT(val), id, &propval), L"JS_GetPropertyById");
			jsval newPropval = GetOrClone(propval);

			if (JSVAL_IS_INT(idval))
			{
				// int jsids are portable across runtimes
				CLONE_REQUIRE(JS_SetPropertyById(cxTo, newObj, id, &newPropval), L"JS_SetPropertyById");
			}
			else if (JSVAL_IS_STRING(idval))
			{
				// string jsids are runtime-specific, so we need to copy the string content
				JSString* idstr = JS_ValueToString(cxFrom, idval);
				CLONE_REQUIRE(idstr, L"JS_ValueToString (id)");
				CLONE_REQUIRE(JS_SetUCProperty(cxTo, newObj, JS_GetStringChars(idstr), JS_GetStringLength(idstr), &newPropval), L"JS_SetUCProperty");
			}
			else
			{
				// this apparently could be an XML object; ignore it
			}
		}

		return OBJECT_TO_JSVAL(newObj);
	}

	JSContext* cxFrom;
	JSContext* cxTo;
	std::map<jsval, jsval> m_Mapping;
	std::deque<CScriptValRooted> m_ValuesOld;
	std::deque<CScriptValRooted> m_ValuesNew;
};

jsval ScriptInterface::CloneValueFromOtherContext(const ScriptInterface& otherContext, jsval val)
{
	PROFILE("CloneValueFromOtherContext");

	JSContext* cxTo = GetContext();
	JSContext* cxFrom = otherContext.GetContext();

	ValueCloner cloner(cxFrom, cxTo);
	return cloner.GetOrClone(val);
}
